#ifndef HS_CONNECTION_H
#define HS_CONNECTION_H

#ifdef KQUEUE
typedef void (*hs_io_cb_t)(struct kevent *ev);
#else
typedef void (*hs_io_cb_t)(struct epoll_event *ev);
#endif

struct http_request_s;
struct http_server_s;

void hs_terminate_connection(struct http_request_s *request);
void hs_accept_connections(struct http_server_s *server, hs_io_cb_t io_cb,
                           void (*err_responder)(struct http_request_s *),
                           int64_t max_mem_usage);

#endif
