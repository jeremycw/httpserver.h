void hs_server_listen_cb(struct kevent* ev) {
  http_server_t* server = (http_server_t*)ev->udata;
  if (ev->filter == EVFILT_TIMER) {
    hs_generate_date_time(server->date);
  } else {
    hs_accept_connections(server);
  }
}

void hs_session_io_cb(struct kevent* ev) {
  http_request_t* request = (http_request_t*)ev->udata;
  if (ev->filter == EVFILT_TIMER) {
    request->timeout -= 1;
    if (request->timeout == 0) hs_end_session(request);
  } else {
    http_session(request);
  }
}

void hs_server_init(http_server_t* serv) {
  serv->loop = kqueue();
  struct kevent ev_set;
  EV_SET(&ev_set, 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, NOTE_SECONDS, 1, serv);
  kevent(serv->loop, &ev_set, 1, NULL, 0, NULL);
}

void hs_add_server_sock_events(http_server_t* serv) {
  struct kevent ev_set;
  EV_SET(&ev_set, serv->socket, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, serv);
  kevent(serv->loop, &ev_set, 1, NULL, 0, NULL);
}

int http_server_listen_addr(http_server_t* serv, const char* ipaddr) {
  http_listen(serv, ipaddr);

  struct kevent ev_list[1];

  while (1) {
    int nev = kevent(serv->loop, NULL, 0, ev_list, 1, NULL);
    for (int i = 0; i < nev; i++) {
      ev_cb_t* ev_cb = (ev_cb_t*)ev_list[i].udata;
      ev_cb->handler(&ev_list[i]);
    }
  }
  return 0;
}

int http_server_listen(http_server_t* serv) {
  return http_server_listen_addr(serv, NULL);
}

void hs_delete_events(http_request_t* request) {
  struct kevent ev_set;
  EV_SET(&ev_set, request->socket, EVFILT_TIMER, EV_DELETE, 0, 0, request);
  kevent(request->server->loop, &ev_set, 1, NULL, 0, NULL);
}

int http_server_poll(http_server_t* serv) {
  struct kevent ev;
  struct timespec ts = {0, 0};
  int nev = kevent(serv->loop, NULL, 0, &ev, 1, &ts);
  if (nev <= 0) return nev;
  ev_cb_t* ev_cb = (ev_cb_t*)ev.udata;
  ev_cb->handler(&ev);
  return nev;
}

void hs_add_events(http_request_t* request) {
  struct kevent ev_set[2];
  EV_SET(&ev_set[0], request->socket, EVFILT_READ, EV_ADD, 0, 0, request);
  EV_SET(&ev_set[1], request->socket, EVFILT_TIMER, EV_ADD | EV_ENABLE, NOTE_SECONDS, 1, request);
  kevent(request->server->loop, ev_set, 2, NULL, 0, NULL);
}

void hs_add_write_event(http_request_t* request) {
  struct kevent ev_set[2];
  EV_SET(&ev_set[0], request->socket, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, request);
  kevent(request->server->loop, ev_set, 2, NULL, 0, NULL);
}
