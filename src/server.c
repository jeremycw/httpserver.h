#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

#include "http_parser.h"
#include "data.h"
#include "lib.h"
#include "server.h"

#include "../test/debugbreak.h"

// This is the heart of the request logic. This is the state machine that
// controls what happens when an IO event is received.
void hs_server_process_read_ready(struct hs_server_process_read_ready_s* params) {
  params->request_state = HTTP_SESSION_READ;
  params->request_timeout = HTTP_REQUEST_TIMEOUT;

  struct hsh_buffer_s* buffer = params->buffer;

  // Check if there is already content in the buffer.
  if (buffer->index < buffer->length) goto skip_socket_read;

  int bytes;
  do {
    bytes = read(
      params->request_socket,
      buffer->buf + buffer->length,
      buffer->capacity - buffer->length
    );
    if (bytes > 0) buffer->length += bytes;

    if (
      buffer->length == buffer->capacity &&
      buffer->capacity != params->max_request_buf_capacity
    ) {
      params->server_memused -= buffer->capacity;
      buffer->capacity *= 2;
      if (buffer->capacity > params->max_request_buf_capacity) {
        buffer->capacity = params->max_request_buf_capacity;
      }
      params->server_memused += buffer->capacity;
      buffer->buf = (char*)realloc(buffer->buf, buffer->capacity);
      assert(buffer->buf != NULL);
    }
  } while (bytes > 0 && buffer->capacity < params->max_request_buf_capacity);

  buffer->sequence_id++;

  if (bytes == params->eof_rc) {
    params->after_event = HS_EVT_SOCKET_CLOSED;
    return;
  }

skip_socket_read:

  do {
    struct hsh_token_s token = hsh_parser_exec(params->parser, params->buffer, params->max_request_buf_capacity);
    switch (token.type) {
      case HSH_TOK_HEADERS_DONE:
        if (
          HSH_FLAG_CHECK(token.flags, HSH_TOK_FLAG_STREAMED_BODY)
          || HSH_FLAG_CHECK(token.flags, HSH_TOK_FLAG_NO_BODY)
        ) {
          params->after_event = HS_EVT_REQUEST_CALLBACK;
          return;
        }
        break;
      case HSH_TOK_BODY:
        if (HSH_FLAG_CHECK(token.flags, HSH_TOK_FLAG_SMALL_BODY)) {
          params->after_event = HS_EVT_REQUEST_CALLBACK;
        } else {
          params->after_event = HS_EVT_BODY_CALLBACK;
        }
        hs_token_array_push(params->tokens, token);
        return;
      case HSH_TOK_ERR:
        params->after_event = HS_EVT_PARSER_ERR;
        return;
      case HSH_TOK_NONE:
        return;
      default:
        hs_token_array_push(params->tokens, token);
        break;
    }
  } while (1);
}

// Writes response bytes out to the socket. Runs when we get a socket ready to
// write event or when writing to the socket for the first time.
void hs_server_process_write_ready(struct hs_server_process_write_ready_s* params) {
  int bytes = write(
    params->request_socket,
    params->buffer->buf + params->bytes_written,
    params->buffer->length - params->bytes_written
  );
  if (bytes > 0) params->bytes_written += bytes;

  if (errno == EPIPE) {
    params->after_event = HS_EVT_SOCKET_CLOSED;
    return;
  }

  if (params->bytes_written != params->buffer->length) {
    // All bytes of the body were not written and we need to wait until the
    // socket is writable again to complete the write

    // add write event listener
    {
#ifdef EPOLL
      // epoll
      struct epoll_event ev;
      ev.events = EPOLLOUT | EPOLLET;
      ev.data.ptr = params->request_ptr;
      epoll_ctl(params->event_loop, EPOLL_CTL_MOD, params->request_socket, &ev);
#else
      // kqueue
      struct kevent ev_set[2];
      EV_SET(&ev_set[0], params->request_socket, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, params->request_ptr);
      kevent(params->event_loop, ev_set, 2, NULL, 0, NULL);
#endif
    }

    params->request_state = HTTP_SESSION_WRITE;
    params->request_timeout = HTTP_REQUEST_TIMEOUT;
  } else if (HSH_FLAG_CHECK(params->request_flags, HTTP_CHUNKED_RESPONSE)) {
    // All bytes of the chunk were written and we need to get the next chunk
    // from the application.
    params->request_state = HTTP_SESSION_WRITE;
    params->request_timeout = HTTP_REQUEST_TIMEOUT;
    hsh_buffer_free(params->buffer, &params->server_memused);
    params->after_event = HS_EVT_BODY_CALLBACK;
  } else {
    if (HSH_FLAG_CHECK(params->request_flags, HTTP_KEEP_ALIVE)) {
      params->request_state = HTTP_SESSION_INIT;
      hsh_buffer_free(params->buffer, &params->server_memused);
      params->request_timeout = HTTP_REQUEST_TIMEOUT;
    } else {
      // XXX end session event
      params->after_event = HS_EVT_END_SESSION;
    }
  }
}

// void hs_handle_client_socket_event(http_request_t* request) {
//   enum hs_event_e event = HS_EVT_NONE;
// 
//   if (request->state == HTTP_SESSION_READ) {
//     hs_server_process_read_ready(params);
//     event = params->after_event;
//   } else if (request->state == HTTP_SESSION_WRITE) {
//     hs_server_process_write_ready(params);
//     event = params->after_event;
//   }
//   
//   while (event != HS_EVT_NONE) {
//     switch (event) {
//       case HS_EVT_REQUEST_CALLBACK:
//         // XXX gather input
//         hs_server_exec_request_callback(params);
//         // XXX store output
//         event = params->after_event;
//         break;
//       case HS_EVT_BODY_CALLBACK:
//         // XXX gather input
//         hs_server_exec_body_callback(params);
//         // XXX store output
//         event = params->after_event;
//         break;
//       case HS_EVT_SOCKET_CLOSED:
//         // XXX gather input
//         hs_server_handle_early_socket_close(params);
//         // XXX store output
//         event = params->after_event;
//         break;
//       case HS_EVT_PARSER_ERR:
//         // XXX gather input
//         hs_server_handle_parser_error(params);
//         // XXX store output
//         event = params->after_event;
//         break;
//     }
//   }
// }
