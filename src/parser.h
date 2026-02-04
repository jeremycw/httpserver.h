#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

// HSH_TOK_HEADERS_DONE flags
#define HSH_TOK_FLAG_NO_BODY 0x1
#define HSH_TOK_FLAG_STREAMED_BODY 0x2

// HSH_TOK_BODY flags
#define HSH_TOK_FLAG_BODY_FINAL 0x1
#define HSH_TOK_FLAG_SMALL_BODY 0x2

struct hsh_token_s hsh_parser_exec(struct hsh_parser_s *parser,
                                   struct hsh_buffer_s *buffer,
                                   int max_buf_capacity);
void hsh_parser_init(struct hsh_parser_s *parser);

#endif
