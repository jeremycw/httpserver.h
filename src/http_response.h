#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H


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

#endif
