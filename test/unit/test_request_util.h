#ifndef HS_TEST_REQUEST_UTIL_H
#define HS_TEST_REQUEST_UTIL_H

MunitResult test_request_keep_alive_http_1_1(const MunitParameter params[], void* data);
MunitResult test_request_keep_alive_http_1_0(const MunitParameter params[], void* data);
MunitResult test_request_keep_alive_connection_close(const MunitParameter params[], void* data);

#endif