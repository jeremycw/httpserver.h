#define HTTPSERVER_IMPL
#include "../httpserver.h"

void handle_request(http_request_t* request) {
  http_response_t response = {};
  http_response_status(&response, 200);
  http_response_header(&response, "Content-Type", "text/plain");
  http_response_body(&response, "Hello World!", 12);
  http_respond(request, &response);
}

int main() {
  http_server_t server;
  http_server_init(&server, 8080, handle_request);
  http_server_listen(&server);
}
