#ifdef KQUEUE
#include <sys/event.h>
#else
#include <sys/epoll.h>
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

void _hs_connection_process_io(http_request_t *request);

void hs_write_cb(http_request_t *request) {
  request->state = HTTP_SESSION_WRITE;
  _hs_connection_process_io(request);
}

void hs_read(http_request_t *request) {
  request->state = HTTP_SESSION_READ;
  _hs_connection_process_io(request);
}

void _hs_connection_process_io(http_request_t *request) {
  if (request->state == HTTP_SESSION_READ) {
    struct hs_read_opts_s opts;
    opts.initial_request_buf_capacity = HTTP_REQUEST_BUF_SIZE;
    opts.max_request_buf_capacity = HTTP_MAX_REQUEST_BUF_SIZE;
    opts.eof_rc = 0;

    enum hs_read_rc_e rc = hs_read_socket(request, opts);
    switch (rc) {
    case HS_READ_RC_PARSE_ERR:
      hs_respond_error(request, 400, "Bad Request", hs_write_cb);
      break;
    case HS_READ_RC_SOCKET_ERR:
      hs_terminate_connection(request);
      break;
    case HS_READ_RC_SUCCESS:
      break;
    }
  } else if (request->state == HTTP_SESSION_WRITE) {
    enum hs_write_rc_e rc = hs_write_socket(request);
    switch (rc) {
    case HS_WRITE_RC_SUCCESS_CLOSE:
    case HS_WRITE_RC_SOCKET_ERR:
      // Error or response complete, connection: close
      hs_terminate_connection(request);
      break;
    case HS_WRITE_RC_SUCCESS:
      // Response complete, keep-alive connection
      hs_read(request);
      break;
    default:
      break;
    }
  }
}

void _hs_mem_error_responder(http_request_t *request) {
  hs_respond_error(request, 503, "Service Unavailable", hs_write_cb);
}

#ifdef KQUEUE

void hs_connection_io_cb(struct kevent *ev) {
  http_request_t *request = (http_request_t *)ev->udata;
  if (ev->filter == EVFILT_TIMER) {
    request->timeout -= 1;
    if (request->timeout == 0)
      hs_terminate_connection(request);
  } else {
    _hs_connection_process_io(request);
  }
}

void hs_accept_cb(struct kevent *ev) {
  http_server_t *server = (http_server_t *)ev->udata;
  if (ev->filter == EVFILT_TIMER) {
    hs_generate_date_time(server->date);
  } else {
    hs_accept_connections(server, hs_connection_io_cb, _hs_mem_error_responder,
                          HTTP_MAX_TOTAL_EST_MEM_USAGE);
  }
}

#else

void hs_connection_io_cb(struct epoll_event *ev) {
  _hs_connection_process_io((http_request_t *)ev->data.ptr);
}

void hs_accept_cb(struct epoll_event *ev) {
  hs_accept_connections((http_server_t *)ev->data.ptr, hs_connection_io_cb,
                        _hs_mem_error_responder);
}

void hs_server_timer_cb(struct epoll_event *ev) {
  http_server_t *server =
      (http_server_t *)((char *)ev->data.ptr - sizeof(epoll_cb_t));
  uint64_t res;
  int bytes = read(server->timerfd, &res, sizeof(res));
  (void)bytes; // suppress warning
  hs_generate_date_time(server->date);
}

void hs_request_timer_cb(struct epoll_event *ev) {
  http_request_t *request =
      (http_request_t *)((char *)ev->data.ptr - sizeof(epoll_cb_t));
  uint64_t res;
  int bytes = read(request->timerfd, &res, sizeof(res));
  (void)bytes; // suppress warning
  request->timeout -= 1;
  if (request->timeout == 0)
    hs_terminate_connection(request);
}

#endif
