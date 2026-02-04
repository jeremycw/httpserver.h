#ifndef HS_REQUEST_UTIL_H
#define HS_REQUEST_UTIL_H

#ifndef HTTPSERVER_IMPL
#include "common.h"
#endif

// http version indicators
#define HTTP_1_0 0
#define HTTP_1_1 1

struct http_string_s {
  char const *buf;
  int len;
};

typedef struct http_string_s http_string_t;

http_string_t hs_get_token_string(http_request_t *request,
                                  enum hsh_token_e token_type);
http_string_t hs_request_header(http_request_t *request, char const *key);
void hs_request_detect_keep_alive_flag(http_request_t *request);
int hs_request_iterate_headers(http_request_t *request, http_string_t *key,
                               http_string_t *val, int *iter);
void hs_request_set_keep_alive_flag(http_request_t *request, int directive);
http_string_t hs_request_chunk(struct http_request_s *request);

#endif
