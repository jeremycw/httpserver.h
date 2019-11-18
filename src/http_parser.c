#ifndef HTTPSERVER_H
#include "http_parser.h"
#endif

#define HTTP_LWS 2
#define HTTP_CR 3
#define HTTP_CRLF 4

#define HTTP_HEADER_END 5

#define CONTENT_LENGTH_LOW "content-length"
#define CONTENT_LENGTH_UP "CONTENT-LENGTH"

http_token_t http_parse(http_parser_t* parser, char* input, int n) {
  for (int i = parser->start; i < n; ++i, parser->start = i + 1, parser->len++) {
    char c = input[i];
    switch (parser->state) {
      case HTTP_METHOD:
        if (c == ' ') {
          http_token_t token = {
            .index = parser->token_start_index,
            .type = parser->state,
            .len = parser->len
          };
          parser->state = HTTP_TARGET;
          parser->len = 0;
          parser->token_start_index = i + 1;
          return token;
        }
        break;
      case HTTP_TARGET:
        if (c == ' ') {
          http_token_t token = {
            .index = parser->token_start_index,
            .type = parser->state,
            .len = parser->len
          };
          parser->state = HTTP_VERSION;
          parser->token_start_index = i + 1;
          parser->len = 0;
          return token;
        }
        break;
      case HTTP_VERSION:
        if (c == '\r') {
          parser->sub_state = HTTP_CR;
          return (http_token_t) {
            .index = parser->token_start_index,
            .type = HTTP_VERSION,
            .len = parser->len 
          };
        } else if (parser->sub_state == HTTP_CR && c == '\n') {
          parser->sub_state = 0;
          parser->len = 0;
          parser->token_start_index = i + 1;
          parser->state = HTTP_HEADER_KEY;
        }
        break;
      case HTTP_HEADER_KEY:
        if (c == ':') {
          parser->state = HTTP_HEADER_VALUE;
          parser->sub_state = HTTP_LWS;
          if (parser->len == parser->content_length_i + 1) parser->in_content_length = 1;
          parser->content_length_i = 0;
          return (http_token_t) {
            .index = parser->token_start_index,
            .type = HTTP_HEADER_KEY,
            .len = parser->len - 1
          };
        } else if (
          (c == CONTENT_LENGTH_UP[parser->content_length_i] ||
            c == CONTENT_LENGTH_LOW[parser->content_length_i]) &&
          parser->content_length_i < sizeof(CONTENT_LENGTH_LOW) - 1
        ) {
          parser->content_length_i++;
        }
        break;
      case HTTP_HEADER_VALUE:
        if (parser->sub_state == HTTP_LWS && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
          continue;
        } else if (parser->sub_state == HTTP_LWS) {
          parser->sub_state = 0;
          parser->len = 0;
          parser->token_start_index = i;
          if (parser->in_content_length) {
            parser->content_length *= 10;
            parser->content_length += c - '0';
          }
        } else if (parser->sub_state != HTTP_LWS && c == '\r') {
          parser->sub_state = HTTP_CR;
          parser->state = HTTP_HEADER_END;
          parser->in_content_length = 0;
          return (http_token_t) {
            .index = parser->token_start_index,
            .type = HTTP_HEADER_VALUE,
            .len = parser->len
          };
        } else if (parser->in_content_length) {
          parser->content_length *= 10;
          parser->content_length += c - '0';
        }
        break;
      case HTTP_HEADER_END:
        if (parser->sub_state == 0 && c == '\r') {
          parser->sub_state = HTTP_CR;
        } else if (parser->sub_state == HTTP_CR && c == '\n') {
          parser->sub_state = HTTP_CRLF;
        } else if (parser->sub_state == HTTP_CRLF && c == '\r') {
          parser->sub_state = 0;
          return (http_token_t) {
            .index = i + 2,
            .type = HTTP_BODY,
            .len = parser->content_length
          };
        } else if (parser->sub_state == HTTP_CRLF && c != '\r') {
          parser->sub_state = 0;
          parser->len = 0;
          parser->token_start_index = i;
          i--;
          parser->state = HTTP_HEADER_KEY;
        }
        break;

    }
  }
  return (http_token_t) { .index = 0, .type = HTTP_NONE, .len = 0 };
}
