#include <unistd.h>
#include "munit.h"
#define HTTPSERVER_IMPL
#include "../http_parser.h"
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
  buffer->length = len;
  buffer->sequence_id = 1;
}

static MunitResult test_parser_small_body_complete(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;
  char const* request_string = "GET /foo HTTP/1.1\r\nHost: www.jeremycw.com\r\nContent-Length: 16\r\n\r\naaaaaaaaaaaaaaaa";

  struct hsh_parser_s parser = { 0 };
  struct hsh_buffer_s buffer = { 0 };
  setup_buffer_and_parser(&parser, &buffer, request_string);

  enum hsh_token_e expected_types[] = {
    HSH_TOK_METHOD,
    HSH_TOK_TARGET,
    HSH_TOK_VERSION,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADERS_DONE,
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
    "",
    "aaaaaaaaaaaaaaaa"
  };

  int i = 0;
  struct hsh_token_s out;
  while ((out = hsh_parser_exec(&parser, &buffer, 1024)).type != HSH_TOK_NONE) {
    munit_assert_memory_equal(out.len, expected_values[i], &buffer.buf[out.index]);
    munit_assert(expected_types[i] == out.type);
    i++;
  }

  munit_assert(i == 9);

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

  enum hsh_token_e expected_types[] = {
    HSH_TOK_METHOD,
    HSH_TOK_TARGET,
    HSH_TOK_VERSION,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADERS_DONE
  };

  char const* expected_values[] = {
    "GET",
    "/foo",
    "HTTP/1.1",
    "Host",
    "www.jeremycw.com",
    "Content-Length",
    "16",
    ""
  };

  int i = 0;
  struct hsh_token_s out;
  while ((out = hsh_parser_exec(&parser, &buffer, 1024)).type != HSH_TOK_NONE) {
    munit_assert_memory_equal(out.len, expected_values[i], &buffer.buf[out.index]);
    munit_assert(expected_types[i] == out.type);
    i++;
  }

  munit_assert(i == 8);

  memcpy(buffer.buf + buffer.length, "aaaaaaaa", 8);
  buffer.length += 8;
  buffer.sequence_id++;

  out = hsh_parser_exec(&parser, &buffer, 1024);

  munit_assert(out.type == HSH_TOK_BODY);
  munit_assert_memory_equal(out.len, "aaaaaaaaaaaaaaaa", &buffer.buf[out.index]);

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


  enum hsh_token_e expected_types[] = {
    HSH_TOK_METHOD,
    HSH_TOK_TARGET,
    HSH_TOK_VERSION,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADERS_DONE,
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
    "",
    "aaaaaaaa"
  };

  int i = 0;
  struct hsh_token_s out;
  int max_buf_capacity = strlen(request_string);

  while ((out = hsh_parser_exec(&parser, &buffer, max_buf_capacity)).type != HSH_TOK_NONE) {
    munit_assert_memory_equal(out.len, expected_values[i], &buffer.buf[out.index]);
    munit_assert(expected_types[i] == out.type);
    i++;
  }

  munit_assert(i == 9);

  memcpy(buffer.buf + buffer.length - 8, "bbbbbbbb", 8);
  buffer.sequence_id++;

  out = hsh_parser_exec(&parser, &buffer, max_buf_capacity);

  munit_assert(out.type == HSH_TOK_BODY);
  munit_assert_memory_equal(out.len, "bbbbbbbb", &buffer.buf[out.index]);

  free(buffer.buf);

  return MUNIT_OK;
}

static MunitResult test_parser_chunked_body(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;
  char const* request_string = "POST /chunked/test HTTP/1.0\r\nHost: www.jeremycw.com\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nabcde\r\na\r\n1234567890\r\n0\r\n\r\n";

  struct hsh_parser_s parser = { 0 };
  struct hsh_buffer_s buffer = { 0 };
  setup_buffer_and_parser(&parser, &buffer, request_string);

  enum hsh_token_e expected_types[] = {
    HSH_TOK_METHOD,
    HSH_TOK_TARGET,
    HSH_TOK_VERSION,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADERS_DONE,
    HSH_TOK_BODY,
    HSH_TOK_BODY,
    HSH_TOK_BODY
  };

  char const* expected_values[] = {
    "POST",
    "/chunked/test",
    "HTTP/1.0",
    "Host",
    "www.jeremycw.com",
    "Transfer-Encoding",
    "chunked",
    "",
    "abcde",
    "1234567890",
    ""
  };

  int i = 0;
  struct hsh_token_s out;
  int max_buf_capacity = strlen(request_string);
  while ((out = hsh_parser_exec(&parser, &buffer, max_buf_capacity)).type != HSH_TOK_NONE) {
    munit_assert_memory_equal(out.len, expected_values[i], &buffer.buf[out.index]);
    munit_assert(expected_types[i] == out.type);
    i++;
  }

  munit_assert(i == 11);

  free(buffer.buf);

  return MUNIT_OK;
}

static MunitResult test_parser_chunked_body_partial(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;
  char const* request_string = "POST /chunked/test HTTP/1.0\r\nHost: www.jeremycw.com\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nabcde\r\na\r\n12345678";

  struct hsh_parser_s parser = { 0 };
  struct hsh_buffer_s buffer = { 0 };
  setup_buffer_and_parser(&parser, &buffer, request_string);

  enum hsh_token_e expected_types[] = {
    HSH_TOK_METHOD,
    HSH_TOK_TARGET,
    HSH_TOK_VERSION,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADER_KEY,
    HSH_TOK_HEADER_VALUE,
    HSH_TOK_HEADERS_DONE,
    HSH_TOK_BODY
  };

  char const* expected_values[] = {
    "POST",
    "/chunked/test",
    "HTTP/1.0",
    "Host",
    "www.jeremycw.com",
    "Transfer-Encoding",
    "chunked",
    "",
    "abcde"
  };

  int i = 0;
  struct hsh_token_s out;
  int max_buf_capacity = strlen(request_string);
  while ((out = hsh_parser_exec(&parser, &buffer, max_buf_capacity)).type != HSH_TOK_NONE) {
    munit_assert_memory_equal(out.len, expected_values[i], &buffer.buf[out.index]);
    munit_assert(expected_types[i] == out.type);
    i++;
  }

  munit_assert(i == 9);

  memcpy(buffer.buf + buffer.length, "90\r\n0\r\n\r\n", 9);
  buffer.length += 9;
  buffer.sequence_id++;

  out = hsh_parser_exec(&parser, &buffer, max_buf_capacity);

  munit_assert(out.type == HSH_TOK_BODY);
  munit_assert_memory_equal(out.len, "1234567890", &buffer.buf[out.index]);

  free(buffer.buf);

  return MUNIT_OK;
}

// static MunitResult test_server_read_socket(const MunitParameter params[], void* data) {
//   (void)params;
//   (void)data;
// 
//   struct hsh_buffer_s buffer = { };
//   int64_t memused = 0;
//   struct hs_read_socket_options_s opts = { 32, 1024 };
// 
//   int fd = openat(AT_FDCWD, "test/test_server_read_socket.txt", O_RDWR);
//   munit_assert(fd >= 0);
// 
//   int bytes = hs_read_socket(&buffer, fd, &memused, opts);
//   close(fd);
// 
//   printf("read: %d\n", bytes);
// 
//   return MUNIT_OK;
// }

static MunitTest tests[] = {
  // definition order: test-name, test-func, setup-func, teardown-func, options, params
  { (char*) "/parser/small_body/complete", test_parser_small_body_complete, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/parser/small_body/partial", test_parser_small_body_partial, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/parser/large_body", test_parser_large_body, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/parser/chunked_body", test_parser_chunked_body, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/parser/chunked_body/partial", test_parser_chunked_body_partial, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  // { (char*) "/server/read_socket", test_server_read_socket, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
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
