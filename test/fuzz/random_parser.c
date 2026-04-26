#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>

#define HTTPSERVER_IMPL
#include "httpserver.h"

static int total_tests = 0;
static int error_count = 0;

static void fuzz_request_handler(struct http_request_s* request) {
  http_request_method(request);
  http_request_target(request);
  http_request_body(request);
}

enum mutation_category {
  MUT_VALID,
  MUT_METHOD_BOUNDARY,
  MUT_INVALID_VERSION,
  MUT_CHUNKED_EDGE,
  MUT_CONTENT_LENGTH_EXTREME,
  MUT_MALFORMED_HEADERS,
  MUT_BUFFER_UNDERFLOW,
  MUT_EOF_MIDSTREAM,
};

static const char* category_name(enum mutation_category cat) {
  switch (cat) {
    case MUT_VALID: return "valid";
    case MUT_METHOD_BOUNDARY: return "method_boundary";
    case MUT_INVALID_VERSION: return "invalid_version";
    case MUT_CHUNKED_EDGE: return "chunked_edge";
    case MUT_CONTENT_LENGTH_EXTREME: return "content_length_extreme";
    case MUT_MALFORMED_HEADERS: return "malformed_headers";
    case MUT_BUFFER_UNDERFLOW: return "buffer_underflow";
    case MUT_EOF_MIDSTREAM: return "eof_midstream";
    default: return "unknown";
  }
}

static char random_printable() {
  const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 \r\n\t:/-";
  return chars[rand() % strlen(chars)];
}

static void mutate_bitflip(char* buf, size_t size) {
  if (size == 0) return;
  int flips = 1 + rand() % 3;
  for (int i = 0; i < flips; i++) {
    size_t pos = rand() % size;
    buf[pos] ^= (1 << (rand() % 8));
  }
}

static void mutate_delete(char* buf, size_t* size) {
  if (*size < 2) return;
  int dels = 1 + rand() % 3;
  for (int i = 0; i < dels && *size > 1; i++) {
    size_t pos = rand() % (*size);
    memmove(buf + pos, buf + pos + 1, *size - pos - 1);
    (*size)--;
  }
}

static void mutate_insert(char* buf, size_t* size, size_t max_size) {
  if (*size >= max_size - 1) return;
  int ins = 1 + rand() % 2;
  for (int i = 0; i < ins && *size < max_size - 1; i++) {
    size_t pos = rand() % (*size + 1);
    memmove(buf + pos + 1, buf + pos, *size - pos);
    buf[pos] = random_printable();
    (*size)++;
  }
}

static void mutate_truncate(char* buf, size_t* size) {
  if (*size < 10) return;
  size_t new_size = (*size) * (rand() % 4) / 4;
  if (new_size < 10) new_size = 10;
  *size = new_size;
}

static void generate_valid_http(char* buf, size_t* size, size_t max_size) {
  const char* methods[] = {"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH"};
  const char* targets[] = {"/", "/foo", "/bar", "/api/test", "/path/to/resource"};

  const char* method = methods[rand() % (sizeof(methods) / sizeof(methods[0]))];
  const char* target = targets[rand() % (sizeof(targets) / sizeof(targets[0]))];

  int offset = sprintf(buf, "%s %s HTTP/1.%d\r\n", method, target, rand() % 2);

  int num_headers = rand() % 4;
  for (int i = 0; i < num_headers && offset < max_size - 60; i++) {
    const char* headers[] = {"Host", "Content-Type", "Content-Length", "Accept", "User-Agent", "Connection"};
    const char* header = headers[rand() % (sizeof(headers) / sizeof(headers[0]))];
    const char* values[] = {"localhost:8080", "application/json", "text/html", "*/*", "test", "close", "keep-alive"};
    const char* value = values[rand() % (sizeof(values) / sizeof(values[0]))];

    offset += snprintf(buf + offset, max_size - offset, "%s: %s\r\n", header, value);
  }

  offset += snprintf(buf + offset, max_size - offset, "\r\n");

  if (rand() % 2 && strcmp(method, "GET") != 0) {
    int body_len = rand() % 50;
    offset += snprintf(buf + offset, max_size - offset, "Body: %d\r\n", body_len);
  }

  *size = offset;
}

static void generate_method_boundary(char* buf, size_t* size, size_t max_size) {
  int len = 20 + rand() % 15;
  for (int i = 0; i < len; i++) {
    buf[i] = random_printable();
  }
  buf[len] = ' ';
  strcpy(buf + len + 1, "/ HTTP/1.1\r\n\r\n");
  *size = len + 14;
}

static void generate_invalid_version(char* buf, size_t* size, size_t max_size) {
  (void)max_size;
  int ver = rand() % 20;
  sprintf(buf, "GET / HTTP/1.%d\r\n\r\n", ver);
  *size = strlen(buf);
}

static void generate_chunked_edge(char* buf, size_t* size, size_t max_size) {
  int offset = sprintf(buf, "POST / HTTP/1.1\r\n");
  offset += snprintf(buf + offset, max_size - offset, "Transfer-Encoding: chunked\r\n\r\n");

  int num_chunks = 1 + rand() % 4;
  for (int i = 0; i < num_chunks && offset < max_size - 50; i++) {
    int chunk_size = rand() % 20;
    offset += snprintf(buf + offset, max_size - offset, "%x\r\n", chunk_size);
    for (int j = 0; j < chunk_size && offset < max_size - 10; j++) {
      buf[offset++] = random_printable();
    }
    if (offset < max_size - 10) {
      offset += snprintf(buf + offset, max_size - offset, "\r\n");
    }
  }

  if (offset < max_size - 10) {
    offset += snprintf(buf + offset, max_size - offset, "0\r\n\r\n");
  }

  *size = offset;
}

static void generate_content_length_extreme(char* buf, size_t* size, size_t max_size) {
  int big_len = (rand() % 2 == 0) ? 10000 : (rand() % 5000);
  int offset = sprintf(buf, "POST / HTTP/1.1\r\n");
  offset += snprintf(buf + offset, max_size - offset, "Content-Length: %d\r\n\r\n", big_len);

  int body_size = (big_len > 100) ? 50 : big_len;
  for (int i = 0; i < body_size; i++) {
    buf[offset++] = random_printable();
  }

  *size = offset;
}

static void generate_malformed_headers(char* buf, size_t* size, size_t max_size) {
  int offset = sprintf(buf, "GET / HTTP/1.1\r\n");

  switch (rand() % 6) {
    case 0: offset += snprintf(buf + offset, max_size - offset, "Host:\r\n"); break;
    case 1: offset += snprintf(buf + offset, max_size - offset, "Host\r\n"); break;
    case 2: offset += snprintf(buf + offset, max_size - offset, "Host: \r\n"); break;
    case 3: offset += snprintf(buf + offset, max_size - offset, "Host\tlocalhost\r\n"); break;
    case 4: offset += snprintf(buf + offset, max_size - offset, "Host: localhost\n"); break;
    case 5: offset += snprintf(buf + offset, max_size - offset, ": localhost\r\n"); break;
  }

  offset += snprintf(buf + offset, max_size - offset, "\r\n");
  *size = offset;
}

static void generate_buffer_underflow(char* buf, size_t* size, size_t max_size) {
  (void)max_size;
  int offset = sprintf(buf, "GET / HTTP/1.1\r\n");

  switch (rand() % 4) {
    case 0: offset += snprintf(buf + offset, 100, "Host: local"); break;
    case 1: offset += snprintf(buf + offset, 100, "Content-Length: 10"); break;
    case 2: offset += snprintf(buf + offset, 100, "POST"); break;
    case 3: offset += snprintf(buf + offset, 100, "HTTP/1.1\r\n"); break;
  }

  *size = offset;
}

static void generate_eof_midstream(char* buf, size_t* size, size_t max_size) {
  int offset = sprintf(buf, "POST / HTTP/1.1\r\n");
  offset += snprintf(buf + offset, max_size - offset, "Content-Length: 100\r\n\r\n");

  int partial_body = 10 + rand() % 40;
  for (int i = 0; i < partial_body; i++) {
    buf[offset++] = random_printable();
  }

  *size = offset;
}

static void run_fuzz_test(struct http_server_s* server, char* buf, size_t size, enum mutation_category cat) {
  total_tests++;

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
}

static void fuzz_category(struct http_server_s* server, enum mutation_category cat, int count) {
  char buf[8192];
  size_t size;

  for (int i = 0; i < count; i++) {
    memset(buf, 0, sizeof(buf));
    size = 0;

    switch (cat) {
      case MUT_VALID:
        generate_valid_http(buf, &size, sizeof(buf));
        if (rand() % 2) mutate_bitflip(buf, size);
        break;
      case MUT_METHOD_BOUNDARY:
        generate_method_boundary(buf, &size, sizeof(buf));
        mutate_bitflip(buf, size);
        break;
      case MUT_INVALID_VERSION:
        generate_invalid_version(buf, &size, sizeof(buf));
        mutate_bitflip(buf, size);
        break;
      case MUT_CHUNKED_EDGE:
        generate_chunked_edge(buf, &size, sizeof(buf));
        if (rand() % 3 == 0) mutate_truncate(buf, &size);
        break;
      case MUT_CONTENT_LENGTH_EXTREME:
        generate_content_length_extreme(buf, &size, sizeof(buf));
        if (rand() % 2) mutate_delete(buf, &size);
        break;
      case MUT_MALFORMED_HEADERS:
        generate_malformed_headers(buf, &size, sizeof(buf));
        mutate_bitflip(buf, size);
        break;
      case MUT_BUFFER_UNDERFLOW:
        generate_buffer_underflow(buf, &size, sizeof(buf));
        break;
      case MUT_EOF_MIDSTREAM:
        generate_eof_midstream(buf, &size, sizeof(buf));
        break;
      default:
        continue;
    }

    if (size > 0 && size < sizeof(buf)) {
      run_fuzz_test(server, buf, size, cat);
    }
  }
}

int main(int argc, char** argv) {
  unsigned int seed = (argc > 1) ? atoi(argv[1]) : time(NULL);
  srand(seed);
  printf("Fuzz seed: %u\n", seed);

  struct http_server_s* server = http_server_init(8080, fuzz_request_handler);
  if (!server) {
    fprintf(stderr, "Failed to create server\n");
    return 1;
  }

  printf("Running enhanced fuzz tests...\n");

  int tests_per_category = 500;

  struct {
    enum mutation_category cat;
    const char* name;
  } categories[] = {
    { MUT_VALID, "valid" },
    { MUT_METHOD_BOUNDARY, "method_boundary" },
    { MUT_INVALID_VERSION, "invalid_version" },
    { MUT_CHUNKED_EDGE, "chunked_edge" },
    { MUT_CONTENT_LENGTH_EXTREME, "content_length_extreme" },
    { MUT_MALFORMED_HEADERS, "malformed_headers" },
    { MUT_BUFFER_UNDERFLOW, "buffer_underflow" },
    { MUT_EOF_MIDSTREAM, "eof_midstream" },
  };

  int num_categories = sizeof(categories) / sizeof(categories[0]);

  for (int c = 0; c < num_categories; c++) {
    printf("  Testing %s (%d tests)...\n", categories[c].name, tests_per_category);
    fuzz_category(server, categories[c].cat, tests_per_category);
  }

  printf("\n=== Fuzz Results ===\n");
  printf("Total tests: %d\n", total_tests);
  printf("Error count: %d\n", error_count);
  printf("===================\n");

  free(server);
  return 0;
}