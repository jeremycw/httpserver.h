
#line 1 "src/parser.rl"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#ifndef HTTPSERVER_IMPL
#include "parser.h"
#endif

#define HSH_P_FLAG_CHUNKED 0x1
#define HSH_P_FLAG_TOKEN_READY 0x2
#define HSH_P_FLAG_DONE 0x4

#define HSH_ENTER_TOKEN(tok_type, max_len) \
  parser->token.type = tok_type; \
  parser->token.index = p - buffer->buf; \
  parser->token.flags = 0; \
  parser->limit_count = 0; \
  parser->limit_max = max_len;


#line 228 "src/parser.rl"



#line 31 "src/parser.c"
static const char _hsh_http_actions[] = {
	0, 1, 2, 1, 6, 1, 10, 1, 
	13, 1, 14, 1, 15, 1, 16, 1, 
	17, 1, 18, 2, 0, 10, 2, 1, 
	10, 2, 4, 10, 2, 5, 14, 2, 
	5, 16, 2, 5, 17, 2, 7, 10, 
	2, 8, 10, 2, 10, 11, 2, 12, 
	13, 2, 13, 14, 3, 3, 9, 10, 
	3, 4, 10, 6, 3, 7, 4, 10, 
	3, 9, 3, 10, 3, 13, 5, 14, 
	3, 13, 14, 12, 3, 14, 12, 13, 
	4, 5, 14, 12, 13, 4, 13, 5, 
	14, 12, 4, 14, 12, 13, 15
};

static const short _hsh_http_key_offsets[] = {
	0, 0, 4, 9, 10, 11, 12, 13, 
	14, 15, 16, 17, 18, 20, 21, 22, 
	39, 53, 55, 58, 60, 61, 79, 94, 
	110, 126, 142, 158, 174, 190, 205, 221, 
	237, 253, 269, 285, 301, 315, 317, 322, 
	324, 328, 344, 360, 376, 392, 408, 424, 
	440, 455, 471, 487, 503, 519, 535, 551, 
	567, 583, 597, 599, 603, 606, 609, 612, 
	615, 618, 621, 622, 623, 624, 631, 632, 
	632, 633, 634, 635, 642, 643, 643, 644, 
	652, 661, 669, 677, 686, 688, 697, 698, 
	699, 699, 699, 713, 713, 713, 721, 729, 
	729, 729
};

static const char _hsh_http_trans_keys[] = {
	65, 90, 97, 122, 32, 65, 90, 97, 
	122, 32, 32, 72, 84, 84, 80, 47, 
	49, 46, 48, 49, 13, 10, 9, 32, 
	34, 44, 47, 67, 84, 99, 116, 123, 
	125, 40, 41, 58, 64, 91, 93, 9, 
	32, 34, 44, 47, 58, 123, 125, 40, 
	41, 59, 64, 91, 93, 9, 32, 9, 
	13, 32, 10, 13, 10, 9, 13, 32, 
	34, 44, 47, 67, 84, 99, 116, 123, 
	125, 40, 41, 58, 64, 91, 93, 9, 
	10, 32, 34, 44, 47, 58, 123, 125, 
	40, 41, 59, 64, 91, 93, 9, 32, 
	34, 44, 47, 58, 79, 111, 123, 125, 
	40, 41, 59, 64, 91, 93, 9, 32, 
	34, 44, 47, 58, 78, 110, 123, 125, 
	40, 41, 59, 64, 91, 93, 9, 32, 
	34, 44, 47, 58, 84, 116, 123, 125, 
	40, 41, 59, 64, 91, 93, 9, 32, 
	34, 44, 47, 58, 69, 101, 123, 125, 
	40, 41, 59, 64, 91, 93, 9, 32, 
	34, 44, 47, 58, 78, 110, 123, 125, 
	40, 41, 59, 64, 91, 93, 9, 32, 
	34, 44, 47, 58, 84, 116, 123, 125, 
	40, 41, 59, 64, 91, 93, 9, 32, 
	34, 44, 45, 47, 58, 123, 125, 40, 
	41, 59, 64, 91, 93, 9, 32, 34, 
	44, 47, 58, 76, 108, 123, 125, 40, 
	41, 59, 64, 91, 93, 9, 32, 34, 
	44, 47, 58, 69, 101, 123, 125, 40, 
	41, 59, 64, 91, 93, 9, 32, 34, 
	44, 47, 58, 78, 110, 123, 125, 40, 
	41, 59, 64, 91, 93, 9, 32, 34, 
	44, 47, 58, 71, 103, 123, 125, 40, 
	41, 59, 64, 91, 93, 9, 32, 34, 
	44, 47, 58, 84, 116, 123, 125, 40, 
	41, 59, 64, 91, 93, 9, 32, 34, 
	44, 47, 58, 72, 104, 123, 125, 40, 
	41, 59, 64, 91, 93, 9, 32, 34, 
	44, 47, 58, 123, 125, 40, 41, 59, 
	64, 91, 93, 9, 32, 9, 13, 32, 
	48, 57, 10, 13, 10, 13, 48, 57, 
	9, 32, 34, 44, 47, 58, 82, 114, 
	123, 125, 40, 41, 59, 64, 91, 93, 
	9, 32, 34, 44, 47, 58, 65, 97, 
	123, 125, 40, 41, 59, 64, 91, 93, 
	9, 32, 34, 44, 47, 58, 78, 110, 
	123, 125, 40, 41, 59, 64, 91, 93, 
	9, 32, 34, 44, 47, 58, 83, 115, 
	123, 125, 40, 41, 59, 64, 91, 93, 
	9, 32, 34, 44, 47, 58, 70, 102, 
	123, 125, 40, 41, 59, 64, 91, 93, 
	9, 32, 34, 44, 47, 58, 69, 101, 
	123, 125, 40, 41, 59, 64, 91, 93, 
	9, 32, 34, 44, 47, 58, 82, 114, 
	123, 125, 40, 41, 59, 64, 91, 93, 
	9, 32, 34, 44, 45, 47, 58, 123, 
	125, 40, 41, 59, 64, 91, 93, 9, 
	32, 34, 44, 47, 58, 69, 101, 123, 
	125, 40, 41, 59, 64, 91, 93, 9, 
	32, 34, 44, 47, 58, 78, 110, 123, 
	125, 40, 41, 59, 64, 91, 93, 9, 
	32, 34, 44, 47, 58, 67, 99, 123, 
	125, 40, 41, 59, 64, 91, 93, 9, 
	32, 34, 44, 47, 58, 79, 111, 123, 
	125, 40, 41, 59, 64, 91, 93, 9, 
	32, 34, 44, 47, 58, 68, 100, 123, 
	125, 40, 41, 59, 64, 91, 93, 9, 
	32, 34, 44, 47, 58, 73, 105, 123, 
	125, 40, 41, 59, 64, 91, 93, 9, 
	32, 34, 44, 47, 58, 78, 110, 123, 
	125, 40, 41, 59, 64, 91, 93, 9, 
	32, 34, 44, 47, 58, 71, 103, 123, 
	125, 40, 41, 59, 64, 91, 93, 9, 
	32, 34, 44, 47, 58, 123, 125, 40, 
	41, 59, 64, 91, 93, 9, 32, 9, 
	13, 32, 99, 10, 13, 104, 10, 13, 
	117, 10, 13, 110, 10, 13, 107, 10, 
	13, 101, 10, 13, 100, 13, 10, 48, 
	13, 48, 57, 65, 70, 97, 102, 10, 
	13, 10, 48, 13, 48, 57, 65, 70, 
	97, 102, 10, 48, 13, 48, 49, 57, 
	65, 70, 97, 102, 10, 13, 48, 49, 
	57, 65, 70, 97, 102, 13, 48, 49, 
	57, 65, 70, 97, 102, 13, 48, 49, 
	57, 65, 70, 97, 102, 10, 13, 48, 
	49, 57, 65, 70, 97, 102, 13, 48, 
	10, 13, 48, 49, 57, 65, 70, 97, 
	102, 13, 10, 9, 32, 34, 44, 47, 
	58, 123, 125, 40, 41, 59, 64, 91, 
	93, 13, 48, 49, 57, 65, 70, 97, 
	102, 13, 48, 49, 57, 65, 70, 97, 
	102, 0
};

static const char _hsh_http_single_lengths[] = {
	0, 0, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 0, 1, 1, 11, 
	8, 2, 3, 2, 1, 12, 9, 10, 
	10, 10, 10, 10, 10, 9, 10, 10, 
	10, 10, 10, 10, 8, 2, 3, 2, 
	2, 10, 10, 10, 10, 10, 10, 10, 
	9, 10, 10, 10, 10, 10, 10, 10, 
	10, 8, 2, 4, 3, 3, 3, 3, 
	3, 3, 1, 1, 1, 1, 1, 0, 
	1, 1, 1, 1, 1, 0, 1, 2, 
	3, 2, 2, 3, 2, 3, 1, 1, 
	0, 0, 8, 0, 0, 2, 2, 0, 
	0, 0
};

static const char _hsh_http_range_lengths[] = {
	0, 2, 2, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 1, 0, 0, 3, 
	3, 0, 0, 0, 0, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 0, 1, 0, 
	1, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 3, 0, 0, 
	0, 0, 0, 3, 0, 0, 0, 3, 
	3, 3, 3, 3, 0, 3, 0, 0, 
	0, 0, 3, 0, 0, 3, 3, 0, 
	0, 0
};

static const short _hsh_http_index_offsets[] = {
	0, 0, 3, 7, 9, 11, 13, 15, 
	17, 19, 21, 23, 25, 27, 29, 31, 
	46, 58, 61, 65, 68, 70, 86, 99, 
	113, 127, 141, 155, 169, 183, 196, 210, 
	224, 238, 252, 266, 280, 292, 295, 300, 
	303, 307, 321, 335, 349, 363, 377, 391, 
	405, 418, 432, 446, 460, 474, 488, 502, 
	516, 530, 542, 545, 550, 554, 558, 562, 
	566, 570, 574, 576, 578, 580, 585, 587, 
	588, 590, 592, 594, 599, 601, 602, 604, 
	610, 617, 623, 629, 636, 639, 646, 648, 
	650, 651, 652, 664, 665, 666, 672, 678, 
	679, 680
};

static const char _hsh_http_indicies[] = {
	1, 1, 0, 2, 3, 3, 0, 0, 
	4, 6, 5, 7, 0, 8, 0, 9, 
	0, 10, 0, 11, 0, 12, 0, 13, 
	0, 14, 0, 15, 0, 16, 0, 0, 
	0, 0, 0, 0, 18, 19, 18, 19, 
	0, 0, 0, 0, 0, 17, 0, 0, 
	0, 0, 0, 21, 0, 0, 0, 0, 
	0, 20, 22, 22, 0, 24, 25, 24, 
	23, 0, 27, 26, 28, 0, 0, 30, 
	0, 0, 0, 0, 31, 32, 31, 32, 
	0, 0, 0, 0, 0, 29, 0, 33, 
	0, 0, 0, 0, 21, 0, 0, 0, 
	0, 0, 20, 0, 0, 0, 0, 0, 
	21, 34, 34, 0, 0, 0, 0, 0, 
	20, 0, 0, 0, 0, 0, 21, 35, 
	35, 0, 0, 0, 0, 0, 20, 0, 
	0, 0, 0, 0, 21, 36, 36, 0, 
	0, 0, 0, 0, 20, 0, 0, 0, 
	0, 0, 21, 37, 37, 0, 0, 0, 
	0, 0, 20, 0, 0, 0, 0, 0, 
	21, 38, 38, 0, 0, 0, 0, 0, 
	20, 0, 0, 0, 0, 0, 21, 39, 
	39, 0, 0, 0, 0, 0, 20, 0, 
	0, 0, 0, 40, 0, 21, 0, 0, 
	0, 0, 0, 20, 0, 0, 0, 0, 
	0, 21, 41, 41, 0, 0, 0, 0, 
	0, 20, 0, 0, 0, 0, 0, 21, 
	42, 42, 0, 0, 0, 0, 0, 20, 
	0, 0, 0, 0, 0, 21, 43, 43, 
	0, 0, 0, 0, 0, 20, 0, 0, 
	0, 0, 0, 21, 44, 44, 0, 0, 
	0, 0, 0, 20, 0, 0, 0, 0, 
	0, 21, 45, 45, 0, 0, 0, 0, 
	0, 20, 0, 0, 0, 0, 0, 21, 
	46, 46, 0, 0, 0, 0, 0, 20, 
	0, 0, 0, 0, 0, 47, 0, 0, 
	0, 0, 0, 20, 48, 48, 0, 49, 
	25, 49, 50, 23, 28, 27, 26, 0, 
	27, 51, 26, 0, 0, 0, 0, 0, 
	21, 52, 52, 0, 0, 0, 0, 0, 
	20, 0, 0, 0, 0, 0, 21, 53, 
	53, 0, 0, 0, 0, 0, 20, 0, 
	0, 0, 0, 0, 21, 54, 54, 0, 
	0, 0, 0, 0, 20, 0, 0, 0, 
	0, 0, 21, 55, 55, 0, 0, 0, 
	0, 0, 20, 0, 0, 0, 0, 0, 
	21, 56, 56, 0, 0, 0, 0, 0, 
	20, 0, 0, 0, 0, 0, 21, 57, 
	57, 0, 0, 0, 0, 0, 20, 0, 
	0, 0, 0, 0, 21, 58, 58, 0, 
	0, 0, 0, 0, 20, 0, 0, 0, 
	0, 59, 0, 21, 0, 0, 0, 0, 
	0, 20, 0, 0, 0, 0, 0, 21, 
	60, 60, 0, 0, 0, 0, 0, 20, 
	0, 0, 0, 0, 0, 21, 61, 61, 
	0, 0, 0, 0, 0, 20, 0, 0, 
	0, 0, 0, 21, 62, 62, 0, 0, 
	0, 0, 0, 20, 0, 0, 0, 0, 
	0, 21, 63, 63, 0, 0, 0, 0, 
	0, 20, 0, 0, 0, 0, 0, 21, 
	64, 64, 0, 0, 0, 0, 0, 20, 
	0, 0, 0, 0, 0, 21, 65, 65, 
	0, 0, 0, 0, 0, 20, 0, 0, 
	0, 0, 0, 21, 66, 66, 0, 0, 
	0, 0, 0, 20, 0, 0, 0, 0, 
	0, 21, 67, 67, 0, 0, 0, 0, 
	0, 20, 0, 0, 0, 0, 0, 68, 
	0, 0, 0, 0, 0, 20, 69, 69, 
	0, 70, 25, 70, 71, 23, 0, 27, 
	72, 26, 0, 27, 73, 26, 0, 27, 
	74, 26, 0, 27, 75, 26, 0, 27, 
	76, 26, 0, 27, 77, 26, 78, 0, 
	79, 0, 81, 80, 82, 83, 83, 83, 
	0, 84, 0, 85, 86, 0, 87, 0, 
	89, 88, 90, 91, 91, 91, 0, 92, 
	0, 93, 95, 94, 96, 97, 98, 98, 
	98, 94, 99, 96, 97, 98, 98, 98, 
	94, 101, 102, 103, 103, 103, 100, 104, 
	97, 98, 98, 98, 94, 105, 96, 97, 
	98, 98, 98, 94, 106, 95, 94, 107, 
	96, 97, 98, 98, 98, 94, 108, 0, 
	109, 0, 110, 111, 0, 0, 0, 0, 
	0, 21, 0, 0, 0, 0, 0, 20, 
	112, 0, 101, 102, 103, 103, 103, 100, 
	96, 97, 98, 98, 98, 94, 0, 113, 
	114, 0
};

static const char _hsh_http_trans_targs[] = {
	0, 2, 3, 2, 4, 4, 5, 6, 
	7, 8, 9, 10, 11, 12, 13, 14, 
	15, 16, 23, 41, 16, 17, 18, 19, 
	18, 39, 19, 20, 21, 16, 22, 23, 
	41, 90, 24, 25, 26, 27, 28, 29, 
	30, 31, 32, 33, 34, 35, 36, 37, 
	38, 38, 40, 40, 42, 43, 44, 45, 
	46, 47, 48, 49, 50, 51, 52, 53, 
	54, 55, 56, 57, 58, 59, 59, 60, 
	61, 62, 63, 64, 65, 19, 67, 68, 
	69, 72, 70, 69, 71, 91, 73, 92, 
	75, 86, 76, 75, 77, 78, 79, 84, 
	80, 82, 79, 81, 79, 80, 82, 79, 
	83, 93, 85, 94, 87, 95, 96, 97, 
	91, 96, 97
};

static const char _hsh_http_trans_actions[] = {
	17, 19, 3, 5, 22, 5, 3, 1, 
	0, 0, 0, 0, 0, 0, 0, 3, 
	0, 64, 64, 64, 5, 3, 0, 25, 
	25, 56, 5, 3, 5, 52, 52, 52, 
	52, 43, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 3, 
	0, 25, 60, 37, 5, 5, 5, 5, 
	5, 5, 5, 5, 5, 5, 5, 5, 
	5, 5, 5, 5, 3, 0, 25, 25, 
	5, 5, 5, 5, 5, 40, 0, 0, 
	46, 0, 0, 7, 0, 28, 0, 11, 
	46, 0, 0, 7, 0, 28, 76, 9, 
	76, 49, 72, 76, 80, 80, 68, 85, 
	76, 90, 76, 90, 0, 11, 31, 34, 
	9, 13, 15
};

static const char _hsh_http_eof_actions[] = {
	0, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0
};

static const int hsh_http_start = 1;
static const int hsh_http_first_final = 90;
static const int hsh_http_error = 0;

static const int hsh_http_en_chunk_end = 66;
static const int hsh_http_en_chunked_body = 74;
static const int hsh_http_en_small_body = 88;
static const int hsh_http_en_large_body = 89;
static const int hsh_http_en_main = 1;


#line 231 "src/parser.rl"

void hsh_parser_init(struct hsh_parser_s* parser) {
  memset(parser, 0, sizeof(struct hsh_parser_s));
  parser->state = hsh_http_start;
}

struct hsh_token_s hsh_parser_exec(struct hsh_parser_s* parser, struct hsh_buffer_s* buffer, int max_buf_capacity) {
  struct hsh_token_s none = {};
  none.type = HSH_TOK_NONE;
  if (HTTP_FLAG_CHECK(parser->flags, HSH_P_FLAG_DONE) || parser->sequence_id == buffer->sequence_id) {
    return none;
  }
  int cs = parser->state;
  char* eof = NULL;
  char *p = buffer->buf + buffer->index;
  char *pe = buffer->buf + buffer->length;
  
#line 376 "src/parser.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _hsh_http_trans_keys + _hsh_http_key_offsets[cs];
	_trans = _hsh_http_index_offsets[cs];

	_klen = _hsh_http_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _hsh_http_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _hsh_http_indicies[_trans];
	cs = _hsh_http_trans_targs[_trans];

	if ( _hsh_http_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _hsh_http_actions + _hsh_http_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 26 "src/parser.rl"
	{ HSH_ENTER_TOKEN(HSH_TOK_METHOD, 32) }
	break;
	case 1:
#line 27 "src/parser.rl"
	{ HSH_ENTER_TOKEN(HSH_TOK_TARGET, 1024) }
	break;
	case 2:
#line 28 "src/parser.rl"
	{ HSH_ENTER_TOKEN(HSH_TOK_VERSION, 16) }
	break;
	case 3:
#line 29 "src/parser.rl"
	{ HSH_ENTER_TOKEN(HSH_TOK_HEADER_KEY, 256) }
	break;
	case 4:
#line 30 "src/parser.rl"
	{ HSH_ENTER_TOKEN(HSH_TOK_HEADER_VALUE, 4096) }
	break;
	case 5:
#line 31 "src/parser.rl"
	{ parser->token.type = HSH_TOK_BODY; parser->token.index = p - buffer->buf; }
	break;
	case 6:
#line 32 "src/parser.rl"
	{
    parser->token.len = p - (buffer->buf + parser->token.index);
    // hsh_token_array_push(&parser->tokens, parser->token);
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
    {p++; goto _out; }
  }
	break;
	case 7:
#line 39 "src/parser.rl"
	{
    parser->content_length *= 10;
    parser->content_length += (*p) - '0';
  }
	break;
	case 8:
#line 44 "src/parser.rl"
	{
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_CHUNKED);
  }
	break;
	case 9:
#line 48 "src/parser.rl"
	{
    parser->limit_count = 0;
    parser->limit_max = 256;
  }
	break;
	case 10:
#line 53 "src/parser.rl"
	{
    parser->limit_count++;
    if (parser->limit_count > parser->limit_max) {
      // parser->rc = (int8_t)HSH_PARSER_ERR;
      {p++; goto _out; }
    }
  }
	break;
	case 11:
#line 61 "src/parser.rl"
	{
    buffer->after_headers_index = p - buffer->buf + 1;
    parser->content_remaining = parser->content_length;
    parser->token = (struct hsh_token_s){ };
    parser->token.type = HSH_TOK_HEADERS_DONE;
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
    if (HTTP_FLAG_CHECK(parser->flags, HSH_P_FLAG_CHUNKED)) {
      HTTP_FLAG_SET(parser->token.flags, HSH_TOK_FLAG_STREAMED_BODY);
      cs = 74;
      {p++; goto _out; }
    } else if (parser->content_length == 0) {
      HTTP_FLAG_SET(parser->token.flags, HSH_TOK_FLAG_NO_BODY);
      {p++; goto _out; }
    // The body won't fit into the buffer at maximum capacity.
    } else if (parser->content_length > max_buf_capacity - buffer->after_headers_index) {
      HTTP_FLAG_SET(parser->token.flags, HSH_TOK_FLAG_STREAMED_BODY);
      cs = 89;
      {p++; goto _out; }
    } else {
      // Resize the buffer to hold the full body
      if (parser->content_length + buffer->after_headers_index > buffer->capacity) {
        buffer->buf = realloc(buffer->buf, parser->content_length + buffer->after_headers_index);
        buffer->capacity = parser->content_length + buffer->after_headers_index;
      }
      cs = 88;
      {p++; goto _out; }
    }
  }
	break;
	case 12:
#line 90 "src/parser.rl"
	{
    parser->content_length = 0;
  }
	break;
	case 13:
#line 94 "src/parser.rl"
	{
    if ((*p) >= 'A' && (*p) <= 'F') {
      parser->content_length *= 0x10;
      parser->content_length += (*p) - 55;
    } else if ((*p) >= 'a' && (*p) <= 'f') {
      parser->content_length *= 0x10;
      parser->content_length += (*p) - 87;
    } else if ((*p) >= '0' && (*p) <= '9') {
      parser->content_length *= 0x10;
      parser->content_length += (*p) - '0';
    }
  }
	break;
	case 14:
#line 107 "src/parser.rl"
	{
    char* last_body_byte = buffer->buf + parser->token.index + parser->content_length - 1;
    if (pe >= last_body_byte) {
      p = last_body_byte;
      parser->token.len = parser->content_length;
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
      cs = 66;
      {p++; goto _out; }
    // The current chunk is at the end of the buffer and the buffer cannot be expanded.
    // Move the remaining contents of the buffer to just after the headers to free up
    // capacity in the buffer.
    } else if (p - buffer->buf + parser->content_length > max_buf_capacity) {
      memcpy(buffer->buf + buffer->after_headers_index, p, pe - p);
      buffer->length = buffer->after_headers_index + pe - p;
      p = buffer->buf + buffer->after_headers_index;
      parser->token.index = buffer->after_headers_index;
      parser->sequence_id = buffer->sequence_id;
      p--;
      {p++; goto _out; }
    }
  }
	break;
	case 15:
#line 129 "src/parser.rl"
	{
    // write 0 byte body to tokens
    parser->token.type = HSH_TOK_BODY;
    parser->token.index = 0;
    parser->token.len = 0;
    parser->token.flags = HSH_TOK_FLAG_BODY_FINAL;
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_DONE);
    {p++; goto _out; }
  }
	break;
	case 16:
#line 140 "src/parser.rl"
	{
    parser->token.index = buffer->after_headers_index;
    parser->token.len = parser->content_length;
    HTTP_FLAG_SET(parser->token.flags, HSH_TOK_FLAG_SMALL_BODY);
    char* last_body_byte = buffer->buf + parser->token.index + parser->content_length - 1;
    if (pe >= last_body_byte) {
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_DONE);
    }
    p--;
    {p++; goto _out; }
  }
	break;
	case 17:
#line 153 "src/parser.rl"
	{
    parser->token.index = buffer->after_headers_index;
    char* last_body_byte = buffer->buf + buffer->after_headers_index + parser->content_remaining - 1;
    if (pe >= last_body_byte) {
      parser->token.len = parser->content_remaining;
      parser->content_remaining = 0;
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_DONE);
    } else {
      parser->token.len = pe - p;
      parser->content_remaining -= parser->token.len;
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
      p = buffer->buf + buffer->after_headers_index;
      parser->sequence_id = buffer->sequence_id;
    }
    p--;
    {p++; goto _out; }
  }
	break;
	case 18:
#line 172 "src/parser.rl"
	{
    // parser->rc = (int8_t)HSH_PARSER_ERR;
    {p++; goto _out; }
  }
	break;
#line 645 "src/parser.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _hsh_http_actions + _hsh_http_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 18:
#line 172 "src/parser.rl"
	{
    // parser->rc = (int8_t)HSH_PARSER_ERR;
    {p++; goto _out; }
  }
	break;
#line 668 "src/parser.c"
		}
	}
	}

	_out: {}
	}

#line 248 "src/parser.rl"
  parser->state = cs;
  buffer->index = p - buffer->buf;
  if (HTTP_FLAG_CHECK(parser->flags, HSH_P_FLAG_TOKEN_READY)) {
    HTTP_FLAG_CLEAR(parser->flags, HSH_P_FLAG_TOKEN_READY);
    return parser->token;
  } else {
    parser->sequence_id = buffer->sequence_id;
    return none;
  }
}
