#include <stddef.h>

#include "munit.h"
#include "test_parser.h"
#include "test_read_socket.h"
#include "test_write_socket.h"

static MunitTest tests[] = {
  // definition order: test-name, test-func, setup-func, teardown-func, options, params
  { (char*) "/parser/small_body/complete", test_parser_small_body_complete, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/parser/small_body/partial", test_parser_small_body_partial, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/parser/large_body", test_parser_large_body, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/parser/chunked_body", test_parser_chunked_body, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/parser/chunked_body/partial", test_parser_chunked_body_partial, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/read_socket/small_body", test_read_socket_small_body, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/read_socket/small_body/expand_buffer", test_read_socket_small_body_expand_buffer, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/read_socket/large_body", test_read_socket_large_body, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  { (char*) "/write_socket/partial", test_write_socket_partial, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
  // end
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite test_suite = {
  (char*) "/hs",         // Test suite name prefix
  tests,                  // Tests
  NULL,                   // Sub test suites
  1,                      // Execution mode (normal)
  MUNIT_SUITE_OPTION_NONE // Options
};

int main(int argc, char** argv) {
  return munit_suite_main(&test_suite, (void*) "httpserver.h", argc, argv);
}
