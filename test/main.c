#define HTTPSERVER_IMPL
#include "../httpserver.h"

#define RESPONSE "Hello, World!"

void handle_request(struct http_request_s* request) {
  struct http_response_s* response = http_response_init();
  http_response_status(response, 200);
  http_response_header(response, "Content-Type", "text/plain");
  http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  http_respond(request, response);
}

int main() {
  printf("request: %lu\n", sizeof(http_request_t));
  struct http_server_s* server = http_server_init(8080, handle_request);
  http_server_listen(server);
}
