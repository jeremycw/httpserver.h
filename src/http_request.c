#include <string.h>
#ifndef HTTPSERVER_H
#include "http_request.h"
#endif


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

void http_request_set_flag(http_request_t* request, int flag, int value) {
  if (value) {
    HTTP_FLAG_SET(request->flags, flag);
  } else {
    HTTP_FLAG_CLEAR(request->flags, flag);
  }
}
