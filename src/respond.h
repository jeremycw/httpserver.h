#ifndef HS_RESPOND_H
#define HS_RESPOND_H

#define HTTP_RESPONSE_BUF_SIZE 1024

struct http_request_s;

typedef void (*hs_req_fn_t)(struct http_request_s *);

// Represents a single header of an HTTP response.
typedef struct http_header_s {
  // The key of the header eg: Content-Type
  char const *key;
  // The value of the header eg: application/json
  char const *value;
  // Pointer to the next header in the linked list.
  struct http_header_s *next;
} http_header_t;

// Represents the response of an HTTP request before it is serialized on the
// wire.
typedef struct http_response_s {
  // Head of the linked list of response headers
  http_header_t *headers;
  // The complete body of the response or the chunk if generating a chunked
  // response.
  char const *body;
  // The length of the body or chunk.
  int content_length;
  // The HTTP status code for the response.
  int status;
} http_response_t;

http_response_t *hs_response_init();
void hs_response_set_header(http_response_t *response, char const *key,
                            char const *value);
void hs_response_set_status(http_response_t *response, int status);
void hs_response_set_body(http_response_t *response, char const *body,
                          int length);
void hs_request_respond(struct http_request_s *request,
                        http_response_t *response, hs_req_fn_t http_write);
void hs_request_respond_chunk(struct http_request_s *request,
                              http_response_t *response, hs_req_fn_t cb,
                              hs_req_fn_t http_write);
void hs_request_respond_chunk_end(struct http_request_s *request,
                                  http_response_t *response,
                                  hs_req_fn_t http_write);
void hs_request_respond_error(struct http_request_s *request, int code,
                              char const *message, hs_req_fn_t http_write);

#endif
