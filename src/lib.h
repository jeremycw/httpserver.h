#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

enum hsh_token_e {
  HSH_TOK_METHOD, HSH_TOK_TARGET, HSH_TOK_VERSION, HSH_TOK_HEADER_KEY,
  HSH_TOK_HEADER_VALUE, HSH_TOK_HEADERS_DONE, HSH_TOK_BODY, HSH_TOK_NONE,
  HSH_TOK_ERR
};

struct hsh_token_s {
  enum hsh_token_e type;
  uint8_t flags;
  int len;
  int index;
};

struct hsh_buffer_s {
  char* buf;
  int32_t capacity;
  int32_t length;
  int32_t index;
  int32_t after_headers_index;
  int8_t sequence_id;
};

struct hs_token_array_s {
  struct hsh_token_s* buf;
  int capacity;
  int size;
};

void hsh_buffer_init(struct hsh_buffer_s* buffer, int initial_capacity);
void hs_token_array_push(struct hs_token_array_s* array, struct hsh_token_s a);
void hs_token_array_init(struct hs_token_array_s* array, int capacity);

#endif
