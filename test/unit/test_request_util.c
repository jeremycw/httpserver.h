#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "munit.h"

#include "request_util.h"
#include "common.h"

static void token_array_init(struct hs_token_array_s *array, int capacity) {
  array->buf = (struct hsh_token_s*)malloc(sizeof(struct hsh_token_s) * capacity);
  array->capacity = capacity;
}

static struct http_request_s* setup_test_request_with_version(const char* version) {
  struct http_request_s* request = calloc(1, sizeof(struct http_request_s));
  request->server = calloc(1, sizeof(struct http_server_s));

  int version_len = strlen(version);
  request->buffer.buf = (char*)calloc(1, 256);
  request->buffer.capacity = 256;
  memcpy(request->buffer.buf, version, version_len);
  request->buffer.length = version_len;

  token_array_init(&request->tokens, 32);

  struct hsh_token_s tok = {
    .type = HSH_TOK_VERSION,
    .index = 0,
    .len = version_len
  };
  request->tokens.buf[0] = tok;
  request->tokens.size = 1;

  return request;
}

static void destroy_test_request(struct http_request_s* request) {
  free(request->server);
  free(request->buffer.buf);
  free(request->tokens.buf);
  free(request);
}

MunitResult test_request_keep_alive_http_1_1(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct http_request_s* request = setup_test_request_with_version("HTTP/1.1");
  request->flags = HTTP_AUTOMATIC;

  hs_request_detect_keep_alive_flag(request);

  munit_assert_true(HTTP_FLAG_CHECK(request->flags, HTTP_KEEP_ALIVE));

  destroy_test_request(request);
  return MUNIT_OK;
}

MunitResult test_request_keep_alive_http_1_0(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct http_request_s* request = setup_test_request_with_version("HTTP/1.0");
  request->flags = HTTP_AUTOMATIC;

  hs_request_detect_keep_alive_flag(request);

  munit_assert_false(HTTP_FLAG_CHECK(request->flags, HTTP_KEEP_ALIVE));

  destroy_test_request(request);
  return MUNIT_OK;
}

MunitResult test_request_keep_alive_connection_close(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct http_request_s* request = setup_test_request_with_version("HTTP/1.1");
  request->flags = HTTP_AUTOMATIC;

  char* buf = (char*)calloc(1, 256);
  memcpy(buf, "HTTP/1.1", 8);
  memcpy(buf + 8, "\r\nConnection: close\r\n\r\n", 23);
  free(request->buffer.buf);
  request->buffer.buf = buf;
  request->buffer.capacity = 256;
  request->buffer.length = 31;

  struct hsh_token_s tok = {
    .type = HSH_TOK_HEADER_KEY,
    .index = 10,
    .len = 10
  };
  request->tokens.buf[1] = tok;
  request->tokens.size = 2;

  hs_request_detect_keep_alive_flag(request);

  munit_assert_false(HTTP_FLAG_CHECK(request->flags, HTTP_KEEP_ALIVE));

  destroy_test_request(request);
  return MUNIT_OK;
}