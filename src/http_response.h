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
  char* body;
  int content_length;
  int status;
} http_response_t;

void http_response_status(http_response_t* response, int status);
void http_response_header(http_response_t* response, char const * key, char const * value);
void http_response_body(http_response_t* response, char* body, int length);
void http_respond(http_request_t* session, http_response_t* response);
void http_response_end(http_request_t* session);

#endif
