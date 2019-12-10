/******************************************************************************
 *
 * MIT License
 * 
 * Copyright (c) 2019 Jeremy Williams
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *****************************************************************************/

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

// String type used to read the request details. The char pointer is NOT null
// terminated.
struct http_string_s {
  char const * buf;
  int len;
};

struct http_server_s;
struct http_request_s;
struct http_response_s;

// Returns the libev event loop that the server is running on. Can be used to
// perform event driven io during a request. See libev man for more information
struct ev_loop* http_server_loop(struct http_server_s* server);

// Allocates and initializes the http server. Takes a port and a function
// pointer that is called to process requests.
struct http_server_s* http_server_init(int port, void (*handler)(struct http_request_s*));

// Starts the event loop and the server listening. During normal operation this
// function will not return. Return value is the error code if the server fails
// to start.
int http_server_listen(struct http_server_s* server);

// Returns the request method as it was read from the HTTP request line.
struct http_string_s http_request_method(struct http_request_s* request);

// Returns the full request target as it was read from the HTTP request line.
struct http_string_s http_request_target(struct http_request_s* request);

// Returns the request body. If no request body was sent buf and len of the
// string will be set to 0.
struct http_string_s http_request_body(struct http_request_s* request);

// Returns the request header value for the given header key. The key is case
// insensitive.
struct http_string_s http_request_header(struct http_request_s* request, char const * key);

// Procedure used to iterate over all the request headers. iter should be
// initialized to zero before calling. Each call will set key and val to the
// key and value of the next header. Returns 0 when there are no more headers.
int http_request_iterate_headers(
  struct http_request_s* request,
  struct http_string_s* key,
  struct http_string_s* val,
  int* iter
);

#define HTTP_KEEP_ALIVE 1
#define HTTP_CLOSE 0

// By default the server will inspect the Connection header and the HTTP
// version to determine whether the connection should be kept alive or not.
// Use this function to override that behaviour to force the connection to
// keep-alive or close by passing in the HTTP_KEEP_ALIVE or HTTP_CLOSE
// directives respectively. This may provide a minor performance improvement
// in cases where you control client and server and want to always close or
// keep the connection alive.
void http_request_connection(struct http_request_s* request, int directive);

// Allocates an http response. This memory will be freed when http_respond is
// called.
struct http_response_s* http_response_init();

// Set the response status. Accepts values between 100 and 599 inclusive. Any
// other value will map to 500.
void http_response_status(struct http_response_s* response, int status);

// Set a response header. Takes two null terminated strings.
void http_response_header(struct http_response_s* response, char const * key, char const * value);

// Set the response body. The caller is responsible for freeing any memory that
// may have been allocated for the body. It is safe to free this memory AFTER
// http_respond has been called.
void http_response_body(struct http_response_s* response, char const * body, int length);

// Starts writing the response to the client. Any memory allocated for the
// response body or response headers is safe to free after this call.
void http_respond(struct http_request_s* request, struct http_response_s* response);

// Minimal example usage.
#ifdef HTTPSERVER_EXAMPLE

#define RESPONSE "Hello, World!"

void handle_request(struct http_request_s* request) {
  struct http_response_s* response = http_response_init();
  http_response_status(response, 200);
  http_response_header(response, "Content-Type", "text/plain");
  http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  http_respond(request, response);
}

int main() {
  struct http_server_s* server = http_server_init(8080, handle_request);
  http_server_listen(server);
}

#endif

#endif

#ifdef HTTPSERVER_IMPL
#ifndef HTTPSERVER_IMPL_ONCE
#define HTTPSERVER_IMPL_ONCE

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <ev.h>

/******************************************************************************
 *
 * HTTP Parser
 *
 *****************************************************************************/

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
          parser->state = HTTP_BODY;
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

/******************************************************************************
 *
 * HTTP Server
 *
 *****************************************************************************/

#define HTTP_READY 0x2
#define HTTP_RESPONSE_READY 0x4
#define HTTP_AUTOMATIC 0x8
#define HTTP_RESPONSE_PAUSED 0x10

#define HTTP_FLAG_SET(var, flag) var |= flag
#define HTTP_FLAG_CLEAR(var, flag) var &= ~flag
#define HTTP_FLAG_CHECK(var, flag) (var & flag)

typedef struct {
  http_token_t* buf;
  int capacity;
  int size;
} http_token_dyn_t;

typedef struct http_request_s {
  http_parser_t parser;
  ev_timer timer;
  ev_io io;
  int state;
  int socket;
  char* buf;
  int bytes;
  int capacity;
  struct http_server_s* server;
  http_token_t token;
  http_token_dyn_t tokens;
  char flags;
} http_request_t;

typedef struct http_server_s {
  ev_io io;
  struct ev_loop* loop;
  int socket;
  int port;
  socklen_t len;
  void (*request_handler)(http_request_t*);
  struct sockaddr_in addr;
  ev_timer timer;
  char* date;
} http_server_t;

void http_token_dyn_push(http_token_dyn_t* dyn, http_token_t a) {
  if (dyn->size == dyn->capacity) {
    dyn->capacity *= 2;
    dyn->buf = realloc(dyn->buf, dyn->capacity * sizeof(http_token_t));
  }
  dyn->buf[dyn->size] = a;
  dyn->size++;
}

void http_token_dyn_init(http_token_dyn_t* dyn, int capacity) {
  dyn->buf = malloc(sizeof(http_token_t) * capacity);
  dyn->size = 0;
  dyn->capacity = capacity;
}

#define BUF_SIZE 1024

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
  int flag = 1;
  setsockopt(serv->socket, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag));
  bind_localhost(serv->socket, &serv->addr, serv->port);
  serv->len = sizeof(serv->addr);
  listen(serv->socket, 128);
}

int read_client_socket(http_request_t* session) {
  if (!session->buf) {
    session->buf = malloc(BUF_SIZE);
    session->capacity = BUF_SIZE;
    http_token_dyn_init(&session->tokens, 32);
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
  return bytes == 0 ? 0 : 1;
}

int write_client_socket(http_request_t* session) {
  int bytes = write(
    session->socket,
    session->buf + session->bytes,
    session->capacity - session->bytes
  );
  if (bytes > 0) session->bytes += bytes;
  return errno == EPIPE ? 0 : 1;
}

void free_buffer(http_request_t* session) {
  free(session->buf);
  session->buf = NULL;
  free(session->tokens.buf);
}

void parse_tokens(http_request_t* session) {
  http_token_t token;
  do {
    token = http_parse(&session->parser, session->buf, session->bytes);
    if (token.type != HTTP_NONE) {
      session->token = token;
      http_token_dyn_push(&session->tokens, token);
    }
  } while (token.type != HTTP_NONE);
  HTTP_FLAG_CLEAR(session->flags, HTTP_READY);
}

void init_session(http_request_t* session) {
  session->flags |= HTTP_AUTOMATIC;
  session->parser = (http_parser_t){ };
  session->bytes = 0;
  session->buf = NULL;
  session->token = (http_token_t){ .type = HTTP_NONE };
}

int parsing_headers(http_request_t* request) {
  return request->token.type != HTTP_BODY;
}

int reading_body(http_request_t* request) {
  if (request->token.type != HTTP_BODY || request->token.len == 0) return 0;
  int size = request->token.index + request->token.len;
  return request->bytes < size;
}

void end_session(http_request_t* session) {
  ev_io_stop(session->server->loop, &session->io);
  ev_timer_stop(session->server->loop, &session->timer);
  close(session->socket);
  free_buffer(session);
  free(session);
}

void http_session_timeout_cb(EV_P_ ev_timer *w, int revents) {
  end_session((http_request_t*)w->data);
}

#define HTTP_SESSION_INIT 0
#define HTTP_SESSION_READ_HEADERS 1
#define HTTP_SESSION_READ_BODY 2
#define HTTP_SESSION_WRITE 3

void reset_timeout(http_request_t* request, float time) {
  request->timer.repeat = time;
  ev_timer_again(request->server->loop, &request->timer);
}

void write_response(http_request_t* request) {
  if (!write_client_socket(request)) { return end_session(request); }
  if (request->bytes != request->capacity) {
    ev_io_stop(request->server->loop, &request->io);
    ev_io_set(&request->io, request->socket, EV_WRITE);
    ev_io_start(request->server->loop, &request->io);
    request->state = HTTP_SESSION_WRITE;
    reset_timeout(request, 20.f);
  } else if (HTTP_FLAG_CHECK(request->flags, HTTP_KEEP_ALIVE)) {
    request->state = HTTP_SESSION_INIT;
    free_buffer(request);
    reset_timeout(request, 120.f);
  } else {
    return end_session(request);
  }
}

void exec_response_handler(http_request_t* request) {
  request->server->request_handler(request);
  if (HTTP_FLAG_CHECK(request->flags, HTTP_RESPONSE_READY)) {
    write_response(request);
  } else {
    request->state = HTTP_SESSION_WRITE;
    HTTP_FLAG_SET(request->flags, HTTP_RESPONSE_PAUSED);
  }
}

void http_session(http_request_t* request) {
  switch (request->state) {
    case HTTP_SESSION_INIT:
      init_session(request);
      request->state = HTTP_SESSION_READ_HEADERS;
      // fallthrough
    case HTTP_SESSION_READ_HEADERS:
      if (!read_client_socket(request)) { return end_session(request); }
      parse_tokens(request);
      if (reading_body(request)) {
        request->state = HTTP_SESSION_READ_BODY;
      } else if (!parsing_headers(request)) {
        return exec_response_handler(request);
      }
      reset_timeout(request, 20.f);
      break;
    case HTTP_SESSION_READ_BODY:
      if (!read_client_socket(request)) { return end_session(request); }
      if (!reading_body(request)) { return exec_response_handler(request); }
      reset_timeout(request, 20.f);
      break;
    case HTTP_SESSION_WRITE:
      write_response(request);
      break;
  }
}

void http_session_io_cb(EV_P_ ev_io *w, int revents) {
  http_session((http_request_t*)w->data);
}

void accept_connections(http_server_t* server) {
  while (errno != EWOULDBLOCK) {
    int sock = accept(server->socket, (struct sockaddr *)&server->addr, &server->len);
    if (sock > 0) {
      http_request_t* session = malloc(sizeof(http_request_t));
      *session = (http_request_t) { .socket = sock, .server = server };
      int flags = fcntl(sock, F_GETFL, 0);
      fcntl(sock, F_SETFL, flags | O_NONBLOCK);
      ev_init(&session->timer, http_session_timeout_cb);
      session->timer.data = session;
      ev_io_init(&session->io, http_session_io_cb, sock, EV_READ);
      session->io.data = session;
      ev_io_start(server->loop, &session->io);
      http_session(session);
    }
  }
  errno = 0;
}

void http_server_listen_cb(EV_P_ ev_io *w, int revents) {
  accept_connections((http_server_t*)w->data);
}

void generate_date_time(char** datetime) {
  time_t rawtime;
  struct tm * timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  *datetime = asctime(timeinfo);
}

void date_generator(EV_P_ ev_timer *w, int revents) {
  http_server_t* server = (http_server_t*)w->data;
  generate_date_time(&server->date);
  ev_timer_again(server->loop, w);
}

http_server_t* http_server_init(int port, void (*handler)(http_request_t*)) {
  http_server_t* serv = malloc(sizeof(http_server_t));
  serv->port = port;
  if (ev_supported_backends() & EVBACKEND_EPOLL) {
    serv->loop = ev_default_loop(EVBACKEND_EPOLL);
  } else if (ev_supported_backends() & EVBACKEND_KQUEUE) {
    serv->loop = ev_default_loop(EVBACKEND_KQUEUE);
  } else {
    serv->loop = EV_DEFAULT;
  }
  ev_init(&serv->timer, date_generator);
  serv->timer.data = serv;
  serv->timer.repeat = 1.f;
  ev_timer_again(serv->loop, &serv->timer);
  generate_date_time(&serv->date);
  serv->request_handler = handler;
  return serv;
}

int http_server_listen(http_server_t* serv) {
  signal(SIGPIPE, SIG_IGN);
  http_listen(serv);
  ev_io_init(&serv->io, http_server_listen_cb, serv->socket, EV_READ);
  serv->io.data = serv;
  ev_io_start(serv->loop, &serv->io);
  ev_run(serv->loop, 0);
  return 0;
}

struct ev_loop* http_server_loop(http_server_t* server) {
  return server->loop;
}

/******************************************************************************
 *
 * HTTP Request
 *
 *****************************************************************************/

typedef struct http_string_s http_string_t;

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

int assign_iteration_headers(
  http_request_t* request,
  http_string_t* key,
  http_string_t* val,
  int* iter
) {
  http_token_t token = request->tokens.buf[*iter];
  *key = (http_string_t) {
    .buf = &request->buf[token.index],
    .len = token.len
  };
  (*iter)++;
  token = request->tokens.buf[*iter];
  *val = (http_string_t) {
    .buf = &request->buf[token.index],
    .len = token.len
  };
  if (request->tokens.buf[*iter + 1].type == HTTP_BODY) return 0;
  return 1;
}

int http_request_iterate_headers(
  http_request_t* request,
  http_string_t* key,
  http_string_t* val,
  int* iter
) {
  if (*iter == 0) {
    for ( ; *iter < request->tokens.size; (*iter)++) {
      http_token_t token = request->tokens.buf[*iter];
      if (token.type == HTTP_HEADER_KEY) {
        return assign_iteration_headers(request, key, val, iter);
      }
    }
    return 0;
  } else {
    (*iter)++;
    return assign_iteration_headers(request, key, val, iter);
  }
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

#define HTTP_1_0 0
#define HTTP_1_1 1

void auto_detect_keep_alive(http_request_t* request) {
  http_string_t str = http_get_token_string(request, HTTP_VERSION);
  int version = str.buf[str.len - 1] == '1';
  str = http_request_header(request, "Connection");
  if (
    (str.len == 5 && case_insensitive_cmp(str.buf, "close", 5)) ||
    (str.len == 0 && version == HTTP_1_0)
  ) {
    HTTP_FLAG_CLEAR(request->flags, HTTP_KEEP_ALIVE);
  } else {
    HTTP_FLAG_SET(request->flags, HTTP_KEEP_ALIVE);
  }
}

void http_request_connection(http_request_t* request, int directive) {
  if (directive == HTTP_KEEP_ALIVE) {
    HTTP_FLAG_CLEAR(request->flags, HTTP_AUTOMATIC);
    HTTP_FLAG_SET(request->flags, HTTP_KEEP_ALIVE);
  } else if (directive == HTTP_CLOSE) {
    HTTP_FLAG_CLEAR(request->flags, HTTP_AUTOMATIC);
    HTTP_FLAG_CLEAR(request->flags, HTTP_KEEP_ALIVE);
  }
}

/******************************************************************************
 *
 * HTTP Response
 *
 *****************************************************************************/

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

  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", ""
};

http_response_t* http_response_init() {
  http_response_t* response = malloc(sizeof(http_response_t));
  *response = (http_response_t){ .status = 200 };
  return response;
}

void http_response_header(http_response_t* response, char const * key, char const * value) {
  http_header_t* header = malloc(sizeof(http_header_t));
  header->key = key;
  header->value = value;
  http_header_t* prev = response->headers;
  header->next = prev;
  response->headers = header;
}

void http_response_status(http_response_t* response, int status) {
  response->status = status > 599 || status < 100 ? 500 : status;
}

void http_response_body(http_response_t* response, char const * body, int length) {
  response->body = body;
  response->content_length = length;
}

typedef struct {
  char* buf;
  int capacity;
  int size;
} grwprintf_t;

void grwprintf_init(grwprintf_t* ctx, int capacity) {
  ctx->size = 0;
  ctx->buf = malloc(capacity);
  ctx->capacity = capacity;
}

void grwmemcpy(grwprintf_t* ctx, char const * src, int size) {
  if (ctx->size + size > ctx->capacity) {
    ctx->capacity = ctx->size + size;
    ctx->buf = realloc(ctx->buf, ctx->capacity);
  }
  memcpy(ctx->buf + ctx->size, src, size);
  ctx->size += size;
}

void grwprintf(grwprintf_t* ctx, char const * fmt, ...) {
  va_list args;
  va_start(args, fmt);

  int bytes = vsnprintf(ctx->buf + ctx->size, ctx->capacity - ctx->size, fmt, args);
  if (bytes + ctx->size > ctx->capacity) {
    while (bytes + ctx->size > ctx->capacity) ctx->capacity *= 2;
    ctx->buf = realloc(ctx->buf, ctx->capacity);
    bytes += vsnprintf(ctx->buf + ctx->size, ctx->capacity - ctx->size, fmt, args);
  }
  ctx->size += bytes;
 
  va_end(args);
}

void http_respond(http_request_t* session, http_response_t* response) {
  if (HTTP_FLAG_CHECK(session->flags, HTTP_AUTOMATIC)) {
    auto_detect_keep_alive(session);
  }
  if (HTTP_FLAG_CHECK(session->flags, HTTP_KEEP_ALIVE)) {
    http_response_header(response, "Connection", "keep-alive");
  } else {
    http_response_header(response, "Connection", "close");
  }

  grwprintf_t printctx;
  grwprintf_init(&printctx, RESPONSE_BUF_SIZE);
  grwprintf(
    &printctx, "HTTP/1.1 %d %s\r\nDate: %.24s\r\n",
    response->status, status_text[response->status], session->server->date
  );
  http_header_t* header = response->headers;
  while (header) {
    grwprintf(&printctx, "%s: %s\r\n", header->key, header->value);
    header = header->next;
  }
  if (response->body) {
    grwprintf(&printctx, "Content-Length: %d\r\n", response->content_length);
  }
  grwprintf(&printctx, "\r\n");
  if (response->body) {
    grwmemcpy(&printctx, response->body, response->content_length);
  }
  header = response->headers;
  while (header) {
    http_header_t* tmp = header;
    header = tmp->next;
    free(tmp);
  }
  free(response);
  free(session->buf);
  session->buf = printctx.buf;
  session->bytes = 0;
  session->capacity = printctx.size;
  HTTP_FLAG_SET(session->flags, HTTP_RESPONSE_READY);
  if (HTTP_FLAG_CHECK(session->flags, HTTP_RESPONSE_PAUSED)) {
    HTTP_FLAG_CLEAR(session->flags, HTTP_RESPONSE_PAUSED);
    http_session(session);
  }
}

#endif
#endif
