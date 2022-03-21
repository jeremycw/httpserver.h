#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define HSH_FLAG_CHECK(var, flag) (var & flag)

// HSH_TOK_HEADERS_DONE flags
#define HSH_TOK_FLAG_NO_BODY 0x1
#define HSH_TOK_FLAG_STREAMED_BODY 0x2

// HSH_TOK_BODY flags
#define HSH_TOK_FLAG_BODY_FINAL 0x1
#define HSH_TOK_FLAG_SMALL_BODY 0x2

#include <stdint.h>
#include "lib.h"

struct hsh_parser_s {
  int64_t content_length;
  int64_t content_remaining;
  struct hsh_token_s token;
  int16_t limit_count;
  int16_t limit_max;
  int8_t state;
  int8_t flags;
  int8_t sequence_id;
};

struct hsh_token_s hsh_parser_exec(struct hsh_parser_s* parser, struct hsh_buffer_s* buffer, int max_buf_capacity);
void hsh_parser_init(struct hsh_parser_s* parser);

#endif
