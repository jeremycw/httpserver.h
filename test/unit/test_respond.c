#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "munit.h"

#include "respond.h"
#include "common.h"

static int http_write_call_count = 0;

static void test_http_write(struct http_request_s* request) {
  http_write_call_count++;
}

static struct http_request_s* setup_test_request() {
  struct http_request_s* request = calloc(1, sizeof(struct http_request_s));
  request->server = calloc(1, sizeof(struct http_server_s));

  request->buffer.buf = (char*)calloc(1, 1024);
  request->buffer.capacity = 1024;

  return request;
}

static void destroy_test_request(struct http_request_s* request) {
  free(request->server);
  free(request->buffer.buf);
  free(request->tokens.buf);
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