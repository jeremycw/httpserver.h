#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http_server.h"

typedef struct {
  char const * buf;
  int len;
} http_string_t;

http_string_t http_request_method(http_request_t* request);
http_string_t http_request_target(http_request_t* request);
http_string_t http_request_body(http_request_t* request);
http_string_t http_request_header(http_request_t* request, char const * key);

#endif
