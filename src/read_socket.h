#ifndef HS_READ_SOCKET_H
#define HS_READ_SOCKET_H

#define HTTP_FLG_STREAMED 0x1

#include <stdint.h>

struct http_request_s;

// Response code for hs_read_socket
enum hs_read_rc_e {
  // Execution was successful
  HS_READ_RC_SUCCESS,
  // There was an error parsing the HTTP request
  HS_READ_RC_PARSE_ERR,
  // There was an error reading the socket
  HS_READ_RC_SOCKET_ERR
};

// Holds configuration options for the hs_read_socket procedure.
struct hs_read_opts_s {
  // Restricts the request buffer from ever growing larger than this size
  int64_t max_request_buf_capacity;
  // The value to be compared to the return of the read call to determine if
  // the connection has been closed. Should generally be 0 in normal operation
  // using sockets but can be useful to change if you want to use files instead
  // of sockets for testing.
  int eof_rc;
  // The initial capacity that is allocated for the request buffer
  int initial_request_buf_capacity;
};

enum hs_read_rc_e
hs_read_request_and_exec_user_cb(struct http_request_s *request,
                                 struct hs_read_opts_s opts);

#endif
