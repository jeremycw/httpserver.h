// httpserver.h has been automatically generated from httpserver.m4 and the
// source files under /src
#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#line 1 "api.h"
#ifndef HS_API_H
#define HS_API_H
/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * @file api.h
 *
 * MIT License
 *
 * Copyright (c) 2019 Jeremy Williams
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
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
 * httpserver.h (0.9.0)
 *
 * Description:
 *
 *   A single header C library for building non-blocking event driven HTTP
 *   servers
 *
 * Usage:
 *
 *   Do this:
 *      #define HTTPSERVER_IMPL
 *   before you include this file in *one* C or C++ file to create the
 *   implementation.
 *
 *   // i.e. it should look like this:
 *   #include ...
 *   #include ...
 *   #include ...
 *   #define HTTPSERVER_IMPL
 *   #include "httpserver.h"
 *
 *   There are some #defines that can be configured. This must be done in the
 *   same file that you define HTTPSERVER_IMPL These defines have default values
 *   and will need to be #undef'd and redefined to configure them.
 *
 *     HTTP_REQUEST_BUF_SIZE - default 1024 - The initial size in bytes of the
 *       read buffer for the request. This buffer grows automatically if it's
 *       capacity is reached but it certain environments it may be optimal to
 *       change this value.
 *
 *     HTTP_RESPONSE_BUF_SIZE - default 1024 - Same as above except for the
 *       response buffer.
 *
 *     HTTP_REQUEST_TIMEOUT - default 20 - The amount of seconds the request
 * will wait for activity on the socket before closing. This only applies mid
 *       request. For the amount of time to hold onto keep-alive connections see
 *       below.
 *
 *     HTTP_KEEP_ALIVE_TIMEOUT - default 120 - The amount of seconds to keep a
 *       connection alive a keep-alive request has completed.
 *
 *     HTTP_MAX_TOTAL_EST_MEM_USAGE - default 4294967296 (4GB) - This is the
 *       amount of read/write buffer space that is allowed to be allocated
 * across all requests before new requests will get 503 responses.
 *
 *     HTTP_MAX_TOKEN_LENGTH - default 8192 (8KB) - This is the max size of any
 *       non body http tokens. i.e: header names, header values, url length,
 * etc.
 *
 *     HTTP_MAX_REQUEST_BUF_SIZE - default 8388608 (8MB) - This is the maximum
 *       amount of bytes that the request buffer will grow to. If the body of
 * the request + headers cannot fit in this size the request body will be
 *       streamed in.
 *
 *   For more details see the documentation of the interface and the example
 *   below.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef __cplusplus
extern "C" {
#endif

// String type used to read the request details. The char pointer is NOT null
// terminated.
struct http_string_s;

struct http_server_s;
struct http_request_s;
struct http_response_s;

/**
 * Get the event loop descriptor that the server is running on.
 *
 * This will be an epoll fd when running on Linux or a kqueue on BSD. This can
 * be used to listen for activity on sockets, etc. The only caveat is that the
 * user data must be set to a struct where the first member is the function
 * pointer to a callback that will handle the event. i.e:
 *
 * For kevent:
 *
 *   struct foo {
 *     void (*handler)(struct kevent*);
 *     ...
 *   }
 *
 *   // Set ev.udata to a foo pointer when registering the event.
 *
 * For epoll:
 *
 *   struct foo {
 *     void (*handler)(struct epoll_event*);
 *     ...
 *   }
 *
 *   // Set ev.data.ptr to a foo pointer when registering the event.
 *
 * @param server The server.
 *
 * @return The descriptor of the event loop.
 */
int http_server_loop(struct http_server_s *server);

/**
 * Allocates and initializes the http server.
 *
 * @param port The port to listen on.
 * @param handler The callback that will fire to handle requests.
 *
 * @return Pointer to the allocated server.
 */
struct http_server_s *
http_server_init(int port, void (*handler)(struct http_request_s *));

/**
 * Listens on the server socket and starts an event loop.
 *
 * During normal operation this function will not return.
 *
 * @param server The server.
 * @param ipaddr The ip to bind to if NULL binds to all interfaces.
 *
 * @return Error code if the server fails.
 */
int http_server_listen_addr(struct http_server_s *server, const char *ipaddr);

/**
 * See http_server_listen_addr
 */
int http_server_listen(struct http_server_s *server);

/**
 * Poll the server socket on specific interface.
 *
 * Use this listen call in place of the one above when you want to integrate
 * an http server into an existing application that has a loop already and you
 * want to use the polling functionality instead. This works well for
 * applications like games that have a constant update loop.
 *
 * @param server The server.
 * @param ipaddr The ip to bind to if NULL bind to all.
 *
 * @return Error code if the poll fails.
 */
int http_server_listen_addr_poll(struct http_server_s *server,
                                 const char *ipaddr);

/**
 * Poll the server socket on all interfaces. See http_server_listen_addr_poll
 *
 * @param server The server.
 *
 * @return Error code if the poll fails.
 */
int http_server_listen_poll(struct http_server_s *server);
/**
 * Poll of the request sockets.
 *
 * Call this function in your update loop. It will trigger the request handler
 * once if there is a request ready. It should be called in a loop until it
 * returns 0.
 *
 * @param server The server.
 *
 * @return Returns 1 if a request was handled and 0 if no requests were handled.
 */
int http_server_poll(struct http_server_s *server);

/**
 * Check if a request flag is set.
 *
 * The flags that can be queried are listed below:
 *
 * HTTP_FLG_STREAMED
 *
 *   This flag will be set when the request body is chunked or the body is too
 *   large to fit in memory are once. This means that the
 *   http_request_read_chunk function must be used to read the body piece by
 *   piece.
 *
 * @param request The request.
 * @param flag One of the flags listed above.
 *
 * @return 1 or 0 if the flag is set or not respectively.
 */
int http_request_has_flag(struct http_request_s *request, int flag);

/**
 * Returns the request method as it was read from the HTTP request line.
 *
 * @param request The request.
 *
 * @return The HTTP method.
 */
struct http_string_s http_request_method(struct http_request_s *request);

/**
 * Returns the full request target (url) from the HTTP request line.
 *
 * @param request The request.
 *
 * @return The target.
 */
struct http_string_s http_request_target(struct http_request_s *request);

/**
 * Retrieves the request body.
 *
 * @param request The request.
 *
 * @return The request body. If no request body was sent buf and len of the
 *   string will be set to 0.
 */
struct http_string_s http_request_body(struct http_request_s *request);

/**
 * Returns the request header value for the given header key.
 *
 * @param request The request.
 * @param key The case insensitive header key to search for.
 *
 * @return The value for the header matching the key. Will be length 0 if not
 *   found.
 */
struct http_string_s http_request_header(struct http_request_s *request,
                                         char const *key);

/**
 * Iterate over the request headers.
 *
 * Each call will set key and val to the key and value of the next header.
 *
 * @param request The request.
 * @param[out] key The key of the header.
 * @param[out] value The key of the header.
 * @param[inout] iter Should be initialized to 0 before calling. Pass back in
 *   with each consecutive call.
 *
 * @return 0 when there are no more headers.
 */
int http_request_iterate_headers(struct http_request_s *request,
                                 struct http_string_s *key,
                                 struct http_string_s *val, int *iter);

/**
 * Stores an arbitrary userdata pointer for this request.
 *
 * This is not used by the library in any way and is strictly for you, the
 * application programmer to make use of.
 *
 * @param request The request.
 * @param data Opaque pointer to user data.
 */
void http_request_set_userdata(struct http_request_s *request, void *data);

/**
 * Retrieve the opaque data pointer that was set with http_request_set_userdata.
 *
 * @param request The request.
 */
void *http_request_userdata(struct http_request_s *request);

/**
 * Stores a server wide opaque pointer for future retrieval.
 *
 * This is not used by the library in any way and is strictly for you, the
 * application programmer to make use of.
 *
 * @param server The server.
 * @param data Opaque data pointer.
 */
void http_server_set_userdata(struct http_server_s *server, void *data);

/**
 * Retrieve the server wide userdata pointer.
 *
 * @param request The request.
 */
void *http_request_server_userdata(struct http_request_s *request);

/**
 * Sets how the request will handle it's connection
 *
 * By default the server will inspect the Connection header and the HTTP
 * version to determine whether the connection should be kept alive or not.
 * Use this function to override that behaviour to force the connection to
 * keep-alive or close by passing in the HTTP_KEEP_ALIVE or HTTP_CLOSE
 * directives respectively. This may provide a minor performance improvement
 * in cases where you control client and server and want to always close or
 * keep the connection alive.
 *
 * @param request The request.
 * @param directive One of HTTP_KEEP_ALIVE or HTTP_CLOSE
 */
void http_request_connection(struct http_request_s *request, int directive);

/**
 * Frees the buffer of a request.
 *
 * When reading in the HTTP request the server allocates a buffer to store
 * the request details such as the headers, method, body, etc. By default this
 * memory will be freed when http_respond is called. This function lets you
 * free that memory before the http_respond call. This can be useful if you
 * have requests that take a long time to complete and you don't require the
 * request data. Accessing any http_string_s's will be invalid after this call.
 *
 * @param request The request to free the buffer of.
 */
void http_request_free_buffer(struct http_request_s *request);

/**
 * Allocates an http response.
 *
 * This memory will be freed when http_respond is called.
 *
 * @return Allocated response.
 */
struct http_response_s *http_response_init();

/**
 * Set the response status.
 *
 * Accepts values between 100 and 599 inclusive. Any other value will map to
 * 500.
 *
 * @param response The response struct to set status on.
 * @param status The HTTP status code.
 */
void http_response_status(struct http_response_s *response, int status);

/**
 * Sets an HTTP response header.
 *
 * @param response The response struct to set the header on.
 * @param key The null-terminated key of the header eg: Content-Type
 * @param value The null-terminated value of the header eg: application/json
 */
void http_response_header(struct http_response_s *response, char const *key,
                          char const *value);

/**
 * Set the response body.
 *
 * The caller is responsible for freeing any memory that
 * may have been allocated for the body. It is safe to free this memory AFTER
 * http_respond has been called. If responding with chunked transfer encoding
 * this will become a single chunk. This procedure can be used again to set
 * subsequent chunks.
 *
 * @param response The response struct to set the body for.
 * @param body The body of the response.
 * @param length The length of the body
 */
void http_response_body(struct http_response_s *response, char const *body,
                        int length);

/**
 * Starts writing the response to the client.
 *
 * Any memory allocated for the response body or response headers is safe to
 * free after this call. Adds the default HTTP response headers, Date and
 * Connection.
 *
 * @param request The request to respond to.
 * @param response The response to respond with.
 */
void http_respond(struct http_request_s *request,
                  struct http_response_s *response);

/**
 * Writes a chunk to the client.
 *
 * The notify_done callback will be called when the write is complete. This call
 * consumes the response so a new response will need to be initialized for each
 * chunk. The response status of the request will be the response status that is
 * set when http_respond_chunk is called the first time. Any headers set for the
 * first call will be sent as the response headers. Transfer-Encoding header
 * will automatically be set to chunked. Headers set for subsequent calls will
 * be ignored.
 *
 * @param request The request to respond to.
 * @param response The response to respond with.
 * @param notify_done The callback that's used to signal user code that another
 *   chunk is ready to be written out.
 */
void http_respond_chunk(struct http_request_s *request,
                        struct http_response_s *response,
                        void (*notify_done)(struct http_request_s *));

/**
 * Ends the chunked response.
 *
 * Used to signal end of transmission on chunked requests. Any headers set
 * before this call will be included as what the HTTP spec refers to as
 * 'trailers' which are essentially more response headers.
 *
 * @param request The request to respond to.
 * @param response  The response to respond with.
 */
void http_respond_chunk_end(struct http_request_s *request,
                            struct http_response_s *response);

/**
 * Read a chunk of the request body.
 *
 * If a request has Transfer-Encoding: chunked or the body is too big to fit in
 * memory all at once you cannot read the body in the typical way. Instead you
 * need to call this function to read one chunk at a time. To check if the
 * request requires this type of reading you can call the http_request_has_flag
 * function to check if the HTTP_FLG_STREAMED flag is set. To read a streamed
 * body you pass a callback that will be called when the chunk is ready. When
 * the callback is called you can use 'http_request_chunk' to get the current
 * chunk. When done with that chunk call this function again to request the
 * next chunk. If the chunk has size 0 then the request body has been completely
 * read and you can now respond.
 *
 * @param request The request.
 * @param chunk_cb Callback for when the chunk is ready.
 */
void http_request_read_chunk(struct http_request_s *request,
                             void (*chunk_cb)(struct http_request_s *));

/**
 * Returns the current chunk of the request body.
 *
 * This chunk is only valid until the next call to 'http_request_read_chunk'.
 *
 * @param request The request.
 *
 * @return The chunk data.
 */
struct http_string_s http_request_chunk(struct http_request_s *request);

#define http_request_read_body http_request_read_chunk

#ifdef __cplusplus
}
#endif

// Minimal example usage.
#ifdef HTTPSERVER_EXAMPLE

#define RESPONSE "Hello, World!"

void handle_request(struct http_request_s *request) {
  struct http_response_s *response = http_response_init();
  http_response_status(response, 200);
  http_response_header(response, "Content-Type", "text/plain");
  http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  http_respond(request, response);
}

int main() {
  struct http_server_s *server = http_server_init(8080, handle_request);
  http_server_listen(server);
}

#endif

#endif

#line 1 "common.h"
#ifndef HS_COMMON_H
#define HS_COMMON_H

// http session states
#define HTTP_SESSION_INIT 0
#define HTTP_SESSION_READ 1
#define HTTP_SESSION_WRITE 2
#define HTTP_SESSION_NOP 3

#define HTTP_REQUEST_TIMEOUT 20

#define HTTP_FLAG_SET(var, flag) var |= flag
#define HTTP_FLAG_CLEAR(var, flag) var &= ~flag
#define HTTP_FLAG_CHECK(var, flag) (var & flag)

#define HTTP_AUTOMATIC 0x8
#define HTTP_CHUNKED_RESPONSE 0x20

#define HTTP_KEEP_ALIVE 1
#define HTTP_CLOSE 0

#include <arpa/inet.h>
#include <sys/socket.h>
#ifdef KQUEUE
#include <sys/event.h>
#else
#include <sys/epoll.h>
#endif

#ifdef EPOLL
typedef void (*epoll_cb_t)(struct epoll_event *);
#endif

typedef struct http_ev_cb_s {
#ifdef KQUEUE
  void (*handler)(struct kevent *ev);
#else
  epoll_cb_t handler;
#endif
} ev_cb_t;

struct hsh_buffer_s {
  char *buf;
  int32_t capacity;
  int32_t length;
  int32_t index;
  int32_t after_headers_index;
  int8_t sequence_id;
};

enum hsh_token_e {
  HSH_TOK_METHOD,
  HSH_TOK_TARGET,
  HSH_TOK_VERSION,
  HSH_TOK_HEADER_KEY,
  HSH_TOK_HEADER_VALUE,
  HSH_TOK_HEADERS_DONE,
  HSH_TOK_BODY,
  HSH_TOK_NONE,
  HSH_TOK_ERR
};

struct hsh_token_s {
  enum hsh_token_e type;
  uint8_t flags;
  int len;
  int index;
};

struct hsh_parser_s {
  int64_t content_length;
  int64_t content_remaining;
  struct hsh_token_s token;
  int16_t limit_count;
  int16_t limit_max;
  int8_t state;
  int8_t flags;
  int8_t sequence_id;
};

struct hs_token_array_s {
  struct hsh_token_s *buf;
  int capacity;
  int size;
};

typedef struct http_request_s {
#ifdef KQUEUE
  void (*handler)(struct kevent *ev);
#else
  epoll_cb_t handler;
  epoll_cb_t timer_handler;
  int timerfd;
#endif
  void (*chunk_cb)(struct http_request_s *);
  void *data;
  struct hsh_buffer_s buffer;
  struct hsh_parser_s parser;
  struct hs_token_array_s tokens;
  int state;
  int socket;
  int timeout;
  int64_t bytes_written;
  struct http_server_s *server;
  char flags;
} http_request_t;

typedef struct http_server_s {
#ifdef KQUEUE
  void (*handler)(struct kevent *ev);
#else
  epoll_cb_t handler;
  epoll_cb_t timer_handler;
#endif
  int64_t memused;
  int socket;
  int port;
  int loop;
  int timerfd;
  socklen_t len;
  void (*request_handler)(http_request_t *);
  struct sockaddr_in addr;
  void *data;
  char date[32];
} http_server_t;

#endif

#line 1 "buffer_util.h"
#ifndef HS_BUFFER_UTIL_H
#define HS_BUFFER_UTIL_H

#include <stdlib.h>

#ifndef HTTPSERVER_IMPL
#include "common.h"
#endif

static inline void _hs_buffer_free(struct hsh_buffer_s *buffer,
                                   int64_t *memused) {
  if (buffer->buf) {
    free(buffer->buf);
    *memused -= buffer->capacity;
    buffer->buf = NULL;
  }
}

#endif

#line 1 "request_util.h"
#ifndef HS_REQUEST_UTIL_H
#define HS_REQUEST_UTIL_H

#include "common.h"

// http version indicators
#define HTTP_1_0 0
#define HTTP_1_1 1

struct http_string_s {
  char const *buf;
  int len;
};

typedef struct http_string_s http_string_t;

http_string_t hs_get_token_string(http_request_t *request,
                                  enum hsh_token_e token_type);
http_string_t hs_request_header(http_request_t *request, char const *key);
void hs_request_detect_keep_alive_flag(http_request_t *request);
int hs_request_iterate_headers(http_request_t *request, http_string_t *key,
                               http_string_t *val, int *iter);
void hs_request_set_keep_alive_flag(http_request_t *request, int directive);
http_string_t hs_request_chunk(struct http_request_s *request);

#endif

#line 1 "parser.h"
#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

// HSH_TOK_HEADERS_DONE flags
#define HSH_TOK_FLAG_NO_BODY 0x1
#define HSH_TOK_FLAG_STREAMED_BODY 0x2

// HSH_TOK_BODY flags
#define HSH_TOK_FLAG_BODY_FINAL 0x1
#define HSH_TOK_FLAG_SMALL_BODY 0x2

struct hsh_token_s hsh_parser_exec(struct hsh_parser_s *parser,
                                   struct hsh_buffer_s *buffer,
                                   int max_buf_capacity);
void hsh_parser_init(struct hsh_parser_s *parser);

#endif

#line 1 "read_socket.h"
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

#line 1 "respond.h"
#ifndef HS_RESPOND_H
#define HS_RESPOND_H

#define HTTP_RESPONSE_BUF_SIZE 1024

struct http_request_s;

typedef void (*hs_req_fn_t)(struct http_request_s *);

// Represents a single header of an HTTP response.
typedef struct http_header_s {
  // The key of the header eg: Content-Type
  char const *key;
  // The value of the header eg: application/json
  char const *value;
  // Pointer to the next header in the linked list.
  struct http_header_s *next;
} http_header_t;

// Represents the response of an HTTP request before it is serialized on the
// wire.
typedef struct http_response_s {
  // Head of the linked list of response headers
  http_header_t *headers;
  // The complete body of the response or the chunk if generating a chunked
  // response.
  char const *body;
  // The length of the body or chunk.
  int content_length;
  // The HTTP status code for the response.
  int status;
} http_response_t;

http_response_t *hs_response_init();
void hs_response_set_header(http_response_t *response, char const *key,
                            char const *value);
void hs_response_set_status(http_response_t *response, int status);
void hs_response_set_body(http_response_t *response, char const *body,
                          int length);
void hs_request_respond(struct http_request_s *request,
                        http_response_t *response, hs_req_fn_t http_write);
void hs_request_respond_chunk(struct http_request_s *request,
                              http_response_t *response, hs_req_fn_t cb,
                              hs_req_fn_t http_write);
void hs_request_respond_chunk_end(struct http_request_s *request,
                                  http_response_t *response,
                                  hs_req_fn_t http_write);
void hs_request_respond_error(struct http_request_s *request, int code,
                              char const *message, hs_req_fn_t http_write);

#endif

#line 1 "server.h"
#ifndef HS_SERVER_H
#define HS_SERVER_H

#ifdef KQUEUE

struct kevent;

typedef void (*hs_evt_cb_t)(struct kevent *ev);

#else

struct epoll_event;

typedef void (*hs_evt_cb_t)(struct epoll_event *ev);

#endif

struct http_request_s;
struct http_server_s;

void hs_server_listen_on_addr(struct http_server_s *serv, const char *ipaddr);
int hs_server_run_event_loop(struct http_server_s *serv, const char *ipaddr);
void hs_generate_date_time(char *datetime);
struct http_server_s *hs_server_init(int port,
                                     void (*handler)(struct http_request_s *),
                                     hs_evt_cb_t accept_cb,
                                     hs_evt_cb_t timer_cb);
int hs_server_poll_events(struct http_server_s *serv);

#endif

#line 1 "write_socket.h"
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

#line 1 "connection.h"
#ifndef HS_CONNECTION_H
#define HS_CONNECTION_H

// Forward declarations
struct http_request_s;
struct http_server_s;

#ifdef KQUEUE
struct kevent;
typedef void (*hs_io_cb_t)(struct kevent *ev);
#else
struct epoll_event;
typedef void (*hs_io_cb_t)(struct epoll_event *ev);
#endif

/* Closes the requests socket and frees its resources.
 *
 * Removes all event watchers from the request socket and frees any allocated
 * buffers associated with the request struct.
 *
 * @param request The request to close
 */
void hs_request_terminate_connection(struct http_request_s *request);

/* Accepts connections on the server socket in a loop until it would block.
 *
 * When a connection is accepted a request struct is allocated and initialized
 * and the request socket is set to non-blocking mode. Event watchers are set
 * on the socket to call io_cb with a read/write ready event occurs. If the
 * server has reached max_mem_usage the err_responder function is called to
 * handle the issue.
 *
 * @param server The http server struct.
 * @param io_cb The callback function to respond to events on the request socket
 * @param epoll_timer_cb The callback function to respond to timer events for
 *   epoll. Can be NULL if not using epoll.
 * @param err_responder The procedure to call when memory usage has reached the
 *   given limit. Typically this could respond with a 503 error and close the
 *   connection.
 * @param max_mem_usage The limit at which err_responder should be called
 *   instead of regular operation.
 */
struct http_request_s *hs_server_accept_connection(struct http_server_s *server,
                                                   hs_io_cb_t io_cb,
                                                   hs_io_cb_t epoll_timer_cb);

#endif

#line 1 "io_events.h"
#ifndef HS_IO_EVENTS_H
#define HS_IO_EVENTS_H

#define HTTP_REQUEST_BUF_SIZE 1024
#define HTTP_MAX_REQUEST_BUF_SIZE 8388608       // 8mb
#define HTTP_MAX_TOTAL_EST_MEM_USAGE 4294967296 // 4gb

struct http_request_s;

void hs_request_begin_write(struct http_request_s *request);
void hs_request_begin_read(struct http_request_s *request);

#ifdef KQUEUE

struct kevent;

void hs_on_kqueue_server_event(struct kevent *ev);

#else

struct epoll_event;

void hs_on_epoll_server_connection_event(struct epoll_event *ev);
void hs_on_epoll_server_timer_event(struct epoll_event *ev);

#endif

#endif

#ifdef HTTPSERVER_IMPL
#ifndef HTTPSERVER_IMPL_ONCE
#define HTTPSERVER_IMPL_ONCE
#line 1 "api.c"
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

#line 1 "request_util.c"
#include <stdlib.h>
#include <string.h>

#ifndef HTTPSERVER_IMPL
#include "common.h"
#include "request_util.h"
#endif

int _hs_case_insensitive_cmp(char const *a, char const *b, int len) {
  for (int i = 0; i < len; i++) {
    char c1 = a[i] >= 'A' && a[i] <= 'Z' ? a[i] + 32 : a[i];
    char c2 = b[i] >= 'A' && b[i] <= 'Z' ? b[i] + 32 : b[i];
    if (c1 != c2)
      return 0;
  }
  return 1;
}

http_string_t hs_get_token_string(http_request_t *request,
                                  enum hsh_token_e token_type) {
  http_string_t str = {0, 0};
  if (request->tokens.buf == NULL)
    return str;
  for (int i = 0; i < request->tokens.size; i++) {
    struct hsh_token_s token = request->tokens.buf[i];
    if (token.type == token_type) {
      str.buf = &request->buffer.buf[token.index];
      str.len = token.len;
      return str;
    }
  }
  return str;
}

http_string_t hs_request_header(http_request_t *request, char const *key) {
  int len = strlen(key);
  for (int i = 0; i < request->tokens.size; i++) {
    struct hsh_token_s token = request->tokens.buf[i];
    if (token.type == HSH_TOK_HEADER_KEY && token.len == len) {
      if (_hs_case_insensitive_cmp(&request->buffer.buf[token.index], key,
                                   len)) {
        token = request->tokens.buf[i + 1];
        return (http_string_t){.buf = &request->buffer.buf[token.index],
                               .len = token.len};
      }
    }
  }
  return (http_string_t){};
}

void hs_request_detect_keep_alive_flag(http_request_t *request) {
  http_string_t str = hs_get_token_string(request, HSH_TOK_VERSION);
  if (str.buf == NULL)
    return;
  int version = str.buf[str.len - 1] == '1';
  str = hs_request_header(request, "Connection");
  if ((str.len == 5 && _hs_case_insensitive_cmp(str.buf, "close", 5)) ||
      (str.len == 0 && version == HTTP_1_0)) {
    HTTP_FLAG_CLEAR(request->flags, HTTP_KEEP_ALIVE);
  } else {
    HTTP_FLAG_SET(request->flags, HTTP_KEEP_ALIVE);
  }
}

int _hs_get_header_key_val(http_request_t *request, http_string_t *key,
                           http_string_t *val, int iter) {
  struct hsh_token_s token = request->tokens.buf[iter];
  if (request->tokens.buf[iter].type == HSH_TOK_HEADERS_DONE)
    return 0;
  *key = (http_string_t){.buf = &request->buffer.buf[token.index],
                         .len = token.len};
  token = request->tokens.buf[iter + 1];
  *val = (http_string_t){.buf = &request->buffer.buf[token.index],
                         .len = token.len};
  return 1;
}

int hs_request_iterate_headers(http_request_t *request, http_string_t *key,
                               http_string_t *val, int *iter) {
  if (*iter == 0) {
    for (; *iter < request->tokens.size; (*iter)++) {
      struct hsh_token_s token = request->tokens.buf[*iter];
      if (token.type == HSH_TOK_HEADER_KEY) {
        int more = _hs_get_header_key_val(request, key, val, *iter);
        (*iter)++;
        return more;
      }
    }
    return 0;
  } else {
    (*iter)++;
    int more = _hs_get_header_key_val(request, key, val, *iter);
    (*iter)++;
    return more;
  }
}

void hs_request_set_keep_alive_flag(http_request_t *request, int directive) {
  if (directive == HTTP_KEEP_ALIVE) {
    HTTP_FLAG_CLEAR(request->flags, HTTP_AUTOMATIC);
    HTTP_FLAG_SET(request->flags, HTTP_KEEP_ALIVE);
  } else if (directive == HTTP_CLOSE) {
    HTTP_FLAG_CLEAR(request->flags, HTTP_AUTOMATIC);
    HTTP_FLAG_CLEAR(request->flags, HTTP_KEEP_ALIVE);
  }
}

http_string_t hs_request_chunk(struct http_request_s *request) {
  struct hsh_token_s token = request->tokens.buf[request->tokens.size - 1];
  return (http_string_t){.buf = &request->buffer.buf[token.index],
                         .len = token.len};
}

#line 1 "parser.c"

#line 1 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
#include <string.h>
#include <stdlib.h>

#ifndef HTTPSERVER_IMPL
#include "common.h"
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


#line 232 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"



#line 28 "/Users/jeremywilliams/code/httpserver.h/build/src/parser.c"
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


#line 235 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"

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
  
#line 373 "/Users/jeremywilliams/code/httpserver.h/build/src/parser.c"
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
#line 23 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{ HSH_ENTER_TOKEN(HSH_TOK_METHOD, 32) }
	break;
	case 1:
#line 24 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{ HSH_ENTER_TOKEN(HSH_TOK_TARGET, 1024) }
	break;
	case 2:
#line 25 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{ HSH_ENTER_TOKEN(HSH_TOK_VERSION, 16) }
	break;
	case 3:
#line 26 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{ HSH_ENTER_TOKEN(HSH_TOK_HEADER_KEY, 256) }
	break;
	case 4:
#line 27 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{ HSH_ENTER_TOKEN(HSH_TOK_HEADER_VALUE, 4096) }
	break;
	case 5:
#line 28 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{
    parser->token.type = HSH_TOK_BODY;
    parser->token.flags = 0;
    parser->token.index = p - buffer->buf;
  }
	break;
	case 6:
#line 33 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{
    parser->token.len = p - (buffer->buf + parser->token.index);
    // hsh_token_array_push(&parser->tokens, parser->token);
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
    {p++; goto _out; }
  }
	break;
	case 7:
#line 40 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{
    parser->content_length *= 10;
    parser->content_length += (*p) - '0';
  }
	break;
	case 8:
#line 45 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{
    HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_CHUNKED);
  }
	break;
	case 9:
#line 49 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{
    parser->limit_count = 0;
    parser->limit_max = 256;
  }
	break;
	case 10:
#line 54 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{
    parser->limit_count++;
    if (parser->limit_count > parser->limit_max) {
      // parser->rc = (int8_t)HSH_PARSER_ERR;
      {p++; goto _out; }
    }
  }
	break;
	case 11:
#line 62 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
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
        buffer->buf = (char*)realloc(buffer->buf, parser->content_length + buffer->after_headers_index);
        buffer->capacity = parser->content_length + buffer->after_headers_index;
      }
      cs = 88;
      {p++; goto _out; }
    }
  }
	break;
	case 12:
#line 91 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{
    parser->content_length = 0;
  }
	break;
	case 13:
#line 95 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
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
#line 108 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
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
#line 130 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
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
#line 141 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{
    parser->token.index = buffer->after_headers_index;
    parser->token.len = parser->content_length;
    HTTP_FLAG_SET(parser->token.flags, HSH_TOK_FLAG_SMALL_BODY);
    char* last_body_byte = buffer->buf + parser->token.index + parser->content_length - 1;
    if (pe >= last_body_byte) {
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_DONE);
    }
    p = pe;
    p--;
    {p++; goto _out; }
  }
	break;
	case 17:
#line 155 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{
    parser->token.index = buffer->after_headers_index;
    char* last_body_byte = buffer->buf + buffer->after_headers_index + parser->content_remaining - 1;
    if (pe >= last_body_byte) {
      parser->token.flags = HSH_TOK_FLAG_BODY_FINAL;
      parser->token.len = parser->content_remaining;
      parser->content_remaining = 0;
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_DONE);
    } else {
      parser->token.len = pe - p;
      parser->content_remaining -= parser->token.len;
      HTTP_FLAG_SET(parser->flags, HSH_P_FLAG_TOKEN_READY);
      p = buffer->buf + buffer->after_headers_index;
      buffer->length = buffer->after_headers_index;
      parser->sequence_id = buffer->sequence_id;
    }
    p--;
    {p++; goto _out; }
  }
	break;
	case 18:
#line 176 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{
    // parser->rc = (int8_t)HSH_PARSER_ERR;
    {p++; goto _out; }
  }
	break;
#line 649 "/Users/jeremywilliams/code/httpserver.h/build/src/parser.c"
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
#line 176 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
	{
    // parser->rc = (int8_t)HSH_PARSER_ERR;
    {p++; goto _out; }
  }
	break;
#line 672 "/Users/jeremywilliams/code/httpserver.h/build/src/parser.c"
		}
	}
	}

	_out: {}
	}

#line 252 "/Users/jeremywilliams/code/httpserver.h/src/parser.rl"
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

#line 1 "read_socket.c"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef HTTPSERVER_IMPL
#include "common.h"
#include "parser.h"
#include "read_socket.h"
#endif

void _hs_token_array_push(struct hs_token_array_s *array,
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

void _hs_buffer_init(struct hsh_buffer_s *buffer, int initial_capacity,
                     int64_t *memused) {
  *buffer = (struct hsh_buffer_s){0};
  buffer->buf = (char *)calloc(1, initial_capacity);
  *memused += initial_capacity;
  assert(buffer->buf != NULL);
  buffer->capacity = initial_capacity;
}

int _hs_read_into_buffer(struct hsh_buffer_s *buffer, int request_socket,
                         int64_t *server_memused,
                         int64_t max_request_buf_capacity) {
  int bytes;
  do {
    bytes = read(request_socket, buffer->buf + buffer->length,
                 buffer->capacity - buffer->length);
    if (bytes > 0)
      buffer->length += bytes;

    if (buffer->length == buffer->capacity &&
        buffer->capacity != max_request_buf_capacity) {
      *server_memused -= buffer->capacity;
      buffer->capacity *= 2;
      if (buffer->capacity > max_request_buf_capacity) {
        buffer->capacity = max_request_buf_capacity;
      }
      *server_memused += buffer->capacity;
      buffer->buf = (char *)realloc(buffer->buf, buffer->capacity);
      assert(buffer->buf != NULL);
    }
  } while (bytes > 0 && buffer->capacity < max_request_buf_capacity);

  buffer->sequence_id++;

  return bytes;
}

int _hs_buffer_requires_read(struct hsh_buffer_s *buffer) {
  return buffer->index >= buffer->length;
}

void _hs_exec_callback(http_request_t *request,
                       void (*cb)(struct http_request_s *)) {
  request->state = HTTP_SESSION_NOP;
  cb(request);
}

enum hs_read_rc_e
_hs_parse_buffer_and_exec_user_cb(http_request_t *request,
                                  int max_request_buf_capacity) {
  enum hs_read_rc_e rc = HS_READ_RC_SUCCESS;

  do {
    struct hsh_token_s token = hsh_parser_exec(
        &request->parser, &request->buffer, max_request_buf_capacity);

    switch (token.type) {
    case HSH_TOK_HEADERS_DONE:
      _hs_token_array_push(&request->tokens, token);
      if (HTTP_FLAG_CHECK(token.flags, HSH_TOK_FLAG_STREAMED_BODY) ||
          HTTP_FLAG_CHECK(token.flags, HSH_TOK_FLAG_NO_BODY)) {
        HTTP_FLAG_SET(request->flags, HTTP_FLG_STREAMED);
        _hs_exec_callback(request, request->server->request_handler);
        return rc;
      }
      break;
    case HSH_TOK_BODY:
      _hs_token_array_push(&request->tokens, token);
      if (HTTP_FLAG_CHECK(token.flags, HSH_TOK_FLAG_SMALL_BODY)) {
        _hs_exec_callback(request, request->server->request_handler);
      } else {
        if (HTTP_FLAG_CHECK(token.flags, HSH_TOK_FLAG_BODY_FINAL) &&
            token.len > 0) {
          _hs_exec_callback(request, request->chunk_cb);

          // A zero length body is used to indicate to the user code that the
          // body has finished streaming. This is natural when dealing with
          // chunked request bodies but requires us to inject a zero length
          // body for non-chunked requests.
          struct hsh_token_s token = {};
          memset(&token, 0, sizeof(struct hsh_token_s));
          token.type = HSH_TOK_BODY;
          _hs_token_array_push(&request->tokens, token);
          _hs_exec_callback(request, request->chunk_cb);
        } else {
          _hs_exec_callback(request, request->chunk_cb);
        }
      }
      return rc;
    case HSH_TOK_ERR:
      return HS_READ_RC_PARSE_ERR;
    case HSH_TOK_NONE:
      return rc;
    default:
      _hs_token_array_push(&request->tokens, token);
      break;
    }
  } while (1);
}

// Reads the request socket if required and parses HTTP in a non-blocking
// manner.
//
// It should be called when a new connection is established and when a read
// ready event occurs for the request socket. It parses the HTTP request and
// fills the tokens array of the request struct. It will also invoke the
// request_hander callback and the chunk_cb callback in the appropriate
// scenarios.
enum hs_read_rc_e hs_read_request_and_exec_user_cb(http_request_t *request,
                                                   struct hs_read_opts_s opts) {
  request->state = HTTP_SESSION_READ;
  request->timeout = HTTP_REQUEST_TIMEOUT;

  if (request->buffer.buf == NULL) {
    _hs_buffer_init(&request->buffer, opts.initial_request_buf_capacity,
                    &request->server->memused);
    hsh_parser_init(&request->parser);
  }

  if (_hs_buffer_requires_read(&request->buffer)) {
    int bytes = _hs_read_into_buffer(&request->buffer, request->socket,
                                     &request->server->memused,
                                     opts.max_request_buf_capacity);

    if (bytes == opts.eof_rc) {
      return HS_READ_RC_SOCKET_ERR;
    }
  }

  return _hs_parse_buffer_and_exec_user_cb(request,
                                           opts.max_request_buf_capacity);
}

#line 1 "respond.c"
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HTTPSERVER_IMPL
#include "buffer_util.h"
#include "common.h"
#include "request_util.h"
#include "respond.h"
#endif

char const *hs_status_text[] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "",

    // 100s
    "Continue", "Switching Protocols", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "",

    // 200s
    "OK", "Created", "Accepted", "Non-Authoritative Information", "No Content",
    "Reset Content", "Partial Content", "", "", "",

    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "",

    // 300s
    "Multiple Choices", "Moved Permanently", "Found", "See Other",
    "Not Modified", "Use Proxy", "", "Temporary Redirect", "", "",

    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "",

    // 400s
    "Bad Request", "Unauthorized", "Payment Required", "Forbidden", "Not Found",
    "Method Not Allowed", "Not Acceptable", "Proxy Authentication Required",
    "Request Timeout", "Conflict",

    "Gone", "Length Required", "", "Payload Too Large", "", "", "", "", "", "",

    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "",

    // 500s
    "Internal Server Error", "Not Implemented", "Bad Gateway",
    "Service Unavailable", "Gateway Timeout", "", "", "", "", "",

    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", ""};
typedef struct {
  char *buf;
  int capacity;
  int size;
  int64_t *memused;
} grwprintf_t;

void _grwprintf_init(grwprintf_t *ctx, int capacity, int64_t *memused) {
  ctx->memused = memused;
  ctx->size = 0;
  ctx->buf = (char *)malloc(capacity);
  *ctx->memused += capacity;
  assert(ctx->buf != NULL);
  ctx->capacity = capacity;
}

void _grwmemcpy(grwprintf_t *ctx, char const *src, int size) {
  if (ctx->size + size > ctx->capacity) {
    *ctx->memused -= ctx->capacity;
    ctx->capacity = ctx->size + size;
    *ctx->memused += ctx->capacity;
    ctx->buf = (char *)realloc(ctx->buf, ctx->capacity);
    assert(ctx->buf != NULL);
  }
  memcpy(ctx->buf + ctx->size, src, size);
  ctx->size += size;
}

void _grwprintf(grwprintf_t *ctx, char const *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  int bytes =
      vsnprintf(ctx->buf + ctx->size, ctx->capacity - ctx->size, fmt, args);
  if (bytes + ctx->size > ctx->capacity) {
    *ctx->memused -= ctx->capacity;
    while (bytes + ctx->size > ctx->capacity)
      ctx->capacity *= 2;
    *ctx->memused += ctx->capacity;
    ctx->buf = (char *)realloc(ctx->buf, ctx->capacity);
    assert(ctx->buf != NULL);
    bytes +=
        vsnprintf(ctx->buf + ctx->size, ctx->capacity - ctx->size, fmt, args);
  }
  ctx->size += bytes;

  va_end(args);
}

void _http_serialize_headers_list(http_response_t *response,
                                  grwprintf_t *printctx) {
  http_header_t *header = response->headers;
  while (header) {
    _grwprintf(printctx, "%s: %s\r\n", header->key, header->value);
    header = header->next;
  }
  _grwprintf(printctx, "\r\n");
}

void _http_serialize_headers(http_request_t *request, http_response_t *response,
                             grwprintf_t *printctx) {
  if (HTTP_FLAG_CHECK(request->flags, HTTP_AUTOMATIC)) {
    hs_request_detect_keep_alive_flag(request);
  }
  if (HTTP_FLAG_CHECK(request->flags, HTTP_KEEP_ALIVE)) {
    hs_response_set_header(response, "Connection", "keep-alive");
  } else {
    hs_response_set_header(response, "Connection", "close");
  }
  _grwprintf(printctx, "HTTP/1.1 %d %s\r\nDate: %s\r\n", response->status,
             hs_status_text[response->status], request->server->date);
  if (!HTTP_FLAG_CHECK(request->flags, HTTP_CHUNKED_RESPONSE)) {
    _grwprintf(printctx, "Content-Length: %d\r\n", response->content_length);
  }
  _http_serialize_headers_list(response, printctx);
}

void _http_perform_response(http_request_t *request, http_response_t *response,
                            grwprintf_t *printctx, hs_req_fn_t http_write) {
  http_header_t *header = response->headers;
  while (header) {
    http_header_t *tmp = header;
    header = tmp->next;
    free(tmp);
  }
  _hs_buffer_free(&request->buffer, &request->server->memused);
  free(response);
  request->buffer.buf = printctx->buf;
  request->buffer.length = printctx->size;
  request->buffer.capacity = printctx->capacity;
  request->bytes_written = 0;
  request->state = HTTP_SESSION_WRITE;
  http_write(request);
}

// See api.h http_response_header
void hs_response_set_header(http_response_t *response, char const *key,
                            char const *value) {
  http_header_t *header = (http_header_t *)malloc(sizeof(http_header_t));
  assert(header != NULL);
  header->key = key;
  header->value = value;
  http_header_t *prev = response->headers;
  header->next = prev;
  response->headers = header;
}

// Serializes the response into the request buffer and calls http_write.
// See api.h http_respond for more details
void hs_request_respond(http_request_t *request, http_response_t *response,
                        hs_req_fn_t http_write) {
  grwprintf_t printctx;
  _grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE, &request->server->memused);
  _http_serialize_headers(request, response, &printctx);
  if (response->body) {
    _grwmemcpy(&printctx, response->body, response->content_length);
  }
  _http_perform_response(request, response, &printctx, http_write);
}

// Serializes a chunk into the request buffer and calls http_write.
// See api.h http_respond_chunk for more details.
void hs_request_respond_chunk(http_request_t *request,
                              http_response_t *response, hs_req_fn_t cb,
                              hs_req_fn_t http_write) {
  grwprintf_t printctx;
  _grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE, &request->server->memused);
  if (!HTTP_FLAG_CHECK(request->flags, HTTP_CHUNKED_RESPONSE)) {
    HTTP_FLAG_SET(request->flags, HTTP_CHUNKED_RESPONSE);
    hs_response_set_header(response, "Transfer-Encoding", "chunked");
    _http_serialize_headers(request, response, &printctx);
  }
  request->chunk_cb = cb;
  _grwprintf(&printctx, "%X\r\n", response->content_length);
  _grwmemcpy(&printctx, response->body, response->content_length);
  _grwprintf(&printctx, "\r\n");
  _http_perform_response(request, response, &printctx, http_write);
}

// Serializes the zero sized final chunk into the request buffer and calls
// http_write. See api.h http_respond_chunk_end for more details.
void hs_request_respond_chunk_end(http_request_t *request,
                                  http_response_t *response,
                                  hs_req_fn_t http_write) {
  grwprintf_t printctx;
  _grwprintf_init(&printctx, HTTP_RESPONSE_BUF_SIZE, &request->server->memused);
  _grwprintf(&printctx, "0\r\n");
  _http_serialize_headers_list(response, &printctx);
  _grwprintf(&printctx, "\r\n");
  HTTP_FLAG_CLEAR(request->flags, HTTP_CHUNKED_RESPONSE);
  _http_perform_response(request, response, &printctx, http_write);
}

// See api.h http_response_status
void hs_response_set_status(http_response_t *response, int status) {
  response->status = status > 599 || status < 100 ? 500 : status;
}

// See api.h http_response_body
void hs_response_set_body(http_response_t *response, char const *body,
                          int length) {
  response->body = body;
  response->content_length = length;
}

// See api.h http_response_init
http_response_t *hs_response_init() {
  http_response_t *response =
      (http_response_t *)calloc(1, sizeof(http_response_t));
  assert(response != NULL);
  response->status = 200;
  return response;
}

// Simple less flexible interface for responses, used for errors.
void hs_request_respond_error(http_request_t *request, int code,
                              char const *message, hs_req_fn_t http_write) {
  struct http_response_s *response = hs_response_init();
  hs_response_set_status(response, code);
  hs_response_set_header(response, "Content-Type", "text/plain");
  hs_response_set_body(response, message, strlen(message));
  hs_request_respond(request, response, http_write);
  http_write(request);
}

#line 1 "server.c"
#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>

#ifdef EPOLL
#include <sys/epoll.h>
#include <sys/timerfd.h>
#else
#include <sys/event.h>
#endif

#ifndef HTTPSERVER_IMPL
#include "common.h"
#include "server.h"
#endif

void _hs_bind_localhost(int s, struct sockaddr_in *addr, const char *ipaddr,
                        int port) {
  addr->sin_family = AF_INET;
  if (ipaddr == NULL) {
    addr->sin_addr.s_addr = INADDR_ANY;
  } else {
    addr->sin_addr.s_addr = inet_addr(ipaddr);
  }
  addr->sin_port = htons(port);
  int rc = bind(s, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
  if (rc < 0) {
    exit(1);
  }
}

#ifdef KQUEUE

void _hs_add_server_sock_events(http_server_t *serv) {
  struct kevent ev_set;
  EV_SET(&ev_set, serv->socket, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, serv);
  kevent(serv->loop, &ev_set, 1, NULL, 0, NULL);
}

void _hs_server_init_events(http_server_t *serv, hs_evt_cb_t unused) {
  (void)unused;

  serv->loop = kqueue();
  struct kevent ev_set;
  EV_SET(&ev_set, 1, EVFILT_TIMER, EV_ADD | EV_ENABLE, NOTE_SECONDS, 1, serv);
  kevent(serv->loop, &ev_set, 1, NULL, 0, NULL);
}

int hs_server_run_event_loop(http_server_t *serv, const char *ipaddr) {
  hs_server_listen_on_addr(serv, ipaddr);

  struct kevent ev_list[1];

  while (1) {
    int nev = kevent(serv->loop, NULL, 0, ev_list, 1, NULL);
    for (int i = 0; i < nev; i++) {
      ev_cb_t *ev_cb = (ev_cb_t *)ev_list[i].udata;
      ev_cb->handler(&ev_list[i]);
    }
  }
  return 0;
}

int hs_server_poll_events(http_server_t *serv) {
  struct kevent ev;
  struct timespec ts = {0, 0};
  int nev = kevent(serv->loop, NULL, 0, &ev, 1, &ts);
  if (nev <= 0)
    return nev;
  ev_cb_t *ev_cb = (ev_cb_t *)ev.udata;
  ev_cb->handler(&ev);
  return nev;
}

#else

void _hs_server_init_events(http_server_t *serv, hs_evt_cb_t timer_cb) {
  serv->loop = epoll_create1(0);
  serv->timer_handler = timer_cb;

  int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
  struct itimerspec ts = {};
  ts.it_value.tv_sec = 1;
  ts.it_interval.tv_sec = 1;
  timerfd_settime(tfd, 0, &ts, NULL);

  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = &serv->timer_handler;
  epoll_ctl(serv->loop, EPOLL_CTL_ADD, tfd, &ev);
  serv->timerfd = tfd;
}

void _hs_add_server_sock_events(http_server_t *serv) {
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = serv;
  epoll_ctl(serv->loop, EPOLL_CTL_ADD, serv->socket, &ev);
}

int hs_server_run_event_loop(http_server_t *serv, const char *ipaddr) {
  hs_server_listen_on_addr(serv, ipaddr);
  struct epoll_event ev_list[1];
  while (1) {
    int nev = epoll_wait(serv->loop, ev_list, 1, -1);
    for (int i = 0; i < nev; i++) {
      ev_cb_t *ev_cb = (ev_cb_t *)ev_list[i].data.ptr;
      ev_cb->handler(&ev_list[i]);
    }
  }
  return 0;
}

int hs_server_poll_events(http_server_t *serv) {
  struct epoll_event ev;
  int nev = epoll_wait(serv->loop, &ev, 1, 0);
  if (nev <= 0)
    return nev;
  ev_cb_t *ev_cb = (ev_cb_t *)ev.data.ptr;
  ev_cb->handler(&ev);
  return nev;
}

#endif

void hs_server_listen_on_addr(http_server_t *serv, const char *ipaddr) {
  // Ignore SIGPIPE. We handle these errors at the call site.
  signal(SIGPIPE, SIG_IGN);
  serv->socket = socket(AF_INET, SOCK_STREAM, 0);
  int flag = 1;
  setsockopt(serv->socket, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag));
  _hs_bind_localhost(serv->socket, &serv->addr, ipaddr, serv->port);
  serv->len = sizeof(serv->addr);
  int flags = fcntl(serv->socket, F_GETFL, 0);
  fcntl(serv->socket, F_SETFL, flags | O_NONBLOCK);
  listen(serv->socket, 128);
  _hs_add_server_sock_events(serv);
}

void hs_generate_date_time(char *datetime) {
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = gmtime(&rawtime);
  strftime(datetime, 32, "%a, %d %b %Y %T GMT", timeinfo);
}

http_server_t *hs_server_init(int port, void (*handler)(http_request_t *),
                              hs_evt_cb_t accept_cb,
                              hs_evt_cb_t epoll_timer_cb) {
  http_server_t *serv = (http_server_t *)malloc(sizeof(http_server_t));
  assert(serv != NULL);
  serv->port = port;
  serv->memused = 0;
  serv->handler = accept_cb;
  _hs_server_init_events(serv, epoll_timer_cb);
  hs_generate_date_time(serv->date);
  serv->request_handler = handler;
  return serv;
}

#line 1 "write_socket.c"
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

#line 1 "connection.c"
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef KQUEUE
#include <sys/event.h>
#else
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#endif

#ifndef HTTPSERVER_IMPL
#include "buffer_util.h"
#include "common.h"
#include "connection.h"
#endif

#ifdef KQUEUE

void _hs_delete_events(http_request_t *request) {
  struct kevent ev_set;
  EV_SET(&ev_set, request->socket, EVFILT_TIMER, EV_DELETE, 0, 0, request);
  kevent(request->server->loop, &ev_set, 1, NULL, 0, NULL);
}

void _hs_add_timer_event(http_request_t *request, hs_io_cb_t unused) {
  (void)unused;

  struct kevent ev_set;
  EV_SET(&ev_set, request->socket, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, 1000,
         request);
  kevent(request->server->loop, &ev_set, 1, NULL, 0, NULL);
}

#else

void _hs_delete_events(http_request_t *request) {
  epoll_ctl(request->server->loop, EPOLL_CTL_DEL, request->socket, NULL);
  epoll_ctl(request->server->loop, EPOLL_CTL_DEL, request->timerfd, NULL);
  close(request->timerfd);
}

void _hs_add_timer_event(http_request_t *request, hs_io_cb_t timer_cb) {
  request->timer_handler = timer_cb;

  // Watch for read events
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = request;
  epoll_ctl(request->server->loop, EPOLL_CTL_ADD, request->socket, &ev);

  // Add timer to timeout requests.
  int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
  struct itimerspec ts = {};
  ts.it_value.tv_sec = 1;
  ts.it_interval.tv_sec = 1;
  timerfd_settime(tfd, 0, &ts, NULL);

  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = &request->timer_handler;
  epoll_ctl(request->server->loop, EPOLL_CTL_ADD, tfd, &ev);
  request->timerfd = tfd;
}

#endif

void hs_request_terminate_connection(http_request_t *request) {
  _hs_delete_events(request);
  close(request->socket);
  _hs_buffer_free(&request->buffer, &request->server->memused);
  free(request->tokens.buf);
  request->tokens.buf = NULL;
  free(request);
}

void _hs_token_array_init(struct hs_token_array_s *array, int capacity) {
  array->buf =
      (struct hsh_token_s *)malloc(sizeof(struct hsh_token_s) * capacity);
  assert(array->buf != NULL);
  array->size = 0;
  array->capacity = capacity;
}

http_request_t *_hs_request_init(int sock, http_server_t *server,
                                 hs_io_cb_t io_cb) {
  http_request_t *request = (http_request_t *)calloc(1, sizeof(http_request_t));
  assert(request != NULL);
  request->socket = sock;
  request->server = server;
  request->handler = io_cb;
  request->timeout = HTTP_REQUEST_TIMEOUT;
  request->flags = HTTP_AUTOMATIC;
  request->parser = (struct hsh_parser_s){};
  request->buffer = (struct hsh_buffer_s){};
  request->tokens.buf = NULL;
  _hs_token_array_init(&request->tokens, 32);
  return request;
}

http_request_t *hs_server_accept_connection(http_server_t *server,
                                            hs_io_cb_t io_cb,
                                            hs_io_cb_t epoll_timer_cb) {
  http_request_t *request = NULL;
  int sock = 0;

  sock = accept(server->socket, (struct sockaddr *)&server->addr, &server->len);

  if (sock > 0) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    request = _hs_request_init(sock, server, io_cb);
    _hs_add_timer_event(request, epoll_timer_cb);
  }
  return request;
}

#line 1 "io_events.c"
#include <stdlib.h>

#ifdef KQUEUE
#include <sys/event.h>
#else
#include <stdint.h>
#include <sys/epoll.h>
#include <unistd.h>
#endif

#ifndef HTTPSERVER_IMPL
#include "buffer_util.h"
#include "common.h"
#include "connection.h"
#include "io_events.h"
#include "read_socket.h"
#include "respond.h"
#include "server.h"
#include "write_socket.h"
#endif

void _hs_read_socket_and_handle_return_code(http_request_t *request) {
  struct hs_read_opts_s opts;
  opts.initial_request_buf_capacity = HTTP_REQUEST_BUF_SIZE;
  opts.max_request_buf_capacity = HTTP_MAX_REQUEST_BUF_SIZE;
  opts.eof_rc = 0;

  enum hs_read_rc_e rc = hs_read_request_and_exec_user_cb(request, opts);
  switch (rc) {
  case HS_READ_RC_PARSE_ERR:
    hs_request_respond_error(request, 400, "Bad Request",
                             hs_request_begin_write);
    break;
  case HS_READ_RC_SOCKET_ERR:
    hs_request_terminate_connection(request);
    break;
  case HS_READ_RC_SUCCESS:
    break;
  }
}

void hs_request_begin_read(http_request_t *request);

void _hs_write_socket_and_handle_return_code(http_request_t *request) {
  enum hs_write_rc_e rc = hs_write_socket(request);

  request->timeout = rc == HS_WRITE_RC_SUCCESS ? HTTP_KEEP_ALIVE_TIMEOUT
                                               : HTTP_REQUEST_TIMEOUT;

  if (rc != HS_WRITE_RC_CONTINUE)
    _hs_buffer_free(&request->buffer, &request->server->memused);

  switch (rc) {
  case HS_WRITE_RC_SUCCESS_CLOSE:
  case HS_WRITE_RC_SOCKET_ERR:
    // Error or response complete, connection: close
    hs_request_terminate_connection(request);
    break;
  case HS_WRITE_RC_SUCCESS:
    // Response complete, keep-alive connection
    hs_request_begin_read(request);
    break;
  case HS_WRITE_RC_SUCCESS_CHUNK:
    // Finished writing chunk, request next
    request->state = HTTP_SESSION_NOP;
    request->chunk_cb(request);
    break;
  case HS_WRITE_RC_CONTINUE:
    break;
  }
}

void _hs_accept_and_begin_request_cycle(http_server_t *server,
                                        hs_io_cb_t on_client_connection_cb,
                                        hs_io_cb_t on_timer_event_cb) {
  http_request_t *request = NULL;
  while ((request = hs_server_accept_connection(server, on_client_connection_cb,
                                                on_timer_event_cb))) {
    if (server->memused > HTTP_MAX_TOTAL_EST_MEM_USAGE) {
      hs_request_respond_error(request, 503, "Service Unavailable",
                               hs_request_begin_write);
    } else {
      hs_request_begin_read(request);
    }
  }
}

#ifdef KQUEUE

void _hs_on_kqueue_client_connection_event(struct kevent *ev) {
  http_request_t *request = (http_request_t *)ev->udata;
  if (ev->filter == EVFILT_TIMER) {
    request->timeout -= 1;
    if (request->timeout == 0)
      hs_request_terminate_connection(request);
  } else {
    if (request->state == HTTP_SESSION_READ) {
      _hs_read_socket_and_handle_return_code(request);
    } else if (request->state == HTTP_SESSION_WRITE) {
      _hs_write_socket_and_handle_return_code(request);
    }
  }
}

void hs_on_kqueue_server_event(struct kevent *ev) {
  http_server_t *server = (http_server_t *)ev->udata;
  if (ev->filter == EVFILT_TIMER) {
    hs_generate_date_time(server->date);
  } else {
    _hs_accept_and_begin_request_cycle(
        server, _hs_on_kqueue_client_connection_event, NULL);
  }
}

#else

void _hs_on_epoll_client_connection_event(struct epoll_event *ev) {
  http_request_t *request = (http_request_t *)ev->data.ptr;
  if (request->state == HTTP_SESSION_READ) {
    _hs_read_socket_and_handle_return_code(request);
  } else if (request->state == HTTP_SESSION_WRITE) {
    _hs_write_socket_and_handle_return_code(request);
  }
}

void _hs_on_epoll_request_timer_event(struct epoll_event *ev) {
  http_request_t *request =
      (http_request_t *)((char *)ev->data.ptr - sizeof(epoll_cb_t));
  uint64_t res;
  int bytes = read(request->timerfd, &res, sizeof(res));
  (void)bytes; // suppress warning
  request->timeout -= 1;
  if (request->timeout == 0)
    hs_request_terminate_connection(request);
}

void hs_on_epoll_server_connection_event(struct epoll_event *ev) {
  _hs_accept_and_begin_request_cycle((http_server_t *)ev->data.ptr,
                                     _hs_on_epoll_client_connection_event,
                                     _hs_on_epoll_request_timer_event);
}

void hs_on_epoll_server_timer_event(struct epoll_event *ev) {
  http_server_t *server =
      (http_server_t *)((char *)ev->data.ptr - sizeof(epoll_cb_t));
  uint64_t res;
  int bytes = read(server->timerfd, &res, sizeof(res));
  (void)bytes; // suppress warning
  hs_generate_date_time(server->date);
}

#endif

void _hs_add_write_event(http_request_t *request) {
#ifdef KQUEUE
  struct kevent ev_set[2];
  EV_SET(&ev_set[0], request->socket, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0,
         request);
  EV_SET(&ev_set[1], request->socket, EVFILT_READ, EV_DISABLE, 0, 0, request);
  kevent(request->server->loop, ev_set, 2, NULL, 0, NULL);
#else
  struct epoll_event ev;
  ev.events = EPOLLOUT | EPOLLET;
  ev.data.ptr = request;
  epoll_ctl(request->server->loop, EPOLL_CTL_MOD, request->socket, &ev);
#endif
}

void hs_request_begin_write(http_request_t *request) {
  request->state = HTTP_SESSION_WRITE;
  _hs_add_write_event(request);
  _hs_write_socket_and_handle_return_code(request);
}

void _hs_add_read_event(http_request_t *request) {
#ifdef KQUEUE
  // No action needed for kqueue since it's read event stays active. Should
  // it be disabled during write?
  struct kevent ev_set;
  EV_SET(&ev_set, request->socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0,
         request);
  kevent(request->server->loop, &ev_set, 1, NULL, 0, NULL);
#else
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = request;
  epoll_ctl(request->server->loop, EPOLL_CTL_MOD, request->socket, &ev);
#endif
}

void hs_request_begin_read(http_request_t *request) {
  request->state = HTTP_SESSION_READ;
  _hs_add_read_event(request);
  _hs_read_socket_and_handle_return_code(request);
}

#endif
#endif
#endif
