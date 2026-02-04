#include <stdlib.h>

#ifdef KQUEUE
#include <sys/event.h>
#else
#include <stdint.h>
#include <sys/epoll.h>
#include <unistd.h>
#endif

#ifndef HTTPSERVER_IMPL
#include "buffer_util.h"
#include "common.h"
#include "connection.h"
#include "io_events.h"
#include "read_socket.h"
#include "respond.h"
#include "server.h"
#include "write_socket.h"
#endif

void _hs_read_socket_and_handle_return_code(http_request_t *request) {
  struct hs_read_opts_s opts;
  opts.initial_request_buf_capacity = HTTP_REQUEST_BUF_SIZE;
  opts.max_request_buf_capacity = HTTP_MAX_REQUEST_BUF_SIZE;
  opts.eof_rc = 0;

  enum hs_read_rc_e rc = hs_read_request_and_exec_user_cb(request, opts);
  switch (rc) {
  case HS_READ_RC_PARSE_ERR:
    hs_request_respond_error(request, 400, "Bad Request",
                             hs_request_begin_write);
    break;
  case HS_READ_RC_SOCKET_ERR:
    hs_request_terminate_connection(request);
    break;
  case HS_READ_RC_SUCCESS:
    break;
  }
}

void hs_request_begin_read(http_request_t *request);

void _hs_write_socket_and_handle_return_code(http_request_t *request) {
  enum hs_write_rc_e rc = hs_write_socket(request);

  request->timeout = rc == HS_WRITE_RC_SUCCESS ? HTTP_KEEP_ALIVE_TIMEOUT
                                               : HTTP_REQUEST_TIMEOUT;

  if (rc != HS_WRITE_RC_CONTINUE)
    _hs_buffer_free(&request->buffer, &request->server->memused);

  switch (rc) {
  case HS_WRITE_RC_SUCCESS_CLOSE:
  case HS_WRITE_RC_SOCKET_ERR:
    // Error or response complete, connection: close
    hs_request_terminate_connection(request);
    break;
  case HS_WRITE_RC_SUCCESS:
    // Response complete, keep-alive connection
    hs_request_begin_read(request);
    break;
  case HS_WRITE_RC_SUCCESS_CHUNK:
    // Finished writing chunk, request next
    request->state = HTTP_SESSION_NOP;
    request->chunk_cb(request);
    break;
  case HS_WRITE_RC_CONTINUE:
    break;
  }
}

void _hs_accept_and_begin_request_cycle(http_server_t *server,
                                        hs_io_cb_t on_client_connection_cb,
                                        hs_io_cb_t on_timer_event_cb) {
  http_request_t *request = NULL;
  while ((request = hs_server_accept_connection(server, on_client_connection_cb,
                                                on_timer_event_cb))) {
    if (server->memused > HTTP_MAX_TOTAL_EST_MEM_USAGE) {
      hs_request_respond_error(request, 503, "Service Unavailable",
                               hs_request_begin_write);
    } else {
      hs_request_begin_read(request);
    }
  }
}

#ifdef KQUEUE

void _hs_on_kqueue_client_connection_event(struct kevent *ev) {
  http_request_t *request = (http_request_t *)ev->udata;
  if (ev->filter == EVFILT_TIMER) {
    request->timeout -= 1;
    if (request->timeout == 0)
      hs_request_terminate_connection(request);
  } else {
    if (request->state == HTTP_SESSION_READ) {
      _hs_read_socket_and_handle_return_code(request);
    } else if (request->state == HTTP_SESSION_WRITE) {
      _hs_write_socket_and_handle_return_code(request);
    }
  }
}

void hs_on_kqueue_server_event(struct kevent *ev) {
  http_server_t *server = (http_server_t *)ev->udata;
  if (ev->filter == EVFILT_TIMER) {
    hs_generate_date_time(server->date);
  } else {
    _hs_accept_and_begin_request_cycle(
        server, _hs_on_kqueue_client_connection_event, NULL);
  }
}

#else

void _hs_on_epoll_client_connection_event(struct epoll_event *ev) {
  http_request_t *request = (http_request_t *)ev->data.ptr;
  if (request->state == HTTP_SESSION_READ) {
    _hs_read_socket_and_handle_return_code(request);
  } else if (request->state == HTTP_SESSION_WRITE) {
    _hs_write_socket_and_handle_return_code(request);
  }
}

void _hs_on_epoll_request_timer_event(struct epoll_event *ev) {
  http_request_t *request =
      (http_request_t *)((char *)ev->data.ptr - sizeof(epoll_cb_t));
  uint64_t res;
  int bytes = read(request->timerfd, &res, sizeof(res));
  (void)bytes; // suppress warning
  request->timeout -= 1;
  if (request->timeout == 0)
    hs_request_terminate_connection(request);
}

void hs_on_epoll_server_connection_event(struct epoll_event *ev) {
  _hs_accept_and_begin_request_cycle((http_server_t *)ev->data.ptr,
                                     _hs_on_epoll_client_connection_event,
                                     _hs_on_epoll_request_timer_event);
}

void hs_on_epoll_server_timer_event(struct epoll_event *ev) {
  http_server_t *server =
      (http_server_t *)((char *)ev->data.ptr - sizeof(epoll_cb_t));
  uint64_t res;
  int bytes = read(server->timerfd, &res, sizeof(res));
  (void)bytes; // suppress warning
  hs_generate_date_time(server->date);
}

#endif

void _hs_add_write_event(http_request_t *request) {
#ifdef KQUEUE
  struct kevent ev_set[2];
  EV_SET(&ev_set[0], request->socket, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0,
         request);
  EV_SET(&ev_set[1], request->socket, EVFILT_READ, EV_DISABLE, 0, 0, request);
  kevent(request->server->loop, ev_set, 2, NULL, 0, NULL);
#else
  struct epoll_event ev;
  ev.events = EPOLLOUT | EPOLLET;
  ev.data.ptr = request;
  epoll_ctl(request->server->loop, EPOLL_CTL_MOD, request->socket, &ev);
#endif
}

void hs_request_begin_write(http_request_t *request) {
  request->state = HTTP_SESSION_WRITE;
  _hs_add_write_event(request);
  _hs_write_socket_and_handle_return_code(request);
}

void _hs_add_read_event(http_request_t *request) {
#ifdef KQUEUE
  // No action needed for kqueue since it's read event stays active. Should
  // it be disabled during write?
  struct kevent ev_set;
  EV_SET(&ev_set, request->socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
         request);
  kevent(request->server->loop, &ev_set, 1, NULL, 0, NULL);
#else
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = request;
  epoll_ctl(request->server->loop, EPOLL_CTL_MOD, request->socket, &ev);
#endif
}

void hs_request_begin_read(http_request_t *request) {
  request->state = HTTP_SESSION_READ;
  _hs_add_read_event(request);
  _hs_read_socket_and_handle_return_code(request);
}
