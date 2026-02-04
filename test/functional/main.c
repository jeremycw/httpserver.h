#define HTTPSERVER_IMPL
#include "../../build/src/httpserver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define RESPONSE "Hello, World!"

ssize_t hs_test_write(int fd, char const *data, size_t size) {
  return write(fd, data, size);
}

int request_target_is(struct http_request_s* request, char const * target) {
  http_string_t url = http_request_target(request);
  int len = strlen(target);
  return len == url.len && memcmp(url.buf, target, url.len) == 0;
}

int chunk_count = 0;

void chunk_cb(struct http_request_s* request) {
  chunk_count++;
  struct http_response_s* response = http_response_init();
  http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  if (chunk_count < 3) {
    http_respond_chunk(request, response, chunk_cb);
  } else {
    http_response_header(response, "Foo-Header", "bar");
    http_respond_chunk_end(request, response);
  }
}

typedef struct {
  char* buf;
  struct http_response_s* response;
  int index;
} chunk_buf_t;

void chunk_req_cb(struct http_request_s* request) {
  http_string_t str = http_request_chunk(request);
  chunk_buf_t* chunk_buffer = (chunk_buf_t*)http_request_userdata(request);
  if (str.len > 0) {
    memcpy(chunk_buffer->buf + chunk_buffer->index, str.buf, str.len);
    chunk_buffer->index += str.len;
    http_request_read_chunk(request, chunk_req_cb);
  } else {
    http_response_body(chunk_buffer->response, chunk_buffer->buf, chunk_buffer->index);
    http_respond(request, chunk_buffer->response);
    free(chunk_buffer->buf);
    free(chunk_buffer);
  }
}

struct http_server_s* poll_server;

void handle_request(struct http_request_s* request) {
  chunk_count = 0;
  http_request_connection(request, HTTP_AUTOMATIC);
  struct http_response_s* response = http_response_init();
  http_response_status(response, 200);
  if (request_target_is(request, "/echo")) {
    http_string_t body = http_request_body(request);
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, body.buf, body.len);
  } else if (request_target_is(request, "/host")) {
    http_string_t ua = http_request_header(request, "Host");
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, ua.buf, ua.len);
  } else if (request_target_is(request, "/poll")) {
    while (http_server_poll(poll_server) > 0);
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  } else if (request_target_is(request, "/empty")) {
    // No Body
  } else if (request_target_is(request, "/chunked")) {
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
    http_respond_chunk(request, response, chunk_cb);
    return;
  } else if (request_target_is(request, "/chunked-req")) {
    chunk_buf_t* chunk_buffer = (chunk_buf_t*)calloc(1, sizeof(chunk_buf_t));
    chunk_buffer->buf = (char*)malloc(512 * 1024);
    chunk_buffer->response = response;
    http_request_set_userdata(request, chunk_buffer);
    http_request_read_chunk(request, chunk_req_cb);
    return;
  } else if (request_target_is(request, "/large")) {
    chunk_buf_t* chunk_buffer = (chunk_buf_t*)calloc(1, sizeof(chunk_buf_t));
    chunk_buffer->buf = (char*)malloc(25165824);
    chunk_buffer->response = response;
    http_request_set_userdata(request, chunk_buffer);
    http_request_read_chunk(request, chunk_req_cb);
    return;
  } else if (request_target_is(request, "/headers")) {
    int iter = 0, i = 0;
    http_string_t key, val;
    char buf[512];
    while (http_request_iterate_headers(request, &key, &val, &iter)) {
      i += snprintf(buf + i, 512 - i, "%.*s: %.*s\n", key.len, key.buf, val.len, val.buf);
    }
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, buf, i);
    return http_respond(request, response);
  } else {
    http_response_header(response, "Content-Type", "text/plain");
    http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  }
  http_respond(request, response);
}

struct http_server_s* server;

void handle_sigterm(int signum) {
  (void)signum;
  free(server);
  free(poll_server);
  exit(0);
}

int main() {
  signal(SIGTERM, handle_sigterm);
  server = http_server_init(8080, handle_request);
  poll_server = http_server_init(8081, handle_request);
  http_server_listen_poll(poll_server);
  http_server_listen(server);
}
