#define HTTPSERVER_IMPL
#include "../httpserver.h"
#define HTTPSERVER_PG_IMPL
#include "../httpserver-pg.h"
#include <signal.h>

struct hs_db_s* db;
struct http_server_s* server;

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

void handle_sigterm(int signum) {
  (void)signum;
  free(server);
  exit(0);
}

int main() {
  signal(SIGTERM, handle_sigterm);
  struct http_server_s* server = http_server_init(8082, handle_request);
  db = hs_db_init("postgres://localhost:5432/jeremy", http_server_loop(server));
  http_server_listen(server);
}
