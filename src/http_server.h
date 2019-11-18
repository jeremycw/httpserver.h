#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <netdb.h>
#include "fiber.h"
#include "varray.h"
#include "http_parser.h"

struct http_response {
  char* buf;
  int len;
  int flags;
};

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
  struct http_response response;
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
