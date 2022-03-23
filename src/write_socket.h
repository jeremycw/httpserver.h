#ifndef HS_WRITE_SOCKET_H
#define HS_WRITE_SOCKET_H

#define HTTP_KEEP_ALIVE_TIMEOUT 120

struct http_request_s;

enum hs_write_rc_e {
  HS_WRITE_RC_SUCCESS,
  HS_WRITE_RC_SUCCESS_CLOSE,
  HS_WRITE_RC_CONTINUE,
  HS_WRITE_RC_SOCKET_ERR
};

enum hs_write_rc_e hs_write_socket(struct http_request_s* request);

#endif
