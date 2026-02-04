#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "munit.h"

#include "read_socket.h"
#include "common.h"

void token_array_init(struct hs_token_array_s *array, int capacity) {
  array->buf =
      (struct hsh_token_s *)malloc(sizeof(struct hsh_token_s) * capacity);
  array->capacity = capacity;
}

void test_callback(struct http_request_s* request) {
  *((int*)request->data) = 1;
}

struct http_request_s* setup_test_request() {
  struct http_request_s* request = calloc(1, sizeof(struct http_request_s));
  request->server = calloc(1, sizeof(struct http_server_s));

  token_array_init(&request->tokens, 32);

  request->server->request_handler = &test_callback;

  return request;
}

void destroy_test_request(struct http_request_s* request) {
  free(request->server);
  free(request->buffer.buf);
  free(request->tokens.buf);
  free(request);
}

MunitResult test_read_socket_small_body(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct http_request_s* request = setup_test_request();

  int callback_ran = 0;
  request->data = (void*)&callback_ran;

  struct hs_read_opts_s opts = {
    .max_request_buf_capacity = 4096,
    .initial_request_buf_capacity = 1024,
    .eof_rc = -1
  };

  int fd = openat(AT_FDCWD, "test/unit/read_socket.txt", O_RDONLY);
  request->socket = fd;

  hs_read_request_and_exec_user_cb(request, opts);

  munit_assert_int(request->timeout, ==, HTTP_REQUEST_TIMEOUT);
  munit_assert_int64(request->server->memused, ==, 1024);
  munit_assert(callback_ran);

  struct hsh_token_s tok = request->tokens.buf[request->tokens.size-1];

  munit_assert_memory_equal(tok.len, "Hello, World!",
                            &request->buffer.buf[tok.index]);

  close(fd);

  destroy_test_request(request);

  return MUNIT_OK;
}

MunitResult test_read_socket_small_body_expand_buffer(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct http_request_s* request = setup_test_request();

  int callback_ran = 0;
  request->data = (void*)&callback_ran;

  struct hs_read_opts_s opts = {
    .max_request_buf_capacity = 4096,
    .initial_request_buf_capacity = 8,
    .eof_rc = -1
  };

  int fd = openat(AT_FDCWD, "test/unit/read_socket.txt", O_RDONLY);
  request->socket = fd;

  hs_read_request_and_exec_user_cb(request, opts);
  munit_assert_int64(request->server->memused, ==, 128);

  struct hsh_token_s tok = request->tokens.buf[request->tokens.size-1];

  munit_assert_memory_equal(tok.len, "Hello, World!",
                            &request->buffer.buf[tok.index]);

  munit_assert(callback_ran);

  close(fd);

  destroy_test_request(request);

  return MUNIT_OK;
}

void test_chunk_callback(struct http_request_s* request) {
  *((int*)request->data) = 2;
}

MunitResult test_read_socket_large_body(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct http_request_s* request = setup_test_request();

  int callback_ran = 0;
  request->data = (void*)&callback_ran;

  request->chunk_cb = &test_chunk_callback;

  struct hs_read_opts_s opts = {
    .max_request_buf_capacity = 72,
    .initial_request_buf_capacity = 72,
    .eof_rc = -1
  };

  int fd = openat(AT_FDCWD, "test/unit/read_socket.txt", O_RDONLY);
  request->socket = fd;

  hs_read_request_and_exec_user_cb(request, opts);

  munit_assert(HTTP_FLAG_CHECK(request->flags, HTTP_FLG_STREAMED));
  munit_assert_int(callback_ran, ==, 1);

  hs_read_request_and_exec_user_cb(request, opts);
  munit_assert_int(callback_ran, ==, 2);

  struct hsh_token_s tok = request->tokens.buf[request->tokens.size-1];
  munit_assert_memory_equal(tok.len, "Hello, ",
                            &request->buffer.buf[tok.index]);

  hs_read_request_and_exec_user_cb(request, opts);
  tok = request->tokens.buf[request->tokens.size-1];
  munit_assert_memory_equal(tok.len, "World!",
                            &request->buffer.buf[tok.index]);

  close(fd);
  destroy_test_request(request);

  return MUNIT_OK;
}
