/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* MIT License
* 
* Copyright (c) 2020 Jeremy Williams
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* httpserver-pg.h (0.1.0)
*
* Description:
*
*   Helper for adding non-blocking PostgreSQL connectivity to your httpserver.h
*   based application. It does, however, not depend on httpserver.h at all and
*   could be potentially used for other applications.
*
* Usage:
*
*   If you include this header in your project you will need to install libpq
*   and link it with -lpq
*
*   Do this:
*      #define HTTPSERVER_PG_IMPL
*   before you include this file in *one* C or C++ file to create the
*   implementation.
*
*   // i.e. it should look like this:
*   #include ...
*   #include ...
*   #include ...
*   #define HTTPSERVER_PG_IMPL
*   #include "httpserver-pg.h"
*
*   There are some #defines that can be configured. This must be done in the
*   same file that you define HTTPSERVER_PG_IMPL These defines have default
*   values and will need to be #undef'd and redefined to configure them.
*
*     HS_DB_CONNECTION_POOL_SIZE - default 25 - The amount of connections to
*       opened with the server on initialization. Even though communication
*       over each connection is non-blocking queries cannot be multiplexed over
*       one connection. Therefore this number amounts to the maximum number of
*       inflight queries to the database that can happen concurrently.
*
*   For more details see the documentation of the interface and the example
*   below.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef HTTPSERVER_PG_H
#define HTTPSERVER_PG_H

#include <libpq-fe.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hs_db_s;
struct hs_db_conn_s;

// Param structure for passing into hs_db_exec
//   server: the context returned by the hs_db_init call
//   data: for passing in application data or context that will be accessible
//     through hs_db_get_data when the callback executes.
//   callback: function pointer that will get executed when the query results
//     are ready.
struct hs_db_params_s {
  struct hs_db_s* server;
  void* data;
  void (*callback)(struct hs_db_conn_s*);
};

// Initialize the pool of connections to the database. connect_str is the string
// that will be passed into libpq. Details can be seen here:
//   https://www.postgresql.org/docs/12/libpq-connect.html#LIBPQ-CONNSTRING
// loop is the file descriptor of the epoll or kqueue event queue.
struct hs_db_s* hs_db_init(char const* connect_str, int loop);

// Execute the passed in query in the context of the params.
int hs_db_exec(struct hs_db_params_s params, char const* query);

// Just a utility method to easily create the hs_db_params_s struct based on the
// given arguments.
struct hs_db_params_s hs_db_exec_params(
  struct hs_db_s* server,
  void* data,
  void (*cb)(struct hs_db_conn_s*)
);

// Just another way to execute a query if you don't want to bundle the params
// into a hs_db_params_s struct. See the description of hs_db_params_s to
// understand what the parameters are.
int hs_db_execp(
  struct hs_db_s* server,
  char const * query,
  void* data,
  void (*cb)(struct hs_db_conn_s*)
);

// For use inside the callback to retrieve the results of the query. See
// https://www.postgresql.org/docs/9.5/libpq-exec.html#LIBPQ-EXEC-SELECT-INFO
// for details on how to work with the result.
PGresult* hs_db_get_result(struct hs_db_conn_s* conn);

// For use inside the callback to retrieve the application data that was passed
// into hs_db_exec via the data pointer.
void* hs_db_get_data(struct hs_db_conn_s* conn);

#ifdef __cplusplus
}
#endif

// *** Example Program ***

#ifdef HS_PG_EXAMPLE

#define HTTPSERVER_IMPL
#include "httpserver.h"
#define HTTPSERVER_PG_IMPL
#include "httpserver-pg.h"

struct hs_db_s* db;

void result_cb(struct hs_db_conn_s* conn) {
  struct http_response_s* response = http_response_init();
  struct http_request_s* request = (struct http_request_s*)hs_db_get_data(conn);
  PGresult* result = hs_db_get_result(conn);
  char* name = PQfname(result, 0);
  http_response_status(response, 200);
  http_response_body(response, name, strlen(name));
  http_respond(request, response);
}

void handle_request(struct http_request_s* request) {
  hs_db_execp(db, "select 1 as test_col", request, result_cb);
}

int main() {
  struct http_server_s* server = http_server_init(8080, handle_request);
  db = hs_db_init("postgres://localhost:5432/jeremy", http_server_loop(server));
  http_server_listen(server);
}

#endif //HS_PG_EXAMPLE

#ifdef HTTPSERVER_PG_IMPL

#ifdef __linux__
#define HS_PG_EPOLL
#else
#define HS_PG_KQUEUE
#endif

#define HS_DB_CONNECTION_POOL_SIZE 25

typedef struct hs_db_conn_s {
#ifdef HS_PG_KQUEUE
  void (*handler)(struct kevent*);
#else
  void (*handler)(struct epoll_event*);
#endif
  PGconn* dbconn;
  PGresult* result;
  void* data;
  struct hs_db_s* server;
  void (*result_cb)(struct hs_db_conn_s* conn);
  struct hs_db_conn_s* next;
} hs_db_conn_t;

typedef struct hs_db_s {
  int loop;
  hs_db_conn_t* avail_conn_head;
  hs_db_conn_t pool[HS_DB_CONNECTION_POOL_SIZE];
} hs_db_t;

#ifdef HS_PG_KQUEUE
void hs_db_read_cb(struct kevent* ev);
#else
void hs_db_read_cb(struct epoll_event* ev);
#endif

void hs_db_add_read_events(hs_db_conn_t* conn);

typedef struct hs_db_params_s hs_db_params_t;

void hs_db_add_avail_conn(hs_db_t* db, hs_db_conn_t* conn) {
  hs_db_conn_t* next = db->avail_conn_head;
  conn->next = next;
  db->avail_conn_head = conn;
}

hs_db_t* hs_db_init(char const * connect_str, int loop) {
  hs_db_t* db = (hs_db_t*)malloc(sizeof(hs_db_t));
  db->loop = loop;
  for (int i = 0; i < HS_DB_CONNECTION_POOL_SIZE; i++) {
    PGconn* dbconn = PQconnectdb(connect_str);
    PQsetnonblocking(dbconn, 1);
    hs_db_conn_t* conn = &db->pool[i];
    conn->dbconn = dbconn;
    conn->handler = hs_db_read_cb;
    conn->server = db;
    hs_db_add_avail_conn(db, conn);
    hs_db_add_read_events(conn);
  }
  return db;
}

hs_db_params_t hs_db_exec_params(hs_db_t* server, void* data, void (*cb)(hs_db_conn_t*)) {
  return (hs_db_params_t) {
    .server = server,
    .data = data,
    .callback = cb
  };
}

int hs_db_exec(hs_db_params_t params, char const* query) {
  if (params.server->avail_conn_head == NULL) return 0;
  hs_db_conn_t* conn = params.server->avail_conn_head;
  params.server->avail_conn_head = conn->next;
  conn->result_cb = params.callback;
  conn->data = params.data;
  return PQsendQuery(conn->dbconn, query);
}

int hs_db_execp(hs_db_t* server, char const * query, void* data, void (*cb)(hs_db_conn_t*)) {
  hs_db_params_t params = hs_db_exec_params(server, data, cb);
  return hs_db_exec(params, query);
}

void hs_db_read_conn(hs_db_conn_t* conn) {
  PQconsumeInput(conn->dbconn);
  PGresult* res;
  while (!PQisBusy(conn->dbconn) && (res = PQgetResult(conn->dbconn))) {
    conn->result = res;
    conn->result_cb(conn);
    PQclear(res);
  }
  if (res == NULL) {
    hs_db_add_avail_conn(conn->server, conn);
  }
}

PGresult* hs_db_get_result(hs_db_conn_t* conn) {
  return conn->result;
}

void* hs_db_get_data(hs_db_conn_t* conn) {
  return conn->data;
}

#ifdef HS_PG_KQUEUE

void hs_db_read_cb(struct kevent* ev) {
  hs_db_read_conn((hs_db_conn_t*)ev->udata);
}

void hs_db_add_read_events(hs_db_conn_t* conn) {
  struct kevent ev_set;
  EV_SET(&ev_set, PQsocket(conn->dbconn), EVFILT_READ, EV_ADD, 0, 0, conn);
  kevent(conn->server->loop, &ev_set, 1, NULL, 0, NULL);
}

#else

void hs_db_add_read_events(hs_db_conn_t* conn) {
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = conn;
  epoll_ctl(conn->server->loop, EPOLL_CTL_ADD, PQsocket(conn->dbconn), &ev);
}

void hs_db_read_cb(struct epoll_event* ev) {
  hs_db_read_conn((hs_db_conn_t*)ev->data.ptr);
}

#endif //HS_PG_KQUEUE

#endif //HTTPSERVER_PG_IMPL

#endif //HTTPSERVER_PG_H

