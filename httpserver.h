#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#define FIBER_IMPL
#ifndef FIBER_H
#define FIBER_H

#include <ev.h>

#define FIBER_TIMEOUT 1

typedef void* fiber_t;

#define _spawn_fiber(_1, _2, _3, NAME, ...) NAME

#define fiber_spawn(...) _spawn_fiber(__VA_ARGS__, \
    spawn_fiber2, spawn_fiber1, _err)(__VA_ARGS__)

#define spawn_fiber_main(coroutine, param) \
  ctx->state = 0; \
  ctx->arg = param; \
  ev_init(&ctx->timer, coroutine##_timeout); \
  ctx->timer.data = ctx; \
  ev_io_init(&ctx->io, coroutine##_callback, -1, EV_READ); \
  await_t await = call_##coroutine(&ctx->state, ctx->arg); \
  fiber_schedule_or_finish(coroutine)

#define spawn_fiber1(coroutine, param) \
  coroutine##_ctx_t* ctx = malloc(sizeof(coroutine##_ctx_t)); \
  spawn_fiber_main(coroutine, param)

#define spawn_fiber2(coroutine, param, fiber_handle) \
  coroutine##_ctx_t* ctx = malloc(sizeof(coroutine##_ctx_t)); \
  fiber_handle = (void*)ctx; \
  spawn_fiber_main(coroutine, param)

#define fiber_resume(coroutine, handle) \
  coroutine##_ctx_t* ctx = (coroutine##_ctx_t*)handle; \
  await_t await = call_##coroutine(&ctx->state, ctx->arg); \
  fiber_schedule_or_finish(coroutine)

#define fiber_schedule_or_finish(coroutine) \
  ev_io_stop(fiber_scheduler, &ctx->io); \
  if (ctx->state == -1) { \
    await.fd = -1; \
    ev_timer_stop(fiber_scheduler, &ctx->timer); \
    free(ctx); \
  } else { \
    if (await.timeout > 0.f) { \
      ctx->timer.repeat = await.timeout; \
      ev_timer_again(fiber_scheduler, &ctx->timer); \
    } else if (await.timeout < 0.f) { \
      ev_timer_stop(fiber_scheduler, &ctx->timer); \
    } \
    ctx->io.data = ctx; \
    ev_io_init(&ctx->io, coroutine##_callback, await.fd, await.type); \
    ev_io_start(fiber_scheduler, &ctx->io); \
  }

#define fiber_decl(name, type) \
  typedef struct { \
    int state; \
    type arg; \
    ev_timer timer; \
    ev_io io; \
  } name##_ctx_t; \
  void name##_callback(EV_P_ ev_io *w, int revents); \
  void name##_timeout(EV_P_ ev_timer *w, int revents); \
  await_t call_##name(int* state, type arg);

#define fiber_defn(name, type) \
  void name##_callback(EV_P_ ev_io *w, int revents) { \
    name##_ctx_t* ctx = (name##_ctx_t*)w->data; \
    await_t await = call_##name(&ctx->state, ctx->arg); \
    fiber_schedule_or_finish(name) \
  } \
  void name##_timeout(EV_P_ ev_timer *w, int revents) { \
    name##_ctx_t* ctx = (name##_ctx_t*)w->data; \
    fibererror = FIBER_TIMEOUT; \
    await_t await = call_##name(&ctx->state, ctx->arg); \
    fiber_schedule_or_finish(name) \
  }

#define fiber_scheduler_init() fiber_scheduler = EV_DEFAULT

#define fiber_scheduler_run() ev_run(fiber_scheduler, 0)

typedef struct {
  int fd;
  int type;
  float timeout;
} await_t;

void schedule_fiber(await_t await, ev_io* io, void* ctx, void(*cb)(struct ev_loop*, ev_io*, int));
await_t fiber_await(int fd, int type, float timeout);
await_t fiber_pause();

extern struct ev_loop* fiber_scheduler;
extern int fibererror;

#endif

#ifdef FIBER_IMPL
#ifndef FIBER_IMPL_ONCE
#define FIBER_IMPL_ONCE

int fibererror = 0;
struct ev_loop* fiber_scheduler;

await_t fiber_pause() {
  return (await_t) {
    .fd = -1,
    .type = 0,
    .timeout = -1.f
  };
}

await_t fiber_await(int fd, int type, float timeout) {
  return (await_t) {
    .fd = fd,
    .type = type,
    .timeout = timeout
  };
}

#endif
#endif
#ifndef VARRAY_H
#define VARRAY_H

#include <stdlib.h>

#define varray_decl(type) \
  typedef struct { \
    type* buf; \
    int capacity; \
    int size; \
  } varray_##type##_t; \
  void varray_##type##_push(varray_##type##_t* varray, type a); \
  void varray_##type##_init(varray_##type##_t* varray, int capacity);

#define varray_defn(type) \
  void varray_##type##_push(varray_##type##_t* varray, type a) { \
    if (varray->size == varray->capacity) { \
      varray->capacity *= 2; \
      varray->buf = realloc(varray->buf, varray->capacity * sizeof(type)); \
    } \
    varray->buf[varray->size] = a; \
    varray->size++; \
  } \
  void varray_##type##_init(varray_##type##_t* varray, int capacity) { \
    varray->buf = malloc(sizeof(type) * capacity); \
    varray->size = 0; \
    varray->capacity = capacity; \
  }

#define varray_t(type) \
  varray_##type##_t

#define varray_push(type, varray, a) \
  varray_##type##_push(varray, a); 

#define varray_init(type, varray, capacity) \
  varray_##type##_init(varray, capacity);

#endif
#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define HTTP_METHOD 0
#define HTTP_TARGET 1
#define HTTP_VERSION 2
#define HTTP_HEADER_KEY 3
#define HTTP_HEADER_VALUE 4
#define HTTP_NONE 6
#define HTTP_BODY 7

typedef struct {
  int index;
  int len;
  int type;
} http_token_t;

typedef struct {
  int content_length;
  int len;
  int token_start_index;
  int start;
  int content_length_i;
  char in_content_length;
  char state;
  char sub_state;
} http_parser_t;

http_token_t http_parse(http_parser_t* parser, char* input, int n);

#endif
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <netdb.h>
#ifndef HTTPSERVER_H
#include "fiber.h"
#include "varray.h"
#include "http_parser.h"
#endif

varray_decl(http_token_t);

typedef struct {
  http_parser_t parser;
  int socket;
  char* buf;
  int bytes;
  int capacity;
  struct http_server_s* server;
  fiber_t fiber;
  http_token_t token;
  varray_t(http_token_t) tokens;
  char flags;
} http_request_t;

typedef struct http_server_s {
  int socket;
  int port;
  socklen_t len;
  void (*request_handler)(http_request_t*);
  struct sockaddr_in addr;
  ev_timer timer;
  char* date;
} http_server_t;

fiber_decl(http_server_listen, http_server_t*);
fiber_decl(http_session, http_request_t*);

struct ev_loop* http_server_loop();
int http_server_listen(http_server_t* serv);
void http_server_init(http_server_t* serv, int port, void (*handler)(http_request_t*));

#endif
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#ifndef HTTPSERVER_H
#include "http_server.h"
#endif

#define HTTP_ACTIVE 0x1
#define HTTP_READY 0x2
#define HTTP_RESPONSE_READY 0x4
#define HTTP_KEEP_ALIVE 0x8
#define HTTP_RESPONSE_PAUSED 0x10

#define HTTP_FLAG_SET(var, flag) var |= flag
#define HTTP_FLAG_CLEAR(var, flag) var &= ~flag
#define HTTP_FLAG_CHECK(var, flag) (var & flag)

typedef struct {
  char const * buf;
  int len;
} http_string_t;

http_string_t http_request_method(http_request_t* request);
http_string_t http_request_target(http_request_t* request);
http_string_t http_request_body(http_request_t* request);
http_string_t http_request_header(http_request_t* request, char const * key);
void http_request_set_flag(http_request_t* request, int flag, int value);

#endif
#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#define HTTP_RESPONSE_READY 0x4
#define HTTP_RESPONSE_KEEP_ALIVE 0x8
#define HTTP_RESPONSE_PAUSED 0x10

typedef struct http_header_s {
  char const * key;
  char const * value;
  struct http_header_s* next;
} http_header_t;

typedef struct {
  http_header_t* headers;
  char const * body;
  int content_length;
  int status;
} http_response_t;

void http_response_status(http_response_t* response, int status);
void http_response_header(http_response_t* response, char const * key, char const * value);
void http_response_body(http_response_t* response, char const * body, int length);
void http_respond(http_request_t* session, http_response_t* response);
void http_response_end(http_request_t* session);

#endif
#ifdef HTTPSERVER_IMPL


#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>

#ifndef HTTPSERVER_H
#include "http_parser.h"
#include "varray.h"
#include "http_server.h"
#include "http_response.h"
#include "http_request.h"
#define FIBER_IMPL
#include "fiber.h"
#endif

#define BUF_SIZE 1024

varray_defn(http_token_t)

fiber_defn(http_server_listen, http_server_t*);
fiber_defn(http_session, http_request_t*);

void accept_connections(http_server_t* arg) {
  while (errno != EWOULDBLOCK) {
    int sock = accept(arg->socket, (struct sockaddr *)&arg->addr, &arg->len);
    if (sock > 0) {
      http_request_t* session = malloc(sizeof(http_request_t));
      *session = (http_request_t) { .socket = sock, .server = arg };
      int flags = fcntl(sock, F_GETFL, 0);
      fcntl(sock, F_SETFL, flags | O_NONBLOCK);
      fiber_spawn(http_session, session, session->fiber);
    }
  }
  errno = 0;
}

void bind_localhost(int s, struct sockaddr_in* addr, int port) {
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = INADDR_ANY;
  addr->sin_port = htons(port);
  int rc = bind(s, (struct sockaddr *)addr, sizeof(struct sockaddr_in));;
  if (rc < 0) {
    exit(1);
  }
}

void http_listen(http_server_t* serv) {
  serv->socket = socket(AF_INET, SOCK_STREAM, 0);
  bind_localhost(serv->socket, &serv->addr, serv->port);
  serv->len = sizeof(serv->addr);
  listen(serv->socket, 128);
}

void read_client_socket(http_request_t* session) {
  if (!session->buf) {
    session->buf = malloc(BUF_SIZE);
    session->capacity = BUF_SIZE;
    varray_init(http_token_t, &session->tokens, 32);
  }
  int bytes;
  do {
    bytes = read(
      session->socket,
      session->buf + session->bytes,
      session->capacity - session->bytes
    );
    if (bytes > 0) {
      session->bytes += bytes;
      HTTP_FLAG_SET(session->flags, HTTP_READY);
    }
    if (session->bytes == session->capacity) {
      session->capacity *= 2;
      session->buf = realloc(session->buf, session->capacity);
    }
  } while (bytes > 0);
  if (bytes == 0) HTTP_FLAG_CLEAR(session->flags, HTTP_ACTIVE);
}

void write_client_socket(http_request_t* session) {
  session->bytes += write(session->socket, session->buf + session->bytes, session->capacity);
}

void nop(http_request_t* session) { }

void free_buffer(http_request_t* session) {
  free(session->buf);
  session->buf = NULL;
  free(session->tokens.buf);
}

void end_session(http_request_t* session) {
  close(session->socket);
  free(session);
}

void parse_tokens(http_request_t* session) {
  http_token_t token;
  do {
    token = http_parse(&session->parser, session->buf, session->bytes);
    if (token.type != HTTP_NONE) {
      session->token = token;
      varray_push(http_token_t, &session->tokens, token);
    }
  } while (token.type != HTTP_NONE);
  HTTP_FLAG_CLEAR(session->flags, HTTP_READY);
}

void init_session(http_request_t* session) {
  session->flags |= HTTP_KEEP_ALIVE;
  session->parser = (http_parser_t){ };
  session->bytes = 0;
  session->buf = NULL;
  session->token = (http_token_t){ .type = HTTP_NONE };
}

void handle_timeout(http_request_t* request) {
  if (fibererror) {
    HTTP_FLAG_CLEAR(request->flags, HTTP_ACTIVE);
    fibererror = 0;
  }
}

int parsing_headers(http_request_t* request) {
  return request->token.type != HTTP_BODY && HTTP_FLAG_CHECK(request->flags, HTTP_ACTIVE);
}

int reading_body(http_request_t* request) {
  int size = request->token.index + request->token.len;
  return request->bytes < size && HTTP_FLAG_CHECK(request->flags, HTTP_ACTIVE);
}


await_t call_http_server_listen(int* state, http_server_t* arg) {
switch (*state) {
case 0:
http_listen(arg);
while ( 1 ) {
*state = 10;
return  fiber_await(arg->socket, EV_READ, -1.f); 
case 10:
accept_connections(arg);
}
}
*state = -1;
await_t ret;
return ret;
}
await_t call_http_session(int* state, http_request_t* arg) {
switch (*state) {
case 0:
 HTTP_FLAG_SET(arg->flags, HTTP_ACTIVE); 
while ( HTTP_FLAG_CHECK(arg->flags, HTTP_ACTIVE) ) {
init_session(arg);
read_client_socket(arg);
parse_tokens(arg);
while (parsing_headers(arg)) {
if ( arg->bytes == 0 ) {
free_buffer(arg);
}
*state = 11;
return  fiber_await(arg->socket, EV_READ, arg->bytes == 0 ? 120.f : 20.f); 
case 11:
handle_timeout(arg);
read_client_socket(arg);
parse_tokens(arg);
}
if ( arg->token.len > 0 ) {
read_client_socket(arg);
while (reading_body(arg)) {
*state = 12;
return  fiber_await(arg->socket, EV_READ, 20.f); 
case 12:
handle_timeout(arg);
read_client_socket(arg);
}
}
if ( HTTP_FLAG_CHECK(arg->flags, HTTP_ACTIVE) ) {
 arg->server->request_handler(arg); 
if ( !HTTP_FLAG_CHECK(arg->flags, HTTP_RESPONSE_READY) ) {
 HTTP_FLAG_SET(arg->flags, HTTP_RESPONSE_PAUSED); 
*state = 13;
return  fiber_pause(); 
case 13:
nop(arg);
}
write_client_socket(arg);
while ( arg->bytes != arg->capacity ) {
*state = 14;
return  fiber_await(arg->socket, EV_WRITE, 20.f); 
case 14:
handle_timeout(arg);
write_client_socket(arg);
}
}
free_buffer(arg);
}
end_session(arg);
}
*state = -1;
await_t ret;
return ret;
}


void generate_date_time(char** datetime) {
  time_t rawtime;
  struct tm * timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  *datetime = asctime(timeinfo);
}

void date_generator(EV_P_ ev_timer *w, int revents) {
  generate_date_time((char**)w->data);
  ev_timer_again(fiber_scheduler, w);
}

void http_server_init(http_server_t* serv, int port, void (*handler)(http_request_t*)) {
  fiber_scheduler_init();
  serv->port = port;
  ev_init(&serv->timer, date_generator);
  serv->timer.data = (void*)&serv->date;
  serv->timer.repeat = 1.f;
  ev_timer_again(fiber_scheduler, &serv->timer);
  generate_date_time(&serv->date);
  serv->request_handler = handler;
}

int http_server_listen(http_server_t* serv) {
  fiber_spawn(http_server_listen, serv);
  fiber_scheduler_run();
  return 0;
}

struct ev_loop* http_server_loop() {
  return fiber_scheduler;
}


#include <string.h>
#ifndef HTTPSERVER_H
#include "http_request.h"
#endif


http_string_t http_get_token_string(http_request_t* request, int token_type) {
  for (int i = 0; i < request->tokens.size; i++) {
    http_token_t token = request->tokens.buf[i];
    if (token.type == token_type) {
      return (http_string_t) {
        .buf = &request->buf[token.index],
        .len = token.len
      };
    }
  }
  return (http_string_t) { };
}

int case_insensitive_cmp(char const * a, char const * b, int len) {
  for (int i = 0; i < len; i++) {
    char c1 = a[i] >= 'A' && a[i] <= 'Z' ? a[i] + 32 : a[i];
    char c2 = b[i] >= 'A' && b[i] <= 'Z' ? b[i] + 32 : b[i];
    if (c1 != c2) return 0;
  }
  return 1;
}

http_string_t http_request_method(http_request_t* request) {
  return http_get_token_string(request, HTTP_METHOD);
}

http_string_t http_request_target(http_request_t* request) {
  return http_get_token_string(request, HTTP_TARGET);
}

http_string_t http_request_body(http_request_t* request) {
  return http_get_token_string(request, HTTP_BODY);
}

http_string_t http_request_header(http_request_t* request, char const * key) {
  int len = strlen(key);
  for (int i = 0; i < request->tokens.size; i++) {
    http_token_t token = request->tokens.buf[i];
    if (token.type == HTTP_HEADER_KEY && token.len == len) {
      if (case_insensitive_cmp(&request->buf[token.index], key, len)) {
        token = request->tokens.buf[i + 1];
        return (http_string_t) {
          .buf = &request->buf[token.index],
          .len = token.len
        };
      }
    }
  }
  return (http_string_t) { };
}

void http_request_set_flag(http_request_t* request, int flag, int value) {
  if (value) {
    HTTP_FLAG_SET(request->flags, flag);
  } else {
    HTTP_FLAG_CLEAR(request->flags, flag);
  }
}
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef HTTPSERVER_H
#include "http_server.h"
#include "fiber.h"
#include "http_request.h"
#include "http_response.h"
#endif

#define RESPONSE_BUF_SIZE 512

char const * status_text[] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  
  //100s
  "Continue", "Switching Protocols", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //200s
  "OK", "Created", "Accepted", "Non-Authoratative Information", "No Content",
  "Reset Content", "Partial Content", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //300s
  "Multiple Choices", "Moved Permanently", "Found", "See Other", "Not Modified",
  "Use Proxy", "", "Temporary Redirect", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //400s
  "Bad Request", "Unauthorized", "Payment Required", "Forbidden", "Not Found",
  "Method Not Allowed", "Not Acceptable", "Proxy Authentication Required",
  "Request Timeout", "Conflict",
  "Gone", "Length Required", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //500s
  "Internal Server Error", "Not Implemented", "Bad Gateway", "Service Unavailable",
  "Gateway Timeout", "", "", "", "", ""
};

void http_response_header(http_response_t* response, char const * key, char const * value) {
  http_header_t* header = malloc(sizeof(http_header_t));
  header->key = key;
  header->value = value;
  http_header_t* prev = response->headers;
  header->next = prev;
  response->headers = header;
}

void http_response_status(http_response_t* response, int status) {
  response->status = status;
}

void http_response_body(http_response_t* response, char const * body, int length) {
  response->body = body;
  response->content_length = length;
}

#define buffer_bookkeeping(printf) \
  printf \
  if (bytes + size > capacity) { \
    while (bytes + size > capacity) capacity *= 2; \
    buf = realloc(buf, capacity); \
    printf \
    remaining = capacity - size; \
  } \
  size += bytes; \
  remaining -= bytes;

void http_respond(http_request_t* session, http_response_t* response) {
  char* buf = malloc(RESPONSE_BUF_SIZE);
  int capacity = RESPONSE_BUF_SIZE;
  int remaining = RESPONSE_BUF_SIZE;
  int size = 0;
  if (HTTP_FLAG_CHECK(session->flags, HTTP_KEEP_ALIVE)) {
    http_response_header(response, "Connection", "keep-alive");
  } else {
    HTTP_FLAG_CLEAR(session->flags, HTTP_ACTIVE);
    http_response_header(response, "Connection", "close");
  }

  int bytes = snprintf(
    buf, remaining, "HTTP/1.1 %d %s\r\nDate: %.24s\r\n",
    response->status, status_text[response->status], session->server->date
  );
  size += bytes;
  remaining -= bytes;
  http_header_t* header = response->headers;
  while (header) {
    buffer_bookkeeping(
      bytes = snprintf( 
        buf + size, remaining, "%s: %s\r\n", 
        header->key, header->value 
      );
    )
    header = header->next;
  }
  if (response->body) {
    buffer_bookkeeping(
      bytes = snprintf(
        buf + size, remaining, "Content-Length: %d\r\n",
        response->content_length
      );
    )
  }
  buffer_bookkeeping(bytes = snprintf(buf + size, remaining, "\r\n");)
  if (response->body) {
    buffer_bookkeeping(
      bytes = snprintf(
        buf + size, remaining, "%.*s",
        response->content_length, response->body
      );
    )
  }
  header = response->headers;
  while (header) {
    http_header_t* tmp = header;
    header = tmp->next;
    free(tmp);
  }
  free(session->buf);
  session->buf = buf;
  session->bytes = 0;
  session->capacity = size;
  HTTP_FLAG_SET(session->flags, HTTP_RESPONSE_READY);
  if (HTTP_FLAG_CHECK(session->flags, HTTP_RESPONSE_PAUSED)) {
    HTTP_FLAG_CLEAR(session->flags, HTTP_RESPONSE_PAUSED);
    fiber_resume(http_session, session->fiber);
  }
}

#ifndef HTTPSERVER_H
#include "http_parser.h"
#endif

#define HTTP_LWS 2
#define HTTP_CR 3
#define HTTP_CRLF 4

#define HTTP_HEADER_END 5

#define CONTENT_LENGTH_LOW "content-length"
#define CONTENT_LENGTH_UP "CONTENT-LENGTH"

http_token_t http_parse(http_parser_t* parser, char* input, int n) {
  for (int i = parser->start; i < n; ++i, parser->start = i + 1, parser->len++) {
    char c = input[i];
    switch (parser->state) {
      case HTTP_METHOD:
        if (c == ' ') {
          http_token_t token = {
            .index = parser->token_start_index,
            .type = parser->state,
            .len = parser->len
          };
          parser->state = HTTP_TARGET;
          parser->len = 0;
          parser->token_start_index = i + 1;
          return token;
        }
        break;
      case HTTP_TARGET:
        if (c == ' ') {
          http_token_t token = {
            .index = parser->token_start_index,
            .type = parser->state,
            .len = parser->len
          };
          parser->state = HTTP_VERSION;
          parser->token_start_index = i + 1;
          parser->len = 0;
          return token;
        }
        break;
      case HTTP_VERSION:
        if (c == '\r') {
          parser->sub_state = HTTP_CR;
          return (http_token_t) {
            .index = parser->token_start_index,
            .type = HTTP_VERSION,
            .len = parser->len 
          };
        } else if (parser->sub_state == HTTP_CR && c == '\n') {
          parser->sub_state = 0;
          parser->len = 0;
          parser->token_start_index = i + 1;
          parser->state = HTTP_HEADER_KEY;
        }
        break;
      case HTTP_HEADER_KEY:
        if (c == ':') {
          parser->state = HTTP_HEADER_VALUE;
          parser->sub_state = HTTP_LWS;
          if (parser->len == parser->content_length_i + 1) parser->in_content_length = 1;
          parser->content_length_i = 0;
          return (http_token_t) {
            .index = parser->token_start_index,
            .type = HTTP_HEADER_KEY,
            .len = parser->len - 1
          };
        } else if (
          (c == CONTENT_LENGTH_UP[parser->content_length_i] ||
            c == CONTENT_LENGTH_LOW[parser->content_length_i]) &&
          parser->content_length_i < sizeof(CONTENT_LENGTH_LOW) - 1
        ) {
          parser->content_length_i++;
        }
        break;
      case HTTP_HEADER_VALUE:
        if (parser->sub_state == HTTP_LWS && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
          continue;
        } else if (parser->sub_state == HTTP_LWS) {
          parser->sub_state = 0;
          parser->len = 0;
          parser->token_start_index = i;
          if (parser->in_content_length) {
            parser->content_length *= 10;
            parser->content_length += c - '0';
          }
        } else if (parser->sub_state != HTTP_LWS && c == '\r') {
          parser->sub_state = HTTP_CR;
          parser->state = HTTP_HEADER_END;
          parser->in_content_length = 0;
          return (http_token_t) {
            .index = parser->token_start_index,
            .type = HTTP_HEADER_VALUE,
            .len = parser->len
          };
        } else if (parser->in_content_length) {
          parser->content_length *= 10;
          parser->content_length += c - '0';
        }
        break;
      case HTTP_HEADER_END:
        if (parser->sub_state == 0 && c == '\r') {
          parser->sub_state = HTTP_CR;
        } else if (parser->sub_state == HTTP_CR && c == '\n') {
          parser->sub_state = HTTP_CRLF;
        } else if (parser->sub_state == HTTP_CRLF && c == '\r') {
          parser->sub_state = 0;
          return (http_token_t) {
            .index = i + 2,
            .type = HTTP_BODY,
            .len = parser->content_length
          };
        } else if (parser->sub_state == HTTP_CRLF && c != '\r') {
          parser->sub_state = 0;
          parser->len = 0;
          parser->token_start_index = i;
          i--;
          parser->state = HTTP_HEADER_KEY;
        }
        break;

    }
  }
  return (http_token_t) { .index = 0, .type = HTTP_NONE, .len = 0 };
}
#endif
#endif
