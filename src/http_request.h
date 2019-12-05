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
