#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define HTTP_METHOD 0
#define HTTP_TARGET 1
#define HTTP_VERSION 2
#define HTTP_HEADER_KEY 3
#define HTTP_HEADER_VALUE 4
#define HTTP_NONE 6
#define HTTP_BODY 7

typedef struct {
  int index;
  int len;
  int type;
} http_token_t;

typedef struct {
  int content_length;
  int len;
  int token_start_index;
  int start;
  int content_length_i;
  char in_content_length;
  char state;
  char sub_state;
} http_parser_t;

http_token_t http_parse(http_parser_t* parser, char* input, int n);

#endif
