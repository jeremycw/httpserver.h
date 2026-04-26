enum hs_test_write_mode_e {
  HS_TEST_WRITE_SUCCESS,
  HS_TEST_WRITE_PARTIAL,
  HS_TEST_WRITE_CAPTURE
};

enum hs_test_write_mode_e hs_test_write_mode;

void hs_test_enable_write_stub(int enabled);
void hs_test_reset_capture(void);
char* captured_write_buf;
size_t captured_write_size;

MunitResult test_write_socket_partial(const MunitParameter params[], void* data);
