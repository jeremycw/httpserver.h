#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#define HTTPSERVER_IMPL
#include "httpserver.h"

static void fuzz_request_handler(struct http_request_s* request) {
  http_request_method(request);
  http_request_target(request);
  http_request_body(request);
}

static char random_char() {
  const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\r\n: /-\r\n\r\n";
  return chars[rand() % strlen(chars)];
}

static void generate_random_http(char* buf, size_t* size, size_t max_size) {
  const char* methods[] = {"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH"};
  const char* targets[] = {"/", "/foo", "/bar/baz", "/api/v1/users", "/long/path/here/with/more/stuff"};

  const char* method = methods[rand() % (sizeof(methods) / sizeof(methods[0]))];
  const char* target = targets[rand() % (sizeof(targets) / sizeof(targets[0]))];

  int offset = sprintf(buf, "%s %s HTTP/1.%d\r\n", method, target, rand() % 2);

  int num_headers = rand() % 5;
  for (int i = 0; i < num_headers && offset < max_size - 50; i++) {
    const char* headers[] = {"Host", "Content-Type", "Content-Length", "Accept", "User-Agent", "Connection"};
    const char* header = headers[rand() % (sizeof(headers) / sizeof(headers[0]))];
    const char* values[] = {"localhost:8080", "application/json", "text/html", "*/*", "test", "close", "keep-alive"};
    const char* value = values[rand() % (sizeof(values) / sizeof(values[0]))];

    offset += snprintf(buf + offset, max_size - offset, "%s: %s\r\n", header, value);
  }

  offset += snprintf(buf + offset, max_size - offset, "\r\n");

  if (rand() % 2 && strcmp(method, "GET") != 0) {
    int body_len = rand() % 100;
    offset += snprintf(buf + offset, max_size - offset, "Body goes here: %d bytes\r\n", body_len);
  }

  *size = offset;
}

int main(int argc, char** argv) {
  srand(argc > 1 ? atoi(argv[1]) : time(NULL));

  struct http_server_s* server = http_server_init(8080, fuzz_request_handler);
  if (!server) {
    fprintf(stderr, "Failed to create server\n");
    return 1;
  }

  printf("Running fuzz tests...\n");

  for (int i = 0; i < 10000; i++) {
    char buf[4096];
    size_t size;
    generate_random_http(buf, &size, sizeof(buf));

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
      .max_request_buf_capacity = 4096,
      .initial_request_buf_capacity = 1024,
      .eof_rc = -1
    };

    request.socket = -1;
    hs_read_request_and_exec_user_cb(&request, opts);

    if (request.tokens.buf) {
      free(request.tokens.buf);
    }

    if (i % 1000 == 0) {
      printf("  Completed %d tests...\n", i);
    }
  }

  printf("Fuzz tests completed successfully!\n");

  free(server);
  return 0;
}