#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define HTTPSERVER_IMPL
#include "httpserver.h"

static int headers_callback_called = 0;

static void fuzz_request_handler(struct http_request_s* request) {
  headers_callback_called = 1;

  http_request_method(request);
  http_request_target(request);
  http_request_body(request);

  http_string_t host = http_request_header(request, "Host");
  (void)host;

  int iter = 0;
  http_string_t key, val;
  while (http_request_iterate_headers(request, &key, &val, &iter)) {
    (void)key;
    (void)val;
  }
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  headers_callback_called = 0;

  struct http_server_s* server = http_server_init(8080, fuzz_request_handler);
  if (!server) return 0;

  struct http_request_s request;
  memset(&request, 0, sizeof(request));
  request.server = server;
  request.buffer.buf = (char*)(uintptr_t)data;
  request.buffer.length = size;
  request.buffer.capacity = size;
  request.buffer.index = 0;
  request.flags = HTTP_AUTOMATIC;
  request.tokens.size = 0;

  struct hs_token_array_s tokens;
  tokens.buf = NULL;
  tokens.size = 0;
  tokens.capacity = 0;
  request.tokens = tokens;

  hsh_parser_init(&request.parser);

  struct hs_read_opts_s opts = {
    .max_request_buf_capacity = 8192,
    .initial_request_buf_capacity = 1024,
    .eof_rc = -1
  };

  request.socket = -1;
  hs_read_request_and_exec_user_cb(&request, opts);

  if (request.tokens.buf) {
    free(request.tokens.buf);
  }

  free(server);
  return 0;
}