#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "munit.h"

#include "respond.h"
#include "common.h"
#include "io_events.h"
#include "test_write_socket.h"

static int http_write_call_count = 0;

static void test_http_write(struct http_request_s* request) {
  http_write_call_count++;
  if (hs_test_write_mode == HS_TEST_WRITE_CAPTURE) {
    hs_test_enable_write_stub(1);
    hs_test_write(request->socket, request->buffer.buf, request->buffer.length);
  }
}

static struct http_request_s* setup_test_request(void) {
  struct http_request_s* request = calloc(1, sizeof(struct http_request_s));
  request->server = calloc(1, sizeof(struct http_server_s));

  request->buffer.buf = (char*)calloc(1, 1024);
  request->buffer.capacity = 1024;

  return request;
}

static void destroy_test_request(struct http_request_s* request) {
  if (request->buffer.buf) {
    free(request->buffer.buf);
    request->buffer.buf = NULL;
  }
  if (request->tokens.buf) {
    free(request->tokens.buf);
    request->tokens.buf = NULL;
  }
  free(request->server);
  request->server = NULL;
  free(request);
}

MunitResult test_respond_error_single_write(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct http_request_s* request = setup_test_request();

  http_write_call_count = 0;
  hs_request_respond_error(request, 400, "Bad Request", test_http_write);

  munit_assert_int(http_write_call_count, ==, 1);

  destroy_test_request(request);

  return MUNIT_OK;
}

MunitResult test_respond_error_status_range(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct http_request_s* request = setup_test_request();
  struct http_response_s* response = hs_response_init();

  hs_response_set_status(response, 99);
  munit_assert_int(response->status, ==, 500);

  hs_response_set_status(response, 100);
  munit_assert_int(response->status, ==, 100);

  hs_response_set_status(response, 199);
  munit_assert_int(response->status, ==, 199);

  hs_response_set_status(response, 200);
  munit_assert_int(response->status, ==, 200);

  hs_response_set_status(response, 599);
  munit_assert_int(response->status, ==, 599);

  hs_response_set_status(response, 600);
  munit_assert_int(response->status, ==, 500);

  hs_response_set_status(response, 1000);
  munit_assert_int(response->status, ==, 500);

  hs_response_set_status(response, -1);
  munit_assert_int(response->status, ==, 500);

  free(response);
  destroy_test_request(request);

  return MUNIT_OK;
}

MunitResult test_respond_large_body_capture(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct http_request_s* request = setup_test_request();
  request->server->memused = 0;
  strncpy(request->server->date, "Sun, 01 Jan 2026 00:00:00 GMT", 31);

  hs_test_reset_capture();
  hs_test_enable_write_stub(1);
  hs_test_write_mode = HS_TEST_WRITE_CAPTURE;

  struct http_response_s* response = hs_response_init();
  hs_response_set_status(response, 200);
  hs_response_set_header(response, "Content-Type", "text/plain");

  char large_body[2048];
  memset(large_body, 'X', 2048);
  hs_response_set_body(response, large_body, 2048);

  hs_request_respond(request, response, test_http_write);

  char* headers_end = strstr(captured_write_buf, "\r\n\r\n");
  munit_assert_ptr_not_null(headers_end);

  char* body = headers_end + 4;
  size_t body_len = captured_write_size - (body - captured_write_buf);

  munit_assert_size(body_len, ==, 2048);
  munit_assert_memory_equal(2048, large_body, body);

  hs_test_enable_write_stub(0);
  hs_test_reset_capture();
  destroy_test_request(request);

  return MUNIT_OK;
}

MunitResult test_respond_grwprintf_truncation_bug(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct http_request_s* request = setup_test_request();
  request->server->memused = 0;
  strncpy(request->server->date, "Sun, 01 Jan 2026 00:00:00 GMT", 31);

  hs_test_reset_capture();
  hs_test_enable_write_stub(1);
  hs_test_write_mode = HS_TEST_WRITE_CAPTURE;

  struct http_response_s* response = hs_response_init();
  hs_response_set_status(response, 200);
  hs_response_set_header(response, "Content-Type", "text/plain");

  for (int i = 0; i < 20; i++) {
    char key[32];
    char val[128];
    snprintf(key, sizeof(key), "X-Header-%d", i);
    snprintf(val, sizeof(val), "Lorem ipsum dolor sit amet consectetur adipiscing elit %d", i);
    hs_response_set_header(response, key, val);
  }

  char body[64];
  memset(body, 'Y', 64);
  hs_response_set_body(response, body, 64);

  hs_request_respond(request, response, test_http_write);

  munit_assert_ptr_not_null(captured_write_buf);
  munit_assert_size(captured_write_size, >, 0);

  char* headers_end = NULL;
  for (size_t i = 0; i + 3 < captured_write_size; i++) {
    if (captured_write_buf[i] == '\r' && captured_write_buf[i+1] == '\n' &&
        captured_write_buf[i+2] == '\r' && captured_write_buf[i+3] == '\n') {
      headers_end = &captured_write_buf[i];
      break;
    }
  }
  munit_assert_ptr_not_null(headers_end);

  char* body_start = headers_end + 4;
  size_t body_size = captured_write_size - (body_start - captured_write_buf);

  munit_assert_size(body_size, ==, 64);
  munit_assert_memory_equal(64, body, body_start);

  hs_test_enable_write_stub(0);
  hs_test_reset_capture();
  destroy_test_request(request);

  return MUNIT_OK;
}