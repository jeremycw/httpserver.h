#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef HTTPSERVER_H
#include "http_server.h"
#include "fiber.h"
#include "http_request.h"
#include "http_response.h"
#endif

#define RESPONSE_BUF_SIZE 512

char const * status_text[] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  
  //100s
  "Continue", "Switching Protocols", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //200s
  "OK", "Created", "Accepted", "Non-Authoratative Information", "No Content",
  "Reset Content", "Partial Content", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //300s
  "Multiple Choices", "Moved Permanently", "Found", "See Other", "Not Modified",
  "Use Proxy", "", "Temporary Redirect", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //400s
  "Bad Request", "Unauthorized", "Payment Required", "Forbidden", "Not Found",
  "Method Not Allowed", "Not Acceptable", "Proxy Authentication Required",
  "Request Timeout", "Conflict",
  "Gone", "Length Required", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",

  //500s
  "Internal Server Error", "Not Implemented", "Bad Gateway", "Service Unavailable",
  "Gateway Timeout", "", "", "", "", ""
};

void http_response_header(http_response_t* response, char const * key, char const * value) {
  http_header_t* header = malloc(sizeof(http_header_t));
  header->key = key;
  header->value = value;
  http_header_t* prev = response->headers;
  header->next = prev;
  response->headers = header;
}

void http_response_status(http_response_t* response, int status) {
  response->status = status;
}

void http_response_body(http_response_t* response, char const * body, int length) {
  response->body = body;
  response->content_length = length;
}

#define buffer_bookkeeping(printf) \
  printf \
  if (bytes + size > capacity) { \
    while (bytes + size > capacity) capacity *= 2; \
    buf = realloc(buf, capacity); \
    printf \
    remaining = capacity - size; \
  } \
  size += bytes; \
  remaining -= bytes;

void http_respond(http_request_t* session, http_response_t* response) {
  char* buf = malloc(RESPONSE_BUF_SIZE);
  int capacity = RESPONSE_BUF_SIZE;
  int remaining = RESPONSE_BUF_SIZE;
  int size = 0;
  if (HTTP_FLAG_CHECK(session->flags, HTTP_KEEP_ALIVE)) {
    http_response_header(response, "Connection", "keep-alive");
  } else {
    HTTP_FLAG_CLEAR(session->flags, HTTP_ACTIVE);
    http_response_header(response, "Connection", "close");
  }

  int bytes = snprintf(
    buf, remaining, "HTTP/1.1 %d %s\r\nDate: %.24s\r\n",
    response->status, status_text[response->status], session->server->date
  );
  size += bytes;
  remaining -= bytes;
  http_header_t* header = response->headers;
  while (header) {
    buffer_bookkeeping(
      bytes = snprintf( 
        buf + size, remaining, "%s: %s\r\n", 
        header->key, header->value 
      );
    )
    header = header->next;
  }
  if (response->body) {
    buffer_bookkeeping(
      bytes = snprintf(
        buf + size, remaining, "Content-Length: %d\r\n",
        response->content_length
      );
    )
  }
  buffer_bookkeeping(bytes = snprintf(buf + size, remaining, "\r\n");)
  if (response->body) {
    buffer_bookkeeping(
      bytes = snprintf(
        buf + size, remaining, "%.*s",
        response->content_length, response->body
      );
    )
  }
  header = response->headers;
  while (header) {
    http_header_t* tmp = header;
    header = tmp->next;
    free(tmp);
  }
  free(session->buf);
  session->buf = buf;
  session->bytes = 0;
  session->capacity = size;
  HTTP_FLAG_SET(session->flags, HTTP_RESPONSE_READY);
  if (HTTP_FLAG_CHECK(session->flags, HTTP_RESPONSE_PAUSED)) {
    HTTP_FLAG_CLEAR(session->flags, HTTP_RESPONSE_PAUSED);
    fiber_resume(http_session, session->fiber);
  }
}

