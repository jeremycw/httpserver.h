#ifndef HS_COMMON_H
#define HS_COMMON_H

// http session states
#define HTTP_SESSION_INIT 0
#define HTTP_SESSION_READ 1
#define HTTP_SESSION_WRITE 2
#define HTTP_SESSION_NOP 3

#define HTTP_REQUEST_TIMEOUT 20

#define HTTP_FLAG_SET(var, flag) var |= flag
#define HTTP_FLAG_CLEAR(var, flag) var &= ~flag
#define HTTP_FLAG_CHECK(var, flag) (var & flag)

#define HTTP_AUTOMATIC 0x8
#define HTTP_CHUNKED_RESPONSE 0x20

#define HTTP_KEEP_ALIVE 1
#define HTTP_CLOSE 0

#include <arpa/inet.h>
#include <sys/socket.h>
#ifdef KQUEUE
#include <sys/event.h>
#else
#include <sys/epoll.h>
#endif

#ifdef EPOLL
typedef void (*epoll_cb_t)(struct epoll_event *);
#endif

typedef struct http_ev_cb_s {
#ifdef KQUEUE
  void (*handler)(struct kevent *ev);
#else
  epoll_cb_t handler;
#endif
} ev_cb_t;

struct hsh_buffer_s {
  char *buf;
  int32_t capacity;
  int32_t length;
  int32_t index;
  int32_t after_headers_index;
  int8_t sequence_id;
};

enum hsh_token_e {
  HSH_TOK_METHOD,
  HSH_TOK_TARGET,
  HSH_TOK_VERSION,
  HSH_TOK_HEADER_KEY,
  HSH_TOK_HEADER_VALUE,
  HSH_TOK_HEADERS_DONE,
  HSH_TOK_BODY,
  HSH_TOK_NONE,
  HSH_TOK_ERR
};

struct hsh_token_s {
  enum hsh_token_e type;
  uint8_t flags;
  int len;
  int index;
};

struct hsh_parser_s {
  int64_t content_length;
  int64_t content_remaining;
  struct hsh_token_s token;
  int16_t limit_count;
  int16_t limit_max;
  int8_t state;
  int8_t flags;
  int8_t sequence_id;
};

struct hs_token_array_s {
  struct hsh_token_s *buf;
  int capacity;
  int size;
};

typedef struct http_request_s {
#ifdef KQUEUE
  void (*handler)(struct kevent *ev);
#else
  epoll_cb_t handler;
  epoll_cb_t timer_handler;
  int timerfd;
#endif
  void (*chunk_cb)(struct http_request_s *);
  void *data;
  struct hsh_buffer_s buffer;
  struct hsh_parser_s parser;
  struct hs_token_array_s tokens;
  int state;
  int socket;
  int timeout;
  int64_t bytes_written;
  struct http_server_s *server;
  char flags;
} http_request_t;

typedef struct http_server_s {
#ifdef KQUEUE
  void (*handler)(struct kevent *ev);
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
  void (*request_handler)(http_request_t *);
  struct sockaddr_in addr;
  void *data;
  char date[32];
} http_server_t;

#endif
