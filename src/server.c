#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>

#ifdef EPOLL
#include <sys/epoll.h>
#include <sys/timerfd.h>
#else
#include <sys/event.h>
#endif

#ifndef HTTPSERVER_IMPL
#include "common.h"
#include "server.h"
#endif

void _hs_bind_localhost(int s, struct sockaddr_in *addr, const char *ipaddr,
                        int port) {
  addr->sin_family = AF_INET;
  if (ipaddr == NULL) {
    addr->sin_addr.s_addr = INADDR_ANY;
  } else {
    addr->sin_addr.s_addr = inet_addr(ipaddr);
  }
  addr->sin_port = htons(port);
  int rc = bind(s, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
  if (rc < 0) {
    exit(1);
  }
}

#ifdef KQUEUE

void _hs_add_server_sock_events(http_server_t *serv) {
  struct kevent ev_set;
  EV_SET(&ev_set, serv->socket, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, serv);
  kevent(serv->loop, &ev_set, 1, NULL, 0, NULL);
}

void _hs_server_init_events(http_server_t *serv, hs_evt_cb_t unused) {
  (void)unused;

  serv->loop = kqueue();
  struct kevent ev_set;
  EV_SET(&ev_set, 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, NOTE_SECONDS, 1, serv);
  kevent(serv->loop, &ev_set, 1, NULL, 0, NULL);
}

int hs_server_run_event_loop(http_server_t *serv, const char *ipaddr) {
  hs_server_listen_on_addr(serv, ipaddr);

  struct kevent ev_list[1];

  while (1) {
    int nev = kevent(serv->loop, NULL, 0, ev_list, 1, NULL);
    for (int i = 0; i < nev; i++) {
      ev_cb_t *ev_cb = (ev_cb_t *)ev_list[i].udata;
      ev_cb->handler(&ev_list[i]);
    }
  }
  return 0;
}

int hs_server_poll_events(http_server_t *serv) {
  struct kevent ev;
  struct timespec ts = {0, 0};
  int nev = kevent(serv->loop, NULL, 0, &ev, 1, &ts);
  if (nev <= 0)
    return nev;
  ev_cb_t *ev_cb = (ev_cb_t *)ev.udata;
  ev_cb->handler(&ev);
  return nev;
}

#else

void _hs_server_init_events(http_server_t *serv, hs_evt_cb_t timer_cb) {
  serv->loop = epoll_create1(0);
  serv->timer_handler = timer_cb;

  int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
  struct itimerspec ts = {};
  ts.it_value.tv_sec = 1;
  ts.it_interval.tv_sec = 1;
  timerfd_settime(tfd, 0, &ts, NULL);

  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = &serv->timer_handler;
  epoll_ctl(serv->loop, EPOLL_CTL_ADD, tfd, &ev);
  serv->timerfd = tfd;
}

void _hs_add_server_sock_events(http_server_t *serv) {
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = serv;
  epoll_ctl(serv->loop, EPOLL_CTL_ADD, serv->socket, &ev);
}

int hs_server_run_event_loop(http_server_t *serv, const char *ipaddr) {
  hs_server_listen_on_addr(serv, ipaddr);
  struct epoll_event ev_list[1];
  while (1) {
    int nev = epoll_wait(serv->loop, ev_list, 1, -1);
    for (int i = 0; i < nev; i++) {
      ev_cb_t *ev_cb = (ev_cb_t *)ev_list[i].data.ptr;
      ev_cb->handler(&ev_list[i]);
    }
  }
  return 0;
}

int hs_server_poll_events(http_server_t *serv) {
  struct epoll_event ev;
  int nev = epoll_wait(serv->loop, &ev, 1, 0);
  if (nev <= 0)
    return nev;
  ev_cb_t *ev_cb = (ev_cb_t *)ev.data.ptr;
  ev_cb->handler(&ev);
  return nev;
}

#endif

void hs_server_listen_on_addr(http_server_t *serv, const char *ipaddr) {
  // Ignore SIGPIPE. We handle these errors at the call site.
  signal(SIGPIPE, SIG_IGN);
  serv->socket = socket(AF_INET, SOCK_STREAM, 0);
  int flag = 1;
  setsockopt(serv->socket, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag));
  _hs_bind_localhost(serv->socket, &serv->addr, ipaddr, serv->port);
  serv->len = sizeof(serv->addr);
  int flags = fcntl(serv->socket, F_GETFL, 0);
  fcntl(serv->socket, F_SETFL, flags | O_NONBLOCK);
  listen(serv->socket, 128);
  _hs_add_server_sock_events(serv);
}

void hs_generate_date_time(char *datetime) {
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = gmtime(&rawtime);
  strftime(datetime, 32, "%a, %d %b %Y %T GMT", timeinfo);
}

http_server_t *hs_server_init(int port, void (*handler)(http_request_t *),
                              hs_evt_cb_t accept_cb,
                              hs_evt_cb_t epoll_timer_cb) {
  http_server_t *serv = (http_server_t *)malloc(sizeof(http_server_t));
  assert(serv != NULL);
  serv->port = port;
  serv->memused = 0;
  serv->handler = accept_cb;
  _hs_server_init_events(serv, epoll_timer_cb);
  hs_generate_date_time(serv->date);
  serv->request_handler = handler;
  return serv;
}
