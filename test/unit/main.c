#include <stddef.h>

#include "munit.h"
#include "test_parser.h"
#include "test_read_socket.h"
#include "test_write_socket.h"
#include "test_respond.h"
#include "test_request_util.h"

static MunitTest tests[] = {
  {(char*)"/parser/small_body/complete", test_parser_small_body_complete, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/parser/small_body/partial", test_parser_small_body_partial, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/parser/large_body", test_parser_large_body, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/parser/chunked_body", test_parser_chunked_body, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/parser/chunked_body/partial", test_parser_chunked_body_partial, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/read_socket/small_body", test_read_socket_small_body, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/read_socket/small_body/expand_buffer", test_read_socket_small_body_expand_buffer, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/read_socket/large_body", test_read_socket_large_body, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/write_socket/partial", test_write_socket_partial, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/respond/error_single_write", test_respond_error_single_write, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/respond/error_status_range", test_respond_error_status_range, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/request_util/keep_alive_http_1_1", test_request_keep_alive_http_1_1, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {(char*)"/request_util/keep_alive_http_1_0", test_request_keep_alive_http_1_0, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
  {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}
};

static const MunitSuite test_suite = {
  (char*)"/hs",
  tests,
  NULL,
  1,
  MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char** argv) {
  return munit_suite_main(&test_suite, (void*)"httpserver.h", argc, argv);
}