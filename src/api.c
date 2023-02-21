#include <stdlib.h>

#ifndef HTTPSERVER_IMPL
#include "api.h"
#include "buffer_util.h"
#include "common.h"
#include "io_events.h"
#include "request_util.h"
#include "respond.h"
#include "server.h"
#endif

int http_request_has_flag(http_request_t *request, int flag) {
  return HTTP_FLAG_CHECK(request->flags, flag);
}

int http_server_loop(http_server_t *server) { return server->loop; }

http_server_t *http_server_init(int port, void (*handler)(http_request_t *)) {
#ifdef KQUEUE
  return hs_server_init(port, handler, hs_on_kqueue_server_event, NULL);
#else
  return hs_server_init(port, handler, hs_on_epoll_server_connection_event,
                        hs_on_epoll_server_timer_event);
#endif
}

void http_request_free_buffer(http_request_t *request) {
  _hs_buffer_free(&request->buffer, &request->server->memused);
}

void *http_request_userdata(http_request_t *request) { return request->data; }

void http_request_set_userdata(http_request_t *request, void *data) {
  request->data = data;
}

void http_server_set_userdata(struct http_server_s *serv, void *data) {
  serv->data = data;
}

void *http_request_server_userdata(struct http_request_s *request) {
  return request->server->data;
}

int http_request_iterate_headers(http_request_t *request, http_string_t *key,
                                 http_string_t *val, int *iter) {
  return hs_request_iterate_headers(request, key, val, iter);
}

http_string_t http_request_header(http_request_t *request, char const *key) {
  return hs_request_header(request, key);
}

void http_request_connection(http_request_t *request, int directive) {
  hs_request_set_keep_alive_flag(request, directive);
}

http_string_t http_request_chunk(struct http_request_s *request) {
  return hs_request_chunk(request);
}

http_response_t *http_response_init() { return hs_response_init(); }

void http_response_header(http_response_t *response, char const *key,
                          char const *value) {
  return hs_response_set_header(response, key, value);
}

void http_response_status(http_response_t *response, int status) {
  hs_response_set_status(response, status);
}

void http_response_body(http_response_t *response, char const *body,
                        int length) {
  hs_response_set_body(response, body, length);
}

void http_respond(http_request_t *request, http_response_t *response) {
  hs_request_respond(request, response, hs_request_begin_write);
}

void http_respond_chunk(http_request_t *request, http_response_t *response,
                        void (*cb)(http_request_t *)) {
  hs_request_respond_chunk(request, response, cb, hs_request_begin_write);
}

void http_respond_chunk_end(http_request_t *request,
                            http_response_t *response) {
  hs_request_respond_chunk_end(request, response, hs_request_begin_write);
}

http_string_t http_request_method(http_request_t *request) {
  return hs_get_token_string(request, HSH_TOK_METHOD);
}

http_string_t http_request_target(http_request_t *request) {
  return hs_get_token_string(request, HSH_TOK_TARGET);
}

http_string_t http_request_body(http_request_t *request) {
  return hs_get_token_string(request, HSH_TOK_BODY);
}

int http_server_listen(http_server_t *serv) {
  return hs_server_run_event_loop(serv, NULL);
}

int http_server_listen_addr(http_server_t *serv, const char *ipaddr) {
  return hs_server_run_event_loop(serv, ipaddr);
}

int http_server_poll(http_server_t *serv) {
  return hs_server_poll_events(serv);
}

int http_server_listen_poll(http_server_t *serv) {
  hs_server_listen_on_addr(serv, NULL);
  return 0;
}

int http_server_listen_addr_poll(http_server_t *serv, const char *ipaddr) {
  hs_server_listen_on_addr(serv, ipaddr);
  return 0;
}

void http_request_read_chunk(struct http_request_s *request,
                             void (*chunk_cb)(struct http_request_s *)) {
  request->state = HTTP_SESSION_READ;
  request->chunk_cb = chunk_cb;
  hs_request_begin_read(request);
}
