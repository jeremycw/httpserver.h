#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef KQUEUE
#include <sys/event.h>
#else
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#endif

#ifndef HTTPSERVER_IMPL
#include "buffer_util.h"
#include "common.h"
#include "connection.h"
#endif

#ifdef KQUEUE

void _hs_delete_events(http_request_t *request) {
  struct kevent ev_set;
  EV_SET(&ev_set, request->socket, EVFILT_TIMER, EV_DELETE, 0, 0, request);
  kevent(request->server->loop, &ev_set, 1, NULL, 0, NULL);
}

void _hs_add_timer_event(http_request_t *request, hs_io_cb_t unused) {
  (void)unused;

  struct kevent ev_set;
  EV_SET(&ev_set, request->socket, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 1000,
         request);
  kevent(request->server->loop, &ev_set, 1, NULL, 0, NULL);
}

#else

void _hs_delete_events(http_request_t *request) {
  epoll_ctl(request->server->loop, EPOLL_CTL_DEL, request->socket, NULL);
  epoll_ctl(request->server->loop, EPOLL_CTL_DEL, request->timerfd, NULL);
  close(request->timerfd);
}

void _hs_add_timer_event(http_request_t *request, hs_io_cb_t timer_cb) {
  request->timer_handler = timer_cb;

  // Watch for read events
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = request;
  epoll_ctl(request->server->loop, EPOLL_CTL_ADD, request->socket, &ev);

  // Add timer to timeout requests.
  int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
  struct itimerspec ts = {};
  ts.it_value.tv_sec = 1;
  ts.it_interval.tv_sec = 1;
  timerfd_settime(tfd, 0, &ts, NULL);

  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = &request->timer_handler;
  epoll_ctl(request->server->loop, EPOLL_CTL_ADD, tfd, &ev);
  request->timerfd = tfd;
}

#endif

void hs_request_terminate_connection(http_request_t *request) {
  _hs_delete_events(request);
  close(request->socket);
  _hs_buffer_free(&request->buffer, &request->server->memused);
  free(request->tokens.buf);
  request->tokens.buf = NULL;
  free(request);
}

void _hs_token_array_init(struct hs_token_array_s *array, int capacity) {
  array->buf =
      (struct hsh_token_s *)malloc(sizeof(struct hsh_token_s) * capacity);
  assert(array->buf != NULL);
  array->size = 0;
  array->capacity = capacity;
}

http_request_t *_hs_request_init(int sock, http_server_t *server,
                                 hs_io_cb_t io_cb) {
  http_request_t *request = (http_request_t *)calloc(1, sizeof(http_request_t));
  assert(request != NULL);
  request->socket = sock;
  request->server = server;
  request->handler = io_cb;
  request->timeout = HTTP_REQUEST_TIMEOUT;
  request->flags = HTTP_AUTOMATIC;
  request->parser = (struct hsh_parser_s){};
  request->buffer = (struct hsh_buffer_s){};
  request->tokens.buf = NULL;
  _hs_token_array_init(&request->tokens, 32);
  return request;
}

http_request_t *hs_server_accept_connection(http_server_t *server,
                                            hs_io_cb_t io_cb,
                                            hs_io_cb_t epoll_timer_cb) {
  http_request_t *request = NULL;
  int sock = 0;

  sock = accept(server->socket, (struct sockaddr *)&server->addr, &server->len);

  if (sock > 0) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    request = _hs_request_init(sock, server, io_cb);
    _hs_add_timer_event(request, epoll_timer_cb);
  }
  return request;
}
