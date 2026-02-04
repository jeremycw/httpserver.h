#ifndef HS_SERVER_H
#define HS_SERVER_H

#ifdef KQUEUE

struct kevent;

typedef void (*hs_evt_cb_t)(struct kevent *ev);

#else

struct epoll_event;

typedef void (*hs_evt_cb_t)(struct epoll_event *ev);

#endif

struct http_request_s;
struct http_server_s;

void hs_server_listen_on_addr(struct http_server_s *serv, const char *ipaddr);
int hs_server_run_event_loop(struct http_server_s *serv, const char *ipaddr);
void hs_generate_date_time(char *datetime);
struct http_server_s *hs_server_init(int port,
                                     void (*handler)(struct http_request_s *),
                                     hs_evt_cb_t accept_cb,
                                     hs_evt_cb_t timer_cb);
int hs_server_poll_events(struct http_server_s *serv);

#endif
