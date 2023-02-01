#ifdef KQUEUE
#include <sys/event.h>
#else
#include <sys/epoll.h>
#include <unistd.h>
#endif

#ifndef HTTPSERVER_IMPL
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
    hs_respond_error(request, 400, "Bad Request", hs_begin_write);
    break;
  case HS_READ_RC_SOCKET_ERR:
    hs_terminate_connection(request);
    break;
  case HS_READ_RC_SUCCESS:
    break;
  }
}

void hs_begin_read(http_request_t *request);

void _hs_write_socket_and_handle_return_code(http_request_t *request) {
  enum hs_write_rc_e rc = hs_write_socket(request);
  switch (rc) {
  case HS_WRITE_RC_SUCCESS_CLOSE:
  case HS_WRITE_RC_SOCKET_ERR:
    // Error or response complete, connection: close
    hs_terminate_connection(request);
    break;
  case HS_WRITE_RC_SUCCESS:
    // Response complete, keep-alive connection
    hs_begin_read(request);
    break;
  default:
    break;
  }
}

void _hs_mem_error_responder(http_request_t *request) {
  hs_respond_error(request, 503, "Service Unavailable", hs_begin_write);
}

#ifdef KQUEUE

void _hs_on_kqueue_client_connection_event(struct kevent *ev) {
  http_request_t *request = (http_request_t *)ev->udata;
  if (ev->filter == EVFILT_TIMER) {
    request->timeout -= 1;
    if (request->timeout == 0)
      hs_terminate_connection(request);
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
    hs_accept_connections(server, _hs_on_kqueue_client_connection_event, 0,
                          _hs_mem_error_responder,
                          HTTP_MAX_TOTAL_EST_MEM_USAGE);
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
    hs_terminate_connection(request);
}

void hs_on_epoll_server_connection_event(struct epoll_event *ev) {
  hs_accept_connections((http_server_t *)ev->data.ptr,
                        _hs_on_epoll_client_connection_event,
                        _hs_on_epoll_request_timer_event,
                        _hs_mem_error_responder, HTTP_MAX_TOTAL_EST_MEM_USAGE);
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

void hs_begin_write(http_request_t *request) {
  request->state = HTTP_SESSION_WRITE;
  _hs_write_socket_and_handle_return_code(request);
}

void hs_begin_read(http_request_t *request) {
  request->state = HTTP_SESSION_READ;
  _hs_read_socket_and_handle_return_code(request);
}
