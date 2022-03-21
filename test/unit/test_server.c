#include <fcntl.h>
#include <unistd.h>
#include "munit.h"

#include "../../src/server.h"
#include "../../src/data.h"
#include "../../src/lib.h"
#include "../debugbreak.h"

MunitResult test_read_socket_small_body(const MunitParameter params[], void* data) {
  struct hs_server_process_read_ready_s input = { 0 };
  struct hsh_parser_s parser;
  struct hsh_buffer_s buffer;
  struct hs_token_array_s tokens;

  input.parser = &parser;
  input.buffer = &buffer;
  input.tokens = &tokens;
  input.max_request_buf_capacity = 4096;
  input.initial_request_buf_capacity = 1024;
  input.server_memused = 1024;
  input.eof_rc = -1;

  hsh_buffer_init(input.buffer, input.initial_request_buf_capacity);
  hsh_parser_init(input.parser);
  hs_token_array_init(input.tokens, 2);

  int fd = openat(AT_FDCWD, "test/unit/read_socket.txt", O_RDONLY);
  input.request_socket = fd;

  hs_server_process_read_ready(&input);

  munit_assert(input.request_timeout == HTTP_REQUEST_TIMEOUT);
  munit_assert(input.server_memused == 1024);
  munit_assert(input.after_event == HS_EVT_REQUEST_CALLBACK);

  close(fd);

  return MUNIT_OK;
}
