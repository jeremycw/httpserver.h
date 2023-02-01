#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef HTTPSERVER_IMPL
#include "common.h"
#include "parser.h"
#include "read_socket.h"
#endif

void _hs_token_array_push(struct hs_token_array_s *array,
                          struct hsh_token_s a) {
  if (array->size == array->capacity) {
    array->capacity *= 2;
    array->buf = (struct hsh_token_s *)realloc(
        array->buf, array->capacity * sizeof(struct hsh_token_s));
    assert(array->buf != NULL);
  }
  array->buf[array->size] = a;
  array->size++;
}

void _hs_buffer_init(struct hsh_buffer_s *buffer, int initial_capacity,
                     int64_t *memused) {
  *buffer = (struct hsh_buffer_s){0};
  buffer->buf = (char *)calloc(1, initial_capacity);
  *memused += initial_capacity;
  assert(buffer->buf != NULL);
  buffer->capacity = initial_capacity;
}

int _hs_read_into_buffer(struct hsh_buffer_s *buffer, int request_socket,
                         int64_t *server_memused,
                         int64_t max_request_buf_capacity) {
  int bytes;
  do {
    bytes = read(request_socket, buffer->buf + buffer->length,
                 buffer->capacity - buffer->length);
    if (bytes > 0)
      buffer->length += bytes;

    if (buffer->length == buffer->capacity &&
        buffer->capacity != max_request_buf_capacity) {
      *server_memused -= buffer->capacity;
      buffer->capacity *= 2;
      if (buffer->capacity > max_request_buf_capacity) {
        buffer->capacity = max_request_buf_capacity;
      }
      *server_memused += buffer->capacity;
      buffer->buf = (char *)realloc(buffer->buf, buffer->capacity);
      assert(buffer->buf != NULL);
    }
  } while (bytes > 0 && buffer->capacity < max_request_buf_capacity);

  buffer->sequence_id++;

  return bytes;
}

int _hs_buffer_requires_read(struct hsh_buffer_s *buffer) {
  return buffer->index >= buffer->length;
}

void _hs_exec_callback(http_request_t *request,
                       void (*cb)(struct http_request_s *)) {
  request->state = HTTP_SESSION_NOP;
  cb(request);
}

enum hs_read_rc_e
_hs_parse_buffer_and_exec_user_cb(http_request_t *request,
                                  int max_request_buf_capacity) {
  enum hs_read_rc_e rc = HS_READ_RC_SUCCESS;

  do {
    struct hsh_token_s token = hsh_parser_exec(
        &request->parser, &request->buffer, max_request_buf_capacity);

    switch (token.type) {
    case HSH_TOK_HEADERS_DONE:
      _hs_token_array_push(&request->tokens, token);
      if (HTTP_FLAG_CHECK(token.flags, HSH_TOK_FLAG_STREAMED_BODY) ||
          HTTP_FLAG_CHECK(token.flags, HSH_TOK_FLAG_NO_BODY)) {
        HTTP_FLAG_SET(request->flags, HTTP_FLG_STREAMED);
        _hs_exec_callback(request, request->server->request_handler);
        return rc;
      }
      break;
    case HSH_TOK_BODY:
      _hs_token_array_push(&request->tokens, token);
      if (HTTP_FLAG_CHECK(token.flags, HSH_TOK_FLAG_SMALL_BODY)) {
        _hs_exec_callback(request, request->server->request_handler);
      } else {
        if (HTTP_FLAG_CHECK(token.flags, HSH_TOK_FLAG_BODY_FINAL) &&
            token.len > 0) {
          _hs_exec_callback(request, request->chunk_cb);

          // A zero length body is used to indicate to the user code that the
          // body has finished streaming. This is natural when dealing with
          // chunked request bodies but requires us to inject a zero length
          // body for non-chunked requests.
          struct hsh_token_s token = {};
          memset(&token, 0, sizeof(struct hsh_token_s));
          token.type = HSH_TOK_BODY;
          _hs_token_array_push(&request->tokens, token);
          _hs_exec_callback(request, request->chunk_cb);
        } else {
          _hs_exec_callback(request, request->chunk_cb);
        }
      }
      return rc;
    case HSH_TOK_ERR:
      return HS_READ_RC_PARSE_ERR;
    case HSH_TOK_NONE:
      return rc;
    default:
      _hs_token_array_push(&request->tokens, token);
      break;
    }
  } while (1);
}

// Reads the request socket if required and parses HTTP in a non-blocking
// manner.
//
// It should be called when a new connection is established and when a read
// ready event occurs for the request socket. It parses the HTTP request and
// fills the tokens array of the request struct. It will also invoke the
// request_hander callback and the chunk_cb callback in the appropriate
// scenarios.
enum hs_read_rc_e hs_read_request_and_exec_user_cb(http_request_t *request,
                                                   struct hs_read_opts_s opts) {
  request->state = HTTP_SESSION_READ;
  request->timeout = HTTP_REQUEST_TIMEOUT;

  if (request->buffer.buf == NULL) {
    _hs_buffer_init(&request->buffer, opts.initial_request_buf_capacity,
                    &request->server->memused);
    hsh_parser_init(&request->parser);
  }

  if (_hs_buffer_requires_read(&request->buffer)) {
    int bytes = _hs_read_into_buffer(&request->buffer, request->socket,
                                     &request->server->memused,
                                     opts.max_request_buf_capacity);

    if (bytes == opts.eof_rc) {
      return HS_READ_RC_SOCKET_ERR;
    }
  }

  return _hs_parse_buffer_and_exec_user_cb(request,
                                           opts.max_request_buf_capacity);
}
