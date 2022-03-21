#ifndef READ_SOCKET_H
#define READ_SOCKET_H

#include "data.h"

struct hs_server_process_read_ready_s {
  int64_t server_memused; //inout
  struct hsh_buffer_s* buffer; //inout
  struct hsh_parser_s* parser; //inout
  struct hs_token_array_s* tokens; //inout

  int request_timeout; //out
  enum hs_event_e after_event; //out
  enum hs_session_state_e request_state; //out

  int64_t max_request_buf_capacity; //in
  int request_socket; //in
  int initial_request_buf_capacity; //in
  int eof_rc; //in
};

struct hs_server_process_write_ready_s {
  struct hsh_buffer_s* buffer; //inout
  int64_t bytes_written; //inout
  int64_t* server_memused; //inout

  void* request_ptr; //in
  int request_socket; //in
  int event_loop; //in
  uint8_t request_flags; //in


  enum hs_event_e after_event; //out
  int request_timeout; //out
  enum hs_session_state_e request_state; //out
};

void hs_server_process_read_ready(struct hs_server_process_read_ready_s* params);
void hs_server_process_write_ready(struct hs_server_process_write_ready_s* params);
// void hs_handle_client_socket_event(http_request_t* request);

#endif
