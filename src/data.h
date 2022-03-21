#ifndef DATA_H
#define DATA_H

#ifdef __linux__
#define EPOLL
#define _POSIX_C_SOURCE 199309L
#else
#define KQUEUE
#endif

#ifdef KQUEUE
#include <sys/event.h>
#else
#include <sys/epoll.h>
#include <sys/timerfd.h>
#endif

#include <netinet/in.h>
#include "http_parser.h"


// http session states
enum hs_session_state_e {
  HTTP_SESSION_INIT,
  HTTP_SESSION_READ,
  HTTP_SESSION_WRITE,
  HTTP_SESSION_NOP
};

#define HTTP_REQUEST_TIMEOUT 20

enum hs_event_e {
  HS_EVT_NONE,
  HS_EVT_BODY_CALLBACK,
  HS_EVT_PARSER_ERR,
  HS_EVT_SOCKET_CLOSED,
  HS_EVT_REQUEST_CALLBACK
};

#ifdef EPOLL
typedef void (*epoll_cb_t)(struct epoll_event*);
#endif

typedef struct http_ev_cb_s {
#ifdef KQUEUE
  void (*handler)(struct kevent* ev);
#else
  epoll_cb_t handler;
#endif
} ev_cb_t;

typedef struct http_request_s {
#ifdef KQUEUE
  void (*handler)(struct kevent* ev);
#else
  epoll_cb_t handler;
  epoll_cb_t timer_handler;
  int timerfd;
#endif
  void (*chunk_cb)(struct http_request_s*);
  void* data;

  struct hsh_buffer_s buffer;
  struct hsh_parser_s parser;
  struct hs_token_array_s tokens;
  int state;
  int socket;
  int timeout;

  int64_t bytes_written;
  struct http_server_s* server;
  char flags;
} http_request_t;

typedef struct http_server_s {
#ifdef KQUEUE
  void (*handler)(struct kevent* ev);
#else
  epoll_cb_t handler;
  epoll_cb_t timer_handler;
#endif
  int64_t memused;
  int socket;
  int port;
  int loop;
  int timerfd;
  socklen_t len;
  void (*request_handler)(http_request_t*);
  struct sockaddr_in addr;
  void* data;
  char date[32];
} http_server_t;

typedef struct http_header_s {
  char const * key;
  char const * value;
  struct http_header_s* next;
} http_header_t;

typedef struct http_response_s {
  http_header_t* headers;
  char const * body;
  int content_length;
  int status;
} http_response_t;

typedef struct http_string_s http_string_t;

#endif
