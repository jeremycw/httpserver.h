#include <unistd.h>
#include "munit.h"
#include "../http_parser.c"

#define HTTP_REQUEST "GET /foo HTTP/1.1\r\nHost: www.jeremycw.com\r\nContent-Length: 16\r\n\r\naaaaaaaa"

static MunitResult test_http_parser(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct hsh_parser_s parser = { 0 };
  struct hsh_buffer_s buffer = { 0 };
  parser.state = hsh_http_start;
  buffer.buf = malloc(1024);
  buffer.capacity = sizeof(HTTP_REQUEST);
  memcpy(buffer.buf, HTTP_REQUEST, sizeof(HTTP_REQUEST));
  buffer.size = sizeof(HTTP_REQUEST) - 1;
  hsh_parse(&parser, &buffer, test_req_cb, test_body_cb, test_error_cb, NULL);

  munit_assert(1 == 1);

  return MUNIT_OK;
}

static MunitTest tests[] = {
  // definition order: test-name, test-func, setup-func, teardown-func, options, params
  { (char*) "/http/parser", test_http_parser, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  // end
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite test_suite = {
  (char*) "",             // Test suite name prefix
  tests,                  // Tests
  NULL,                   // Sub test suites
  1,                      // Execution mode (normal)
  MUNIT_SUITE_OPTION_NONE // Options
};

int main(int argc, char** argv) {
  return munit_suite_main(&test_suite, (void*) "httpserver.h", argc, argv);
}
