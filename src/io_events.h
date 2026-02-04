#ifndef HS_IO_EVENTS_H
#define HS_IO_EVENTS_H

#define HTTP_REQUEST_BUF_SIZE 1024
#define HTTP_MAX_REQUEST_BUF_SIZE 8388608       // 8mb
#define HTTP_MAX_TOTAL_EST_MEM_USAGE 4294967296 // 4gb

struct http_request_s;

void hs_request_begin_write(struct http_request_s *request);
void hs_request_begin_read(struct http_request_s *request);

#ifdef KQUEUE

struct kevent;

void hs_on_kqueue_server_event(struct kevent *ev);

#else

struct epoll_event;

void hs_on_epoll_server_connection_event(struct epoll_event *ev);
void hs_on_epoll_server_timer_event(struct epoll_event *ev);

#endif

#endif
