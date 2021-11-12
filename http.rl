#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define HSH_FLAG_SET(var, flag) var |= flag
#define HSH_FLAG_CHECK(var, flag) (var & flag)

#define HSH_P_FLAG_CHUNKED 0x1
#define HSH_FLAG_STREAMED 0x2

#define HSH_ENTER_TOKEN(tok_type, max_len) \
  parser->token.type = tok_type; \
  parser->token.index = p - buffer->buf; \
  parser->limit_count = 0; \
  parser->limit_max = max_len;

enum hsh_token_e {
  HSH_TOK_METHOD, HSH_TOK_TARGET, HSH_TOK_VERSION, HSH_TOK_HEADER_KEY,
  HSH_TOK_HEADER_VALUE, HSH_TOK_BODY
};

struct hsh_token_s {
  enum hsh_token_e type;
  int len;
  int index;
};

struct hsh_buffer_s {
  char* buf;
  int32_t capacity;
  int32_t size;
  int32_t index;
  int32_t after_headers_index;
};

struct hsh_token_array_s {
  struct hsh_token_s* buf;
  int capacity;
  int size;
};

struct hsh_parser_s {
  struct hsh_token_s token;
  int64_t content_length;
  int64_t content_remaining;
  struct hsh_token_array_s tokens;
  int16_t limit_count;
  int16_t limit_max;
  int8_t state;
  uint8_t flags;
};

%%{
  machine hsh_http;

  action method { HSH_ENTER_TOKEN(HSH_TOK_METHOD, 32) }
  action target { HSH_ENTER_TOKEN(HSH_TOK_TARGET, 1024) }
  action version { HSH_ENTER_TOKEN(HSH_TOK_VERSION, 16) }
  action header_key { HSH_ENTER_TOKEN(HSH_TOK_HEADER_KEY, 256) }
  action header_value { HSH_ENTER_TOKEN(HSH_TOK_HEADER_VALUE, 4096) }
  action body { parser->token.type = HSH_TOK_BODY; parser->token.index = p - buffer->buf; }
  action emit_token {
    parser->token.len = p - (buffer->buf + parser->token.index);
    printf("%.*s, %X\n", parser->token.len, buffer->buf + parser->token.index, fc);
  }

  action content_length_digit {
    parser->content_length *= 10;
    parser->content_length += fc - '0';
  }

  action transfer_encoding {
    HSH_FLAG_SET(parser->flags, HSH_P_FLAG_CHUNKED);
  }

  action reset_count {
    parser->limit_count = 0;
    parser->limit_max = 256;
  }

  action inc_count {
    parser->limit_count++;
    if (parser->limit_count > parser->limit_max) {
      error_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
      fbreak;
    }
  }

  action done_headers {
    buffer->after_headers_index = p - buffer->buf + 1;
    parser->content_remaining = parser->content_length;
    if (parser->content_length == 0) {
      request_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
    } else if (HSH_FLAG_CHECK(parser->flags, HSH_P_FLAG_CHUNKED)) {
      HSH_FLAG_SET(parser->flags, HSH_FLAG_STREAMED);
      request_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
      fnext chunked_body;
    // The body won't fit into the buffer without resizing it.
    } else if (parser->content_length > buffer->capacity - buffer->after_headers_index) {
      HSH_FLAG_SET(parser->flags, HSH_FLAG_STREAMED);
      request_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
      fnext large_body;
    } else {
      printf("Content-Length: %lld\n", parser->content_length);
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
      printf("%.*s, %X\n", parser->token.len, buffer->buf + parser->token.index, fc);
      body_cb(data, parser->tokens.buf, parser->tokens.size, parser->flags);
      fnext chunk_end;
    }
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
      printf("%.*s, %X\n", parser->token.len, buffer->buf + parser->token.index, fc);
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
    printf("%.*s, %X\n", parser->token.len, buffer->buf + parser->token.index, fc);
    fhold;
    fbreak;
  }

  token = ^[()<>@,;:\\"/\[\]?={} \t]+ >reset_count @inc_count;
  crlf = '\r\n';
  lws = crlf [ \t]+ >reset_count @inc_count;
  ows = [ \t]+ >reset_count @inc_count;

  method = [a-zA-Z]+ @inc_count ' ';
  target = [^ ]+ @inc_count ' ';
  version = 'HTTP/1.' [01] '\r';

  request_line = (
    method >method @emit_token
    target >target @emit_token
    version >version @emit_token '\n' 
  );

  generic_header = (
    ( token ':' ) >header_key @emit_token
    ows ( ^[ \t]? @inc_count ^[\r\n]* @inc_count '\r' ) >header_value @emit_token '\n'
  );

  content_length = (
    ( 'Content-Length'i ':' ) >header_key @emit_token
    ows digit+ >reset_count @inc_count $content_length_digit ows crlf
  );

  transfer_encoding = (
    ( 'Transfer-Encoding'i ':' ) >header_key @emit_token
    ows 'chunked' %transfer_encoding ows crlf
  );

  header = content_length | transfer_encoding | generic_header;

  headers = ( header+ >reset_count @inc_count crlf ) @done_headers;

  chunk = (
    ( ^[0] xdigit* >reset_count @inc_count ) >chunk_start $chunk_size crlf any+ >body $chunk_read
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

typedef void (*hsh_cb_t)(void* data, struct hsh_token_s* tokens, int tokens_n, uint8_t flags);

void hsh_token_array_push(struct hsh_token_array_s* array, struct hsh_token_s a) {
  if (array->size == array->capacity) {
    array->capacity *= 2;
    array->buf = (struct hsh_token_s*)realloc(array->buf, array->capacity * sizeof(struct hsh_token_s));
    assert(array->buf != NULL);
  }
  array->buf[array->size] = a;
  array->size++;
}

void hsh_token_array_init(struct hsh_token_array_s* array, int capacity) {
  array->buf = (struct hsh_token_s*)malloc(sizeof(struct hsh_token_s) * capacity);
  assert(array->buf != NULL);
  array->size = 0;
  array->capacity = capacity;
}

void hsh_parse(
  struct hsh_parser_s* parser,
  struct hsh_buffer_s* buffer,
  hsh_cb_t request_cb,
  hsh_cb_t body_cb,
  hsh_cb_t error_cb,
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

void test_req_cb(void* data, struct hsh_token_s* tokens, int tokens_n, uint8_t flags) {
  printf("Request callback\n");
}

void test_body_cb(void* data, struct hsh_token_s* tokens, int tokens_n, uint8_t flags) {
  printf("Body callback\n");
}

void test_error_cb(void* data, struct hsh_token_s* tokens, int tokens_n, uint8_t flags) {
  printf("Error callback\n");
}

int main(int argc, char **argv) {
  struct hsh_parser_s parser = { 0 };
  struct hsh_buffer_s buffer = { 0 };
  parser.state = hsh_http_start;
  buffer.buf = malloc(1024);
  buffer.capacity = sizeof(HTTP_REQUEST);
  memcpy(buffer.buf, HTTP_REQUEST, sizeof(HTTP_REQUEST));
  buffer.size = sizeof(HTTP_REQUEST) - 1;
  hsh_parse(&parser, &buffer, test_req_cb, test_body_cb, test_error_cb, NULL);
  return 0;
}
