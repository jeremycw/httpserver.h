#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

http_string_t http_request_path(http_request_t* request) {
  http_string_t path = {0, 0};
  http_string_t target = http_get_token_string(request, HS_TOK_TARGET);
  char* q = (char *)memchr(target.buf, '?', target.len);
  if (q == NULL) {
    return target;
  }
  path.buf = target.buf;
  path.len = q - target.buf;
  return path;
}

http_string_t http_request_querystring(http_request_t* request) {
  http_string_t query = {0, 0};
  http_string_t target = http_request_target(request);
  char* q = (char *)memchr(target.buf, '?', target.len);
  if (q == NULL) {
    return query;
  }
  query.buf = &q[1];
  query.len = target.len - (q - target.buf + 1);
  return query;
}

void hs_token_array_init(struct hs_token_array_s *array, int capacity) {
  array->buf =
      (struct hsh_token_s *)malloc(sizeof(struct hsh_token_s) * capacity);
  assert(array->buf != NULL);
  array->size = 0;
  array->capacity = capacity;
}

void hs_token_array_push(struct hs_token_array_s *array,
                          struct hsh_token_s a) {
  if (array->size == array->capacity) {
    array->capacity *= 2;
    array->buf = (struct hsh_token_s *)realloc(
        array->buf, array->capacity * sizeof(struct hsh_token_s));
    assert(array->buf != NULL);
  }
  array->buf[array->size] = a;
  array->size++;
}

void hs_parse_querystring(http_request_t* request, http_string_t query) {
  struct hsh_token_s tok = {HS_TOK_QUERY_KEY, 0, 0, 0};
  if (request->query.buf != NULL) {
    return;
  }
  hs_token_array_init(&request->query, 32);
  for (int i = 0; i < query.len; i++) {
    if (query.buf[i] == '&') {
      if (tok.index != -1) {
        hs_token_array_push(&request->query, tok);
        tok.index = -1;
        tok.len = 0;
      }
      tok.index = i+1;
      tok.type = HSH_TOK_QUERY_KEY;
      continue;
    }
    else if (query.buf[i] == '=') {
      if (tok.index != -1 && tok.type != HSH_TOK_QUERY_VAL) {
        hs_token_array_push(&request->query, tok);
        tok.index = -1;
        tok.len = 0;
      }
      tok.index = i+1;
      tok.type = HSH_TOK_QUERY_VAL;
      continue;
    }
    tok.len++;
  }
  if (tok.index != -1) {
    hs_token_array_push(&request->query, tok);
  }
  return;
}

int hs_case_cmp(const char *s1, int s1len, const char *s2, int s2len) {
  if (s1len != s2len) {
    return 1;
  }

  return memcmp(s1, s2, s1len);
}

http_string_t http_request_query(http_request_t* request, char const * key) {
  http_string_t value = {0, 0};
  http_string_t query = http_request_querystring(request);
  if (query.len == 0) return value;
  if (key == NULL) return value;
  size_t len = strlen(key);
  if(len == 0) return  value;

  hs_parse_querystring(request, query);

  for (int i = request->query.size-1; i >= 0; i--) {
    struct hsh_token_s tok = request->query.buf[i];
    if (tok.type != HSH_TOK_QUERY_KEY) {
      continue;
    }
    if (hs_case_cmp(&query.buf[tok.index], tok.len, key, len) == 0) {
      continue;
    }
    if (i+1 >= request->query.size) {
      return value;
    }
    tok = request->query.buf[i+1];
    if (tok.type != HSH_TOK_QUERY_VAL) {
      return value;
    }
    value.buf = &query.buf[tok.index];
    value.len = tok.len;
    break;
  }
  return value;
}

int http_request_iterate_query(
  http_request_t* request,
  http_string_t* key,
  http_string_t* val,
  int* iter
) {
  http_string_t query = http_request_querystring(request);
  hs_parse_querystring(request, query);
  for(; *iter < request->query.size; (*iter)++) {
    struct hsh_token_s token = request->query.buf[*iter];
    if (token.type != HSH_TOK_QUERY_KEY) {
      continue;
    }
    *key = (http_string_t) {
      .buf = &query.buf[token.index],
      .len = token.len
    };
    if ((*iter)+1 >= request->query.size) {
      *val = (http_string_t) {
        .buf = NULL,
        .len = 0
      };
      return 1;
    }
    (*iter)++;
    token = request->query.buf[*iter];
    if (token.type != HSH_TOK_QUERY_VAL) {
      *val = (http_string_t) {
        .buf = NULL,
        .len = 0
      };
      return 1;
    }
    *val = (http_string_t) {
      .buf = &query.buf[token.index],
      .len = token.len
    };
    return 1;
  }
  return 0;
}