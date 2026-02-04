#include <stdlib.h>
#include <string.h>

#ifndef HTTPSERVER_IMPL
#include "common.h"
#include "request_util.h"
#endif

int _hs_case_insensitive_cmp(char const *a, char const *b, int len) {
  for (int i = 0; i < len; i++) {
    char c1 = a[i] >= 'A' && a[i] <= 'Z' ? a[i] + 32 : a[i];
    char c2 = b[i] >= 'A' && b[i] <= 'Z' ? b[i] + 32 : b[i];
    if (c1 != c2)
      return 0;
  }
  return 1;
}

http_string_t hs_get_token_string(http_request_t *request,
                                  enum hsh_token_e token_type) {
  http_string_t str = {0, 0};
  if (request->tokens.buf == NULL)
    return str;
  for (int i = 0; i < request->tokens.size; i++) {
    struct hsh_token_s token = request->tokens.buf[i];
    if (token.type == token_type) {
      str.buf = &request->buffer.buf[token.index];
      str.len = token.len;
      return str;
    }
  }
  return str;
}

http_string_t hs_request_header(http_request_t *request, char const *key) {
  int len = strlen(key);
  for (int i = 0; i < request->tokens.size; i++) {
    struct hsh_token_s token = request->tokens.buf[i];
    if (token.type == HSH_TOK_HEADER_KEY && token.len == len) {
      if (_hs_case_insensitive_cmp(&request->buffer.buf[token.index], key,
                                   len)) {
        token = request->tokens.buf[i + 1];
        return (http_string_t){.buf = &request->buffer.buf[token.index],
                               .len = token.len};
      }
    }
  }
  return (http_string_t){};
}

void hs_request_detect_keep_alive_flag(http_request_t *request) {
  http_string_t str = hs_get_token_string(request, HSH_TOK_VERSION);
  if (str.buf == NULL)
    return;
  int version = str.buf[str.len - 1] == '1';
  str = hs_request_header(request, "Connection");
  if ((str.len == 5 && _hs_case_insensitive_cmp(str.buf, "close", 5)) ||
      (str.len == 0 && version == HTTP_1_0)) {
    HTTP_FLAG_CLEAR(request->flags, HTTP_KEEP_ALIVE);
  } else {
    HTTP_FLAG_SET(request->flags, HTTP_KEEP_ALIVE);
  }
}

int _hs_get_header_key_val(http_request_t *request, http_string_t *key,
                           http_string_t *val, int iter) {
  struct hsh_token_s token = request->tokens.buf[iter];
  if (request->tokens.buf[iter].type == HSH_TOK_HEADERS_DONE)
    return 0;
  *key = (http_string_t){.buf = &request->buffer.buf[token.index],
                         .len = token.len};
  token = request->tokens.buf[iter + 1];
  *val = (http_string_t){.buf = &request->buffer.buf[token.index],
                         .len = token.len};
  return 1;
}

int hs_request_iterate_headers(http_request_t *request, http_string_t *key,
                               http_string_t *val, int *iter) {
  if (*iter == 0) {
    for (; *iter < request->tokens.size; (*iter)++) {
      struct hsh_token_s token = request->tokens.buf[*iter];
      if (token.type == HSH_TOK_HEADER_KEY) {
        int more = _hs_get_header_key_val(request, key, val, *iter);
        (*iter)++;
        return more;
      }
    }
    return 0;
  } else {
    (*iter)++;
    int more = _hs_get_header_key_val(request, key, val, *iter);
    (*iter)++;
    return more;
  }
}

void hs_request_set_keep_alive_flag(http_request_t *request, int directive) {
  if (directive == HTTP_KEEP_ALIVE) {
    HTTP_FLAG_CLEAR(request->flags, HTTP_AUTOMATIC);
    HTTP_FLAG_SET(request->flags, HTTP_KEEP_ALIVE);
  } else if (directive == HTTP_CLOSE) {
    HTTP_FLAG_CLEAR(request->flags, HTTP_AUTOMATIC);
    HTTP_FLAG_CLEAR(request->flags, HTTP_KEEP_ALIVE);
  }
}

http_string_t hs_request_chunk(struct http_request_s *request) {
  struct hsh_token_s token = request->tokens.buf[request->tokens.size - 1];
  return (http_string_t){.buf = &request->buffer.buf[token.index],
                         .len = token.len};
}
