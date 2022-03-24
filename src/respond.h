#ifndef HS_RESPOND_H
#define HS_RESPOND_H

#define HTTP_RESPONSE_BUF_SIZE 1024

struct http_request_s;

typedef void (*hs_req_fn_t)(struct http_request_s *);

typedef struct http_header_s {
  char const *key;
  char const *value;
  struct http_header_s *next;
} http_header_t;

typedef struct http_response_s {
  http_header_t *headers;
  char const *body;
  int content_length;
  int status;
} http_response_t;

void hs_response_header(http_response_t *response, char const *key,
                        char const *value);
void hs_respond(struct http_request_s *request, http_response_t *response,
                hs_req_fn_t http_write);
void hs_respond_chunk(struct http_request_s *request, http_response_t *response,
                      hs_req_fn_t cb, hs_req_fn_t http_write);
void hs_respond_chunk_end(struct http_request_s *request,
                          http_response_t *response, hs_req_fn_t http_write);
void hs_response_status(http_response_t *response, int status);
void hs_response_body(http_response_t *response, char const *body, int length);
http_response_t *hs_response_init();
void hs_respond_error(struct http_request_s *request, int code,
                      char const *message, hs_req_fn_t http_write);

#endif
