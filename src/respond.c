#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HTTPSERVER_IMPL
#include "buffer_util.h"
#include "common.h"
#include "request_util.h"
#include "respond.h"
#endif

char const *hs_status_text[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "",

    // 100s
    "Continue", "Switching Protocols", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "",

    // 200s
    "OK", "Created", "Accepted", "Non-Authoritative Information", "No Content",
    "Reset Content", "Partial Content", "", "", "",

    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "",

    // 300s
    "Multiple Choices", "Moved Permanently", "Found", "See Other",
    "Not Modified", "Use Proxy", "", "Temporary Redirect", "", "",

    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "",

    // 400s
    "Bad Request", "Unauthorized", "Payment Required", "Forbidden", "Not Found",
    "Method Not Allowed", "Not Acceptable", "Proxy Authentication Required",
    "Request Timeout", "Conflict",

    "Gone", "Length Required", "", "Payload Too Large", "", "", "", "", "", "",

    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "",

    // 500s
    "Internal Server Error", "Not Implemented", "Bad Gateway",
    "Service Unavailable", "Gateway Timeout", "", "", "", "", "",

    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", ""};
typedef struct {
  char *buf;
  int capacity;
  int size;
  int64_t *memused;
} grwprintf_t;

void _grwprintf_init(grwprintf_t *ctx, int capacity, int64_t *memused) {
  ctx->memused = memused;
  ctx->size = 0;
  ctx->buf = (char *)malloc(capacity);
  *ctx->memused += capacity;
  assert(ctx->buf != NULL);
  ctx->capacity = capacity;
}

void _grwmemcpy(grwprintf_t *ctx, char const *src, int size) {
  if (ctx->size + size > ctx->capacity) {
    *ctx->memused -= ctx->capacity;
    ctx->capacity = ctx->size + size;
    *ctx->memused += ctx->capacity;
    ctx->buf = (char *)realloc(ctx->buf, ctx->capacity);
    assert(ctx->buf != NULL);
  }
  memcpy(ctx->buf + ctx->size, src, size);
  ctx->size += size;
}

void _grwprintf(grwprintf_t *ctx, char const *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  int bytes =
      vsnprintf(ctx->buf + ctx->size, ctx->capacity - ctx->size, fmt, args);
  if (bytes + ctx->size > ctx->capacity) {
    *ctx->memused -= ctx->capacity;
    while (bytes + ctx->size > ctx->capacity)
      ctx->capacity *= 2;
    *ctx->memused += ctx->capacity;
    ctx->buf = (char *)realloc(ctx->buf, ctx->capacity);
    assert(ctx->buf != NULL);
    bytes +=
        vsnprintf(ctx->buf + ctx->size, ctx->capacity - ctx->size, fmt, args);
  }
  ctx->size += bytes;

  va_end(args);
}

void _http_serialize_headers_list(http_response_t *response,
                                  grwprintf_t *printctx) {
  http_header_t *header = response->headers;
  while (header) {
    _grwprintf(printctx, "%s: %s\r\n", header->key, header->value);
    header = header->next;
  }
  _grwprintf(printctx, "\r\n");
}

void _http_serialize_headers(http_request_t *request, http_response_t *response,
                             grwprintf_t *printctx) {
  if (HTTP_FLAG_CHECK(request->flags, HTTP_AUTOMATIC)) {
    hs_request_detect_keep_alive_flag(request);
  }
  if (HTTP_FLAG_CHECK(request->flags, HTTP_KEEP_ALIVE)) {
    hs_response_set_header(response, "Connection", "keep-alive");
  } else {
    hs_response_set_header(response, "Connection", "close");
  }
  _grwprintf(printctx, "HTTP/1.1 %d %s\r\nDate: %s\r\n", response->status,
             hs_status_text[response->status], request->server->date);
  if (!HTTP_FLAG_CHECK(request->flags, HTTP_CHUNKED_RESPONSE)) {
    _grwprintf(printctx, "Content-Length: %d\r\n", response->content_length);
  }
  _http_serialize_headers_list(response, printctx);
}

void _http_perform_response(http_request_t *request, http_response_t *response,
                            grwprintf_t *printctx, hs_req_fn_t http_write) {
  http_header_t *header = response->headers;
  while (header) {
    http_header_t *tmp = header;
    header = tmp->next;
    free(tmp);
  }
  _hs_buffer_free(&request->buffer, &request->server->memused);
  free(response);
  request->buffer.buf = printctx->buf;
  request->buffer.length = printctx->size;
  request->buffer.capacity = printctx->capacity;
  request->bytes_written = 0;
  request->state = HTTP_SESSION_WRITE;
  http_write(request);
}

// See api.h http_response_header
void hs_response_set_header(http_response_t *response, char const *key,
                            char const *value) {
  http_header_t *header = (http_header_t *)malloc(sizeof(http_header_t));
  assert(header != NULL);
  header->key = key;
  header->value = value;
  http_header_t *prev = response->headers;
  header->next = prev;
  response->headers = header;
}

// Serializes the response into the request buffer and calls http_write.
// See api.h http_respond for more details
void hs_request_respond(http_request_t *request, http_response_t *response,
                        hs_req_fn_t http_write) {
  grwprintf_t printctx;
  _grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE, &request->server->memused);
  _http_serialize_headers(request, response, &printctx);
  if (response->body) {
    _grwmemcpy(&printctx, response->body, response->content_length);
  }
  _http_perform_response(request, response, &printctx, http_write);
}

// Serializes a chunk into the request buffer and calls http_write.
// See api.h http_respond_chunk for more details.
void hs_request_respond_chunk(http_request_t *request,
                              http_response_t *response, hs_req_fn_t cb,
                              hs_req_fn_t http_write) {
  grwprintf_t printctx;
  _grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE, &request->server->memused);
  if (!HTTP_FLAG_CHECK(request->flags, HTTP_CHUNKED_RESPONSE)) {
    HTTP_FLAG_SET(request->flags, HTTP_CHUNKED_RESPONSE);
    hs_response_set_header(response, "Transfer-Encoding", "chunked");
    _http_serialize_headers(request, response, &printctx);
  }
  request->chunk_cb = cb;
  _grwprintf(&printctx, "%X\r\n", response->content_length);
  _grwmemcpy(&printctx, response->body, response->content_length);
  _grwprintf(&printctx, "\r\n");
  _http_perform_response(request, response, &printctx, http_write);
}

// Serializes the zero sized final chunk into the request buffer and calls
// http_write. See api.h http_respond_chunk_end for more details.
void hs_request_respond_chunk_end(http_request_t *request,
                                  http_response_t *response,
                                  hs_req_fn_t http_write) {
  grwprintf_t printctx;
  _grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE, &request->server->memused);
  _grwprintf(&printctx, "0\r\n");
  _http_serialize_headers_list(response, &printctx);
  _grwprintf(&printctx, "\r\n");
  HTTP_FLAG_CLEAR(request->flags, HTTP_CHUNKED_RESPONSE);
  _http_perform_response(request, response, &printctx, http_write);
}

// See api.h http_response_status
void hs_response_set_status(http_response_t *response, int status) {
  response->status = status > 599 || status < 100 ? 500 : status;
}

// See api.h http_response_body
void hs_response_set_body(http_response_t *response, char const *body,
                          int length) {
  response->body = body;
  response->content_length = length;
}

// See api.h http_response_init
http_response_t *hs_response_init() {
  http_response_t *response =
      (http_response_t *)calloc(1, sizeof(http_response_t));
  assert(response != NULL);
  response->status = 200;
  return response;
}

// Simple less flexible interface for responses, used for errors.
void hs_request_respond_error(http_request_t *request, int code,
                              char const *message, hs_req_fn_t http_write) {
  struct http_response_s *response = hs_response_init();
  hs_response_set_status(response, code);
  hs_response_set_header(response, "Content-Type", "text/plain");
  hs_response_set_body(response, message, strlen(message));
  hs_request_respond(request, response, http_write);
  http_write(request);
}
