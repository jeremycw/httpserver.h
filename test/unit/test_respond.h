#ifndef HS_TEST_RESPOND_H
#define HS_TEST_RESPOND_H

MunitResult test_respond_error_single_write(const MunitParameter params[], void* data);
MunitResult test_respond_error_status_range(const MunitParameter params[], void* data);
MunitResult test_respond_large_body_capture(const MunitParameter params[], void* data);
MunitResult test_respond_grwprintf_truncation_bug(const MunitParameter params[], void* data);

#endif