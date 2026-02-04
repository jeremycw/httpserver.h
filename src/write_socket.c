#include <errno.h>
#include <unistd.h>

#ifndef HTTPSERVER_IMPL
#include "common.h"
#include "write_socket.h"
#endif

#ifdef DEBUG
#define write hs_test_write
ssize_t hs_test_write(int fd, char const *data, size_t size);
#endif

// Writes response bytes from the buffer out to the socket.
//
// Runs when we get a socket ready to write event or when initiating an HTTP
// response and writing to the socket for the first time. If the response is
// chunked the chunk_cb callback will be invoked signalling to the user code
// that another chunk is ready to be written.
enum hs_write_rc_e hs_write_socket(http_request_t *request) {
  int bytes =
      write(request->socket, request->buffer.buf + request->bytes_written,
            request->buffer.length - request->bytes_written);
  if (bytes > 0)
    request->bytes_written += bytes;

  enum hs_write_rc_e rc = HS_WRITE_RC_SUCCESS;

  if (errno == EPIPE) {
    rc = HS_WRITE_RC_SOCKET_ERR;
  } else {
    if (request->bytes_written != request->buffer.length) {
      // All bytes of the body were not written and we need to wait until the
      // socket is writable again to complete the write
      rc = HS_WRITE_RC_CONTINUE;
    } else if (HTTP_FLAG_CHECK(request->flags, HTTP_CHUNKED_RESPONSE)) {
      // All bytes of the chunk were written and we need to get the next chunk
      // from the application.
      rc = HS_WRITE_RC_SUCCESS_CHUNK;
    } else {
      if (HTTP_FLAG_CHECK(request->flags, HTTP_KEEP_ALIVE)) {
        rc = HS_WRITE_RC_SUCCESS;
      } else {
        rc = HS_WRITE_RC_SUCCESS_CLOSE;
      }
    }
  }

  return rc;
}
