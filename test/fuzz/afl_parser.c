#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define HTTPSERVER_IMPL
#include "httpserver.h"

static void fuzz_request_handler(struct http_request_s* request) {
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

int main(int argc, char** argv) {
  char buf[8192];
  ssize_t size;

  if (argc > 1) {
    FILE* f = fopen(argv[1], "r");
    if (!f) return 1;
    size = fread(buf, 1, sizeof(buf), f);
    fclose(f);
  } else {
    size = read(STDIN_FILENO, buf, sizeof(buf));
  }

  if (size <= 0) return 0;

  struct http_server_s* server = http_server_init(8080, fuzz_request_handler);
  if (!server) return 1;

  struct http_request_s request;
  memset(&request, 0, sizeof(request));
  request.server = server;
  request.buffer.buf = buf;
  request.buffer.length = size;
  request.buffer.capacity = size;
  request.buffer.index = 0;
  request.flags = HTTP_AUTOMATIC;

  struct hs_token_array_s tokens = {0};
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