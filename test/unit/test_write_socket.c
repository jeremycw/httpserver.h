#include <unistd.h>
#include "munit.h"

#include "../../src/write_socket.h"
#include "../../src/common.h"
#include "../debugbreak.h"

enum hs_test_write_mode_e {
  HS_TEST_WRITE_SUCCESS,
  HS_TEST_WRITE_PARTIAL
};

static enum hs_test_write_mode_e hs_test_write_mode;

ssize_t hs_test_write(int fd, char const *data, size_t size) {
  (void)data;
  (void)fd;

  switch (hs_test_write_mode) {
    case HS_TEST_WRITE_SUCCESS:
      return size;
    case HS_TEST_WRITE_PARTIAL:
      return size / 2;
  }
  return 0;
}

static struct http_request_s* setup_test_request() {
  struct http_request_s* request = calloc(1, sizeof(struct http_request_s));
  request->server = calloc(1, sizeof(struct http_server_s));

  request->buffer.buf = (char *)calloc(1, 1024);
  request->buffer.capacity = 1024;

  return request;
}

static void destroy_test_request(struct http_request_s* request) {
  free(request->server);
  free(request->buffer.buf);
  free(request->tokens.buf);
  free(request);
}

MunitResult test_write_socket_partial(const MunitParameter params[], void* data) {
  (void)params;
  (void)data;

  struct http_request_s* request = setup_test_request();

  request->buffer.length = 512;

  hs_test_write_mode = HS_TEST_WRITE_PARTIAL;
  enum hs_write_rc_e rc = hs_write_socket(request);

  munit_assert_int(rc, ==, HS_WRITE_RC_CONTINUE);

  destroy_test_request(request);

  return MUNIT_OK;
}
