#ifndef TEST_PARSER_H
#define TEST_PARSER_H

MunitResult test_parser_small_body_complete(const MunitParameter params[], void* data);
MunitResult test_parser_small_body_partial(const MunitParameter params[], void* data);
MunitResult test_parser_large_body(const MunitParameter params[], void* data);
MunitResult test_parser_chunked_body(const MunitParameter params[], void* data);
MunitResult test_parser_chunked_body_partial(const MunitParameter params[], void* data);

#endif
