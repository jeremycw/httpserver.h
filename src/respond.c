#include <assert.h>
#include <stdarg.h>
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

void _http_buffer_headers(http_request_t *request, http_response_t *response,
                          grwprintf_t *printctx) {
  http_header_t *header = response->headers;
  while (header) {
    _grwprintf(printctx, "%s: %s\r\n", header->key, header->value);
    header = header->next;
  }
  if (!HTTP_FLAG_CHECK(request->flags, HTTP_CHUNKED_RESPONSE)) {
    _grwprintf(printctx, "Content-Length: %d\r\n", response->content_length);
  }
  _grwprintf(printctx, "\r\n");
}

void _http_respond_headers(http_request_t *request, http_response_t *response,
                           grwprintf_t *printctx) {
  if (HTTP_FLAG_CHECK(request->flags, HTTP_AUTOMATIC)) {
    hs_auto_detect_keep_alive(request);
  }
  if (HTTP_FLAG_CHECK(request->flags, HTTP_KEEP_ALIVE)) {
    hs_response_header(response, "Connection", "keep-alive");
  } else {
    hs_response_header(response, "Connection", "close");
  }
  _grwprintf(printctx, "HTTP/1.1 %d %s\r\nDate: %s\r\n", response->status,
             hs_status_text[response->status], request->server->date);
  _http_buffer_headers(request, response, printctx);
}

void _http_end_response(http_request_t *request, http_response_t *response,
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
  request->state = HTTP_SESSION_WRITE;
  http_write(request);
}

void hs_response_header(http_response_t *response, char const *key,
                        char const *value) {
  http_header_t *header = (http_header_t *)malloc(sizeof(http_header_t));
  assert(header != NULL);
  header->key = key;
  header->value = value;
  http_header_t *prev = response->headers;
  header->next = prev;
  response->headers = header;
}

void hs_respond(http_request_t *request, http_response_t *response,
                hs_req_fn_t http_write) {
  grwprintf_t printctx;
  _grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE, &request->server->memused);
  _http_respond_headers(request, response, &printctx);
  if (response->body) {
    _grwmemcpy(&printctx, response->body, response->content_length);
  }
  _http_end_response(request, response, &printctx, http_write);
}

void hs_respond_chunk(http_request_t *request, http_response_t *response,
                      hs_req_fn_t cb, hs_req_fn_t http_write) {
  grwprintf_t printctx;
  _grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE, &request->server->memused);
  if (!HTTP_FLAG_CHECK(request->flags, HTTP_CHUNKED_RESPONSE)) {
    HTTP_FLAG_SET(request->flags, HTTP_CHUNKED_RESPONSE);
    hs_response_header(response, "Transfer-Encoding", "chunked");
    _http_respond_headers(request, response, &printctx);
  }
  request->chunk_cb = cb;
  _grwprintf(&printctx, "%X\r\n", response->content_length);
  _grwmemcpy(&printctx, response->body, response->content_length);
  _grwprintf(&printctx, "\r\n");
  _http_end_response(request, response, &printctx, http_write);
}

void hs_respond_chunk_end(http_request_t *request, http_response_t *response,
                          hs_req_fn_t http_write) {
  grwprintf_t printctx;
  _grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE, &request->server->memused);
  _grwprintf(&printctx, "0\r\n");
  _http_buffer_headers(request, response, &printctx);
  _grwprintf(&printctx, "\r\n");
  HTTP_FLAG_CLEAR(request->flags, HTTP_CHUNKED_RESPONSE);
  _http_end_response(request, response, &printctx, http_write);
}

void hs_response_status(http_response_t *response, int status) {
  response->status = status > 599 || status < 100 ? 500 : status;
}

void hs_response_body(http_response_t *response, char const *body, int length) {
  response->body = body;
  response->content_length = length;
}

http_response_t *hs_response_init() {
  http_response_t *response =
      (http_response_t *)calloc(1, sizeof(http_response_t));
  assert(response != NULL);
  response->status = 200;
  return response;
}

void hs_respond_error(http_request_t *request, int code, char const *message,
                      hs_req_fn_t http_write) {
  struct http_response_s *response = hs_response_init();
  hs_response_status(response, code);
  hs_response_header(response, "Content-Type", "text/plain");
  hs_response_body(response, message, strlen(message));
  hs_respond(request, response, http_write);
  http_write(request);
}
