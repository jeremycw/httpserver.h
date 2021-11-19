#include <unistd.h>
#include "munit.h"
#include "../http_parser.c"
#include "debugbreak.h"

static void setup_buffer_and_parser(struct hsh_parser_s* parser, struct hsh_buffer_s* buffer, char const* req_str) {
  // setup parser
  hsh_parser_init(parser);

  // setup input buffer
  int len = strlen(req_str);
  buffer->buf = malloc(1024);
  buffer->capacity = 1024;
  memcpy(buffer->buf, req_str, len);
  buffer->size = len;
}

static MunitResult test_parser_small_body_complete(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;
  char const* request_string = "GET /foo HTTP/1.1\r\nHost: www.jeremycw.com\r\nContent-Length: 16\r\n\r\naaaaaaaaaaaaaaaa";

  struct hsh_parser_s parser = { 0 };
  struct hsh_buffer_s buffer = { 0 };
  setup_buffer_and_parser(&parser, &buffer, request_string);

  struct hsh_parser_return_s out = hsh_parser_exec(&parser, &buffer, 1024);

  munit_assert(out.rc == HSH_PARSER_REQ_READY);

  enum hsh_token_e expected_types[] = {
    HSH_TOK_METHOD,
    HSH_TOK_TARGET,
    HSH_TOK_VERSION,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_BODY
  };

  char const* expected_values[] = {
    "GET",
    "/foo",
    "HTTP/1.1",
    "Host",
    "www.jeremycw.com",
    "Content-Length",
    "16",
    "aaaaaaaaaaaaaaaa"
  };

  munit_assert(out.tokens_n > 0);
  for (int i = 0; i < out.tokens_n; i++) {
    munit_assert_memory_equal(out.tokens[i].len, expected_values[i], &buffer.buf[out.tokens[i].index]);
    munit_assert(expected_types[i] == out.tokens[i].type);
  }

  free(buffer.buf);

  return MUNIT_OK;
}

static MunitResult test_parser_small_body_partial(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;
  char const* request_string = "GET /foo HTTP/1.1\r\nHost: www.jeremycw.com\r\nContent-Length: 16\r\n\r\naaaaaaaa";

  struct hsh_parser_s parser = { 0 };
  struct hsh_buffer_s buffer = { 0 };
  setup_buffer_and_parser(&parser, &buffer, request_string);

  struct hsh_parser_return_s out = hsh_parser_exec(&parser, &buffer, 1024);

  munit_assert(out.rc == HSH_PARSER_CONTINUE);
  memcpy(buffer.buf + buffer.size, "aaaaaaaa", 8);
  buffer.size += 8;

  out = hsh_parser_exec(&parser, &buffer, 1024);

  munit_assert(out.rc == HSH_PARSER_REQ_READY);

  enum hsh_token_e expected_types[] = {
    HSH_TOK_METHOD,
    HSH_TOK_TARGET,
    HSH_TOK_VERSION,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_BODY
  };

  char const* expected_values[] = {
    "GET",
    "/foo",
    "HTTP/1.1",
    "Host",
    "www.jeremycw.com",
    "Content-Length",
    "16",
    "aaaaaaaaaaaaaaaa"
  };

  munit_assert(out.tokens_n > 0);
  for (int i = 0; i < out.tokens_n; i++) {
    munit_assert_memory_equal(out.tokens[i].len, expected_values[i], &buffer.buf[out.tokens[i].index]);
    munit_assert(expected_types[i] == out.tokens[i].type);
  }

  free(buffer.buf);

  return MUNIT_OK;
}

static MunitResult test_parser_large_body(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;
  char const* request_string = "GET /foo HTTP/1.1\r\nHost: www.jeremycw.com\r\nContent-Length: 16\r\n\r\naaaaaaaa";

  struct hsh_parser_s parser = { 0 };
  struct hsh_buffer_s buffer = { 0 };
  setup_buffer_and_parser(&parser, &buffer, request_string);

  int max_buf_capacity = strlen(request_string);
  struct hsh_parser_return_s out = hsh_parser_exec(&parser, &buffer, max_buf_capacity);

  munit_assert(out.rc == HSH_PARSER_REQ_READY);

  enum hsh_token_e expected_types[] = {
    HSH_TOK_METHOD,
    HSH_TOK_TARGET,
    HSH_TOK_VERSION,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE
  };

  char const* expected_values[] = {
    "GET",
    "/foo",
    "HTTP/1.1",
    "Host",
    "www.jeremycw.com",
    "Content-Length",
    "16"
  };

  munit_assert(HSH_FLAG_CHECK(out.flags, HSH_FLAG_STREAMED));

  munit_assert(out.tokens_n > 0);
  for (int i = 0; i < out.tokens_n; i++) {
    munit_assert_memory_equal(out.tokens[i].len, expected_values[i], &buffer.buf[out.tokens[i].index]);
    munit_assert(expected_types[i] == out.tokens[i].type);
  }

  out = hsh_parser_exec(&parser, &buffer, max_buf_capacity);

  munit_assert(out.rc == HSH_PARSER_BODY_READY);

  struct hsh_token_s body = out.tokens[out.tokens_n - 1];
  munit_assert(body.type == HSH_TOK_BODY);
  munit_assert_memory_equal(body.len, "aaaaaaaa", &buffer.buf[body.index]);

  memcpy(buffer.buf + buffer.size - 8, "bbbbbbbb", 8);

  out = hsh_parser_exec(&parser, &buffer, max_buf_capacity);

  munit_assert(out.rc == HSH_PARSER_BODY_FINAL);

  body = out.tokens[out.tokens_n - 1];
  munit_assert(body.type == HSH_TOK_BODY);
  munit_assert_memory_equal(body.len, "bbbbbbbb", &buffer.buf[body.index]);

  free(buffer.buf);

  return MUNIT_OK;
}

static MunitTest tests[] = {
  // definition order: test-name, test-func, setup-func, teardown-func, options, params
  { (char*) "/parser/small_body/complete", test_parser_small_body_complete, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/parser/small_body/partial", test_parser_small_body_partial, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/parser/large_body", test_parser_large_body, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  // end
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite test_suite = {
  (char*) "/hsh",         // Test suite name prefix
  tests,                  // Tests
  NULL,                   // Sub test suites
  1,                      // Execution mode (normal)
  MUNIT_SUITE_OPTION_NONE // Options
};

int main(int argc, char** argv) {
  return munit_suite_main(&test_suite, (void*) "httpserver.h", argc, argv);
}
