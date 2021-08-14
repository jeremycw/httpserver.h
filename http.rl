#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define HTTP_FLAG_SET(var, flag) var |= flag
#define HTTP_FLAG_CLEAR(var, flag) var &= ~flag
#define HTTP_FLAG_CHECK(var, flag) (var & flag)

#define HSH_PFLAG_CHUNKED 0x1
#define HSH_PFLAG_STREAMED 0x2

enum http_token_e {
  HSH_TOK_METHOD, HSH_TOK_TARGET, HSH_TOK_VERSION, HSH_TOK_HEADER_KEY,
  HSH_TOK_HEADER_VALUE, HSH_TOK_BODY
};

char const* token_names[] = {
  "Method", "Target", "Version", "HeaderKey", "HeaderValue", "Body"
};

struct token_s {
  enum http_token_e type;
  int len;
  int index;
};

struct buffer_s {
  char* buf;
  int32_t capacity;
  int32_t size;
  int32_t index;
  int32_t after_headers_index;
};

struct token_array_s {
  struct token_s* buf;
  int capacity;
  int size;
};

struct parser_s {
  struct token_s token;
  int64_t content_length;
  int64_t content_remaining;
  struct token_array_s tokens;
  int16_t header_count;
  int8_t state;
  uint8_t flags;
};

%%{
  machine http;

  action method { parser->token.type = HSH_TOK_METHOD; parser->token.index = p - buffer->buf; }
  action target {  parser->token.type = HSH_TOK_TARGET; parser->token.index = p - buffer->buf; }
  action version { parser->token.type = HSH_TOK_VERSION; parser->token.index = p - buffer->buf; }
  action header_key { parser->token.type = HSH_TOK_HEADER_KEY; parser->token.index = p - buffer->buf; }
  action header_value {  parser->token.type = HSH_TOK_HEADER_VALUE; parser->token.index = p - buffer->buf; }
  action body { parser->token.type = HSH_TOK_BODY; parser->token.index = p - buffer->buf; }
  action emit_token {
    parser->token.len = p - (buffer->buf + parser->token.index);
    printf("%s: %.*s, %X\n", token_names[parser->token.type], parser->token.len, buffer->buf + parser->token.index, fc);
  }

  action content_length_digit {
    parser->content_length *= 10;
    parser->content_length += fc - '0';
  }

  action transfer_encoding {
    HTTP_FLAG_SET(parser->flags, HSH_PFLAG_CHUNKED);
  }

  action done_headers {
    buffer->after_headers_index = p - buffer->buf + 1;
    parser->content_remaining = parser->content_length;
    if (parser->content_length == 0) {
      request_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
    } else if (HTTP_FLAG_CHECK(parser->flags, HSH_PFLAG_CHUNKED)) {
      HTTP_FLAG_SET(parser->flags, HSH_PFLAG_STREAMED);
      request_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
      fnext chunked_body;
    // The body won't fit into the buffer without resizing it.
    } else if (parser->content_length > buffer->capacity - buffer->after_headers_index) {
      HTTP_FLAG_SET(parser->flags, HSH_PFLAG_STREAMED);
      request_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
      fnext large_body;
    } else {
      printf("Content-Length: %d\n", parser->content_length);
      fnext small_body;
    }
  }

  action chunk_start {
    parser->content_length = 0;
  }

  action chunk_size {
    if (fc >= 'A' && fc <= 'F') {
      parser->content_length *= 0x10;
      parser->content_length += fc - 55;
    } else if (fc >= 'a' && fc <= 'f') {
      parser->content_length *= 0x10;
      parser->content_length += fc - 87;
    } else if (fc >= '0' && fc <= '9') {
      parser->content_length *= 0x10;
      parser->content_length += fc - '0';
    }
  }

  action chunk_read {
    printf("Chunked body! %c\n", fc);

    char* last_body_byte = buffer->buf + parser->token.index + parser->content_length - 1;
    if (pe > last_body_byte) {
      p = last_body_byte;
      parser->token.len = parser->content_length;
      printf("%s: %.*s, %X\n", token_names[parser->token.type], parser->token.len, buffer->buf + parser->token.index, fc);
      body_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
      fnext chunk_end;
    }
    // emit body token
  }

  action end_stream {
    // write 0 byte body to tokens
    body_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
  }

  action small_body_read {
    parser->token.index = buffer->after_headers_index;
    parser->token.len = parser->content_length;
    char* last_body_byte = buffer->buf + parser->token.index + parser->content_length - 1;
    if (pe > last_body_byte) {
      printf("%s: %.*s, %X\n", token_names[parser->token.type], parser->token.len, buffer->buf + parser->token.index, fc);
      request_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
    }
    fhold;
    fbreak;
  }

  action large_body_read {
    printf("Large body read\n");
    parser->token.index = buffer->after_headers_index;
    char* last_body_byte = buffer->buf + parser->token.index + parser->content_remaining - 1;
    if (pe > last_body_byte) {
      parser->token.len = parser->content_remaining;
      parser->content_remaining = 0;
      // push body token
      body_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);

      // with 0 byte body
      body_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
    } else {
      parser->token.len = pe - buffer->buf + parser->token.index;
      parser->content_remaining -= parser->token.len;
      body_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
    }
    printf("%s: %.*s, %X\n", token_names[parser->token.type], parser->token.len, buffer->buf + parser->token.index, fc);
    fhold;
    fbreak;
  }

  token = ^[()<>@,;:\\"/\[\]?={} \t]+;
  crlf = '\r\n';
  lws = crlf [ \t]+;

  method = [a-zA-Z]+ ' ';
  target = [^ ]+ ' ';
  version = 'HTTP/1.' [01] '\r';

  request_line = (
    method >method @emit_token
    target >target @emit_token
    version >version @emit_token '\n' 
  );

  generic_header = (
    ( token ':' ) >header_key @emit_token
    [ \t]* ( ^[ \t]? ^[\r\n]* '\r' ) >header_value @emit_token '\n'
  );

  content_length = (
    ( 'Content-Length'i ':' ) >header_key @emit_token
    [ \t]* digit+ $content_length_digit [ \t]* crlf
  );

  transfer_encoding = (
    ( 'Transfer-Encoding'i ':' ) >header_key @emit_token
    [ \t]* 'chunked' %transfer_encoding [ \t]* crlf
  );

  header = content_length | transfer_encoding | generic_header;

  headers = ( header+ crlf ) @done_headers;

  chunk = (
    ( ^[0] xdigit* ) >chunk_start $chunk_size crlf any+ >body $chunk_read
  );

  zero_chunk = '0' crlf @end_stream;

  chunk_end := crlf ( chunk | zero_chunk );

  chunked_body := chunk* zero_chunk;

  small_body := any+ >body $small_body_read;

  large_body := any+ >body $large_body_read;

  main :=
    request_line
    headers;
}%%

%% write data;

#define HTTP_REQUEST "GET /foo HTTP/1.1\r\nHost: www.jeremycw.com\r\nContent-Length: 16\r\n\r\naaaaaaaa"

typedef void (*hsh_cb_t)(void* data, struct token_s* tokens, int tokens_n, uint8_t flags);

void http_parse(
  struct parser_s* parser,
  struct buffer_s* buffer,
  hsh_cb_t request_cb,
  hsh_cb_t body_cb,
  void* data
) {
  int cs = parser->state, res = 0;
  char* eof = NULL;
  char *p = buffer->buf + buffer->index;
  char *pe = p + buffer->size;
  %% write exec;
  parser->state = cs;
  buffer->index = p - buffer->buf;
  printf("result = %i\n", res );
}

void test_req_cb(void* data, struct token_s* tokens, int tokens_n, uint8_t flags) {
  printf("Request callback\n");
}

void test_body_cb(void* data, struct token_s* tokens, int tokens_n, uint8_t flags) {
  printf("Body callback\n");
}

int main(int argc, char **argv) {
  struct parser_s parser = { 0 };
  struct buffer_s buffer = { 0 };
  parser.state = http_start;
  buffer.buf = malloc(1024);
  buffer.capacity = sizeof(HTTP_REQUEST);
  memcpy(buffer.buf, HTTP_REQUEST, sizeof(HTTP_REQUEST));
  buffer.size = sizeof(HTTP_REQUEST) - 1;
  http_parse(&parser, &buffer, test_req_cb, test_body_cb, NULL);
  return 0;
}
