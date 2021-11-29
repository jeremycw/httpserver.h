#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define HSH_FLAG_CHECK(var, flag) (var & flag)

// HSH_TOK_HEADERS_DONE flags
#define HSH_TOK_FLAG_NO_BODY 0x1
#define HSH_TOK_FLAG_STREAMED_BODY 0x2

// HSH_TOK_BODY flags
#define HSH_TOK_FLAG_BODY_FINAL 0x1

enum hsh_token_e {
  HSH_TOK_METHOD, HSH_TOK_TARGET, HSH_TOK_VERSION, HSH_TOK_HEADER_KEY,
  HSH_TOK_HEADER_VALUE, HSH_TOK_HEADERS_DONE, HSH_TOK_BODY, HSH_TOK_NONE
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

// struct hsh_token_array_s {
//   struct hsh_token_s* buf;
//   int capacity;
//   int size;
// };

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
