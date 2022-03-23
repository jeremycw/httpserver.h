#ifndef HS_READ_SOCKET_H
#define HS_READ_SOCKET_H

#define HTTP_FLG_STREAMED 0x1

#include <stdint.h>

struct http_request_s;

enum hs_read_rc_e {
  HS_READ_RC_SUCCESS,
  HS_READ_RC_PARSE_ERR,
  HS_READ_RC_SOCKET_ERR
};

struct hs_read_opts_s {
  int64_t max_request_buf_capacity;
  int eof_rc;
  int initial_request_buf_capacity;
};

enum hs_read_rc_e hs_read_socket(struct http_request_s* request, struct hs_read_opts_s opts);

#endif
