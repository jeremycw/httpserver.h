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
