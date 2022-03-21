void hs_server_listen_cb(struct epoll_event* ev) {
  hs_accept_connections((http_server_t*)ev->data.ptr);
}

void hs_session_io_cb(struct epoll_event* ev) {
  http_session((http_request_t*)ev->data.ptr);
}

void hs_server_timer_cb(struct epoll_event* ev) {
  http_server_t* server = (http_server_t*)((char*)ev->data.ptr - sizeof(epoll_cb_t));
  uint64_t res;
  int bytes = read(server->timerfd, &res, sizeof(res));
  (void)bytes; // suppress warning
  hs_generate_date_time(server->date);
}

void hs_request_timer_cb(struct epoll_event* ev) {
  http_request_t* request = (http_request_t*)((char*)ev->data.ptr - sizeof(epoll_cb_t));
  uint64_t res;
  int bytes = read(request->timerfd, &res, sizeof(res));
  (void)bytes; // suppress warning
  request->timeout -= 1;
  if (request->timeout == 0) hs_end_session(request);
}

void hs_add_server_sock_events(http_server_t* serv) {
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = serv;
  epoll_ctl(serv->loop, EPOLL_CTL_ADD, serv->socket, &ev);
}

void hs_server_init(http_server_t* serv) {
  serv->loop = epoll_create1(0);
  serv->timer_handler = hs_server_timer_cb;

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

int http_server_listen_addr(http_server_t* serv, const char* ipaddr) {
  http_listen(serv, ipaddr);
  struct epoll_event ev_list[1];
  while (1) {
    int nev = epoll_wait(serv->loop, ev_list, 1, -1);
    for (int i = 0; i < nev; i++) {
      ev_cb_t* ev_cb = (ev_cb_t*)ev_list[i].data.ptr;
      ev_cb->handler(&ev_list[i]);
    }
  }
  return 0;
}

int http_server_listen(http_server_t* serv) {
  return http_server_listen_addr(serv, NULL);
}

void hs_delete_events(http_request_t* request) {
  epoll_ctl(request->server->loop, EPOLL_CTL_DEL, request->socket, NULL);
  epoll_ctl(request->server->loop, EPOLL_CTL_DEL, request->timerfd, NULL);
  close(request->timerfd);
}

int http_server_poll(http_server_t* serv) {
  struct epoll_event ev;
  int nev = epoll_wait(serv->loop, &ev, 1, 0);
  if (nev <= 0) return nev;
  ev_cb_t* ev_cb = (ev_cb_t*)ev.data.ptr;
  ev_cb->handler(&ev);
  return nev;
}

void hs_add_events(http_request_t* request) {
  request->timer_handler = hs_request_timer_cb;

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

void hs_add_write_event(http_request_t* request) {
  struct epoll_event ev;
  ev.events = EPOLLOUT | EPOLLET;
  ev.data.ptr = request;
  epoll_ctl(request->server->loop, EPOLL_CTL_MOD, request->socket, &ev);
}

