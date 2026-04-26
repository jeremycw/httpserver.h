#include <stddef.h>
#include "munit.h"

enum hs_test_write_mode_e {
  HS_TEST_WRITE_SUCCESS,
  HS_TEST_WRITE_PARTIAL,
  HS_TEST_WRITE_CAPTURE
};

extern enum hs_test_write_mode_e hs_test_write_mode;

void hs_test_enable_write_stub(int enabled);
void hs_test_reset_capture(void);
ssize_t hs_test_write(int fd, char const *data, size_t size);
extern char* captured_write_buf;
extern size_t captured_write_size;

MunitResult test_write_socket_partial(const MunitParameter params[], void* data);
