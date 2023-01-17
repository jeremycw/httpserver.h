#ifndef HS_CONNECTION_H
#define HS_CONNECTION_H

#ifdef KQUEUE
typedef void (*hs_io_cb_t)(struct kevent *ev);
#else
typedef void (*hs_io_cb_t)(struct epoll_event *ev);
#endif

// Forward declarations
struct http_request_s;
struct http_server_s;

/* Closes the requests socket and frees its resources.
 *
 * Removes all event watchers from the request socket and frees any allocated
 * buffers associated with the request struct.
 *
 * @param request The request to close
 */
void hs_terminate_connection(struct http_request_s *request);

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
void hs_accept_connections(struct http_server_s *server, hs_io_cb_t io_cb,
                           hs_io_cb_t epoll_timer_cb,
                           void (*err_responder)(struct http_request_s *),
                           int64_t max_mem_usage);

#endif
