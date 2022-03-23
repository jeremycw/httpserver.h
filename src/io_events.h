#ifndef HS_IO_EVENTS_H
#define HS_IO_EVENTS_H

#define HTTP_REQUEST_BUF_SIZE 1024
#define HTTP_MAX_REQUEST_BUF_SIZE 8388608 // 8mb
#define HTTP_MAX_TOTAL_EST_MEM_USAGE 4294967296 // 4gb

struct http_request_s;

void hs_write_cb(struct http_request_s* request);
void hs_read(struct http_request_s* request);

#ifdef KQUEUE

void hs_connection_io_cb(struct kevent* ev);
void hs_accept_cb(struct kevent* ev);

#else

void hs_connection_io_cb(struct epoll_event* ev);
void hs_accept_cb(struct epoll_event* ev);
void hs_server_timer_cb(struct epoll_event* ev);
void hs_request_timer_cb(struct epoll_event* ev);

#endif

#endif
