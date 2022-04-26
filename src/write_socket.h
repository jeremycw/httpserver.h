#ifndef HS_WRITE_SOCKET_H
#define HS_WRITE_SOCKET_H

#define HTTP_KEEP_ALIVE_TIMEOUT 120

struct http_request_s;

// Response code for hs_write_socket
enum hs_write_rc_e {
  // Successful and has written the full response
  HS_WRITE_RC_SUCCESS,
  // Successful and has written the full chunk
  HS_WRITE_RC_SUCCESS_CHUNK,
  // Successful, has written the full response and the socket should be closed
  HS_WRITE_RC_SUCCESS_CLOSE,
  // Successful but has not written the full response, wait for write ready
  HS_WRITE_RC_CONTINUE,
  // Error writing to the socket
  HS_WRITE_RC_SOCKET_ERR
};

enum hs_write_rc_e hs_write_socket(struct http_request_s *request);

#endif
