#include <string.h>
#include <stdlib.h>

#ifndef HTTPSERVER_IMPL
#include "common.h"
#include "parser.h"
#endif

#define HSH_P_FLAG_CHUNKED 0x1
#define HSH_P_FLAG_TOKEN_READY 0x2
#define HSH_P_FLAG_DONE 0x4

#define HSH_ENTER_TOKEN(tok_type, max_len) \
  parser->token.type = tok_type; \
  parser->token.index = p - buffer->buf; \
  parser->token.flags = 0; \
  parser->limit_count = 0; \
  parser->limit_max = max_len;

%%{
  machine hsh_http;

  action method { HSH_ENTER_TOKEN(HSH_TOK_METHOD, 32) }
  action target { HSH_ENTER_TOKEN(HSH_TOK_TARGET, 1024) }
  action version { HSH_ENTER_TOKEN(HSH_TOK_VERSION, 16) }
  action header_key { HSH_ENTER_TOKEN(HSH_TOK_HEADER_KEY, 256) }
  action header_value { HSH_ENTER_TOKEN(HSH_TOK_HEADER_VALUE, 4096) }
  action body {
    parser->token.type = HSH_TOK_BODY;
    parser->token.flags = 0;
    parser->token.index = p - buffer->buf;
  }
  action emit_token {
    parser->token.len = p - (buffer->buf + parser->token.index);
    // hsh_token_array_push(&parser->tokens, parser->token);
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
    fbreak;
  }

  action content_length_digit {
    parser->content_length *= 10;
    parser->content_length += fc - '0';
  }

  action transfer_encoding {
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_CHUNKED);
  }

  action reset_count {
    parser->limit_count = 0;
    parser->limit_max = 256;
  }

  action inc_count {
    parser->limit_count++;
    if (parser->limit_count > parser->limit_max) {
      // parser->rc = (int8_t)HSH_PARSER_ERR;
      fbreak;
    }
  }

  action done_headers {
    buffer->after_headers_index = p - buffer->buf + 1;
    parser->content_remaining = parser->content_length;
    parser->token = (struct hsh_token_s){ };
    parser->token.type = HSH_TOK_HEADERS_DONE;
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
    if (HTTP_FLAG_CHECK(parser->flags, HSH_P_FLAG_CHUNKED)) {
      HTTP_FLAG_SET(parser->token.flags, HSH_TOK_FLAG_STREAMED_BODY);
      fnext chunked_body;
      fbreak;
    } else if (parser->content_length == 0) {
      HTTP_FLAG_SET(parser->token.flags, HSH_TOK_FLAG_NO_BODY);
      fbreak;
    // The body won't fit into the buffer at maximum capacity.
    } else if (parser->content_length > max_buf_capacity - buffer->after_headers_index) {
      HTTP_FLAG_SET(parser->token.flags, HSH_TOK_FLAG_STREAMED_BODY);
      fnext large_body;
      fbreak;
    } else {
      // Resize the buffer to hold the full body
      if (parser->content_length + buffer->after_headers_index > buffer->capacity) {
        buffer->buf = (char*)realloc(buffer->buf, parser->content_length + buffer->after_headers_index);
        buffer->capacity = parser->content_length + buffer->after_headers_index;
      }
      fnext small_body;
      fbreak;
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
    char* last_body_byte = buffer->buf + parser->token.index + parser->content_length - 1;
    if (pe >= last_body_byte) {
      p = last_body_byte;
      parser->token.len = parser->content_length;
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
      fnext chunk_end;
      fbreak;
    // The current chunk is at the end of the buffer and the buffer cannot be expanded.
    // Move the remaining contents of the buffer to just after the headers to free up
    // capacity in the buffer.
    } else if (p - buffer->buf + parser->content_length > max_buf_capacity) {
      memcpy(buffer->buf + buffer->after_headers_index, p, pe - p);
      buffer->length = buffer->after_headers_index + pe - p;
      p = buffer->buf + buffer->after_headers_index;
      parser->token.index = buffer->after_headers_index;
      parser->sequence_id = buffer->sequence_id;
      fhold;
      fbreak;
    }
  }

  action end_stream {
    // write 0 byte body to tokens
    parser->token.type = HSH_TOK_BODY;
    parser->token.index = 0;
    parser->token.len = 0;
    parser->token.flags = HSH_TOK_FLAG_BODY_FINAL;
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_DONE);
    fbreak;
  }

  action small_body_read {
    parser->token.index = buffer->after_headers_index;
    parser->token.len = parser->content_length;
    HTTP_FLAG_SET(parser->token.flags, HSH_TOK_FLAG_SMALL_BODY);
    char* last_body_byte = buffer->buf + parser->token.index + parser->content_length - 1;
    if (pe >= last_body_byte) {
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_DONE);
    }
    p = pe;
    fhold;
    fbreak;
  }

  action large_body_read {
    parser->token.index = buffer->after_headers_index;
    char* last_body_byte = buffer->buf + buffer->after_headers_index + parser->content_remaining - 1;
    if (pe >= last_body_byte) {
      parser->token.flags = HSH_TOK_FLAG_BODY_FINAL;
      parser->token.len = parser->content_remaining;
      parser->content_remaining = 0;
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_DONE);
    } else {
      parser->token.len = pe - p;
      parser->content_remaining -= parser->token.len;
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
      p = buffer->buf + buffer->after_headers_index;
      buffer->length = buffer->after_headers_index;
      parser->sequence_id = buffer->sequence_id;
    }
    fhold;
    fbreak;
  }

  action error {
    // parser->rc = (int8_t)HSH_PARSER_ERR;
    fbreak;
  }

  token = ^[()<>@,;:\\"/\[\]?={} \t]+ >reset_count @inc_count;
  crlf = '\r\n';
  lws = crlf [ \t]+;
  ows = [ \t]+;

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
    ows digit+ $content_length_digit ows crlf
  );

  transfer_encoding = (
    ( 'Transfer-Encoding'i ':' ) >header_key @emit_token
    ows 'chunked' @transfer_encoding ows crlf
  );

  header = content_length | transfer_encoding | generic_header;

  headers = ( header+ >reset_count @inc_count crlf ) @done_headers;

  chunk = (
    ( ^[0] xdigit* ) >chunk_start $chunk_size crlf any+ >body $chunk_read
  );

  zero_chunk = '0' crlf @end_stream;

  chunk_end := ( crlf ( chunk | zero_chunk ) ) $!error;

  chunked_body := ( chunk* zero_chunk ) $!error;

  small_body := any+ >body $small_body_read;

  large_body := any+ >body $large_body_read;

  main :=
    request_line $!error
    headers $!error;
}%%

%% write data;

void hsh_parser_init(struct hsh_parser_s* parser) {
  memset(parser, 0, sizeof(struct hsh_parser_s));
  parser->state = hsh_http_start;
}

struct hsh_token_s hsh_parser_exec(struct hsh_parser_s* parser, struct hsh_buffer_s* buffer, int max_buf_capacity) {
  struct hsh_token_s none = {};
  none.type = HSH_TOK_NONE;
  if (HTTP_FLAG_CHECK(parser->flags, HSH_P_FLAG_DONE) || parser->sequence_id == buffer->sequence_id) {
    return none;
  }
  int cs = parser->state;
  char* eof = NULL;
  char *p = buffer->buf + buffer->index;
  char *pe = buffer->buf + buffer->length;
  %% write exec;
  parser->state = cs;
  buffer->index = p - buffer->buf;
  if (HTTP_FLAG_CHECK(parser->flags, HSH_P_FLAG_TOKEN_READY)) {
    HTTP_FLAG_CLEAR(parser->flags, HSH_P_FLAG_TOKEN_READY);
    return parser->token;
  } else {
    parser->sequence_id = buffer->sequence_id;
    return none;
  }
}
