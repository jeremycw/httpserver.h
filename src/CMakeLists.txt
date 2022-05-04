add_library(
  evhttpserv
  api.c connection.c io_events.c ${CMAKE_CURRENT_BINARY_DIR}/parser.c
  read_socket.c request_util.c respond.c server.c write_socket.c
)

target_compile_options(
  evhttpserv
  PRIVATE -Wall -Wextra -Werror
  $<$<CONFIG:DEBUG>:-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all>
)

target_link_options(
  evhttpserv
  PUBLIC $<$<CONFIG:DEBUG>:-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all>
)

set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/parser.c
  PROPERTIES COMPILE_FLAGS -Wno-unused-variable
)

add_custom_target(
  httpserver.h
  ALL
  COMMAND m4 -I${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/httpserver.m4 > ${CMAKE_CURRENT_BINARY_DIR}/httpserver.h
  DEPENDS api.h api.c buffer_util.h common.h connection.h connection.c
    io_events.h io_events.c parser.h ${CMAKE_CURRENT_BINARY_DIR}/parser.c
    read_socket.h read_socket.c request_util.h request_util.c respond.h respond.c
    server.h server.c write_socket.h write_socket.c httpserver.m4
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/parser.c
  COMMAND ragel ${CMAKE_CURRENT_SOURCE_DIR}/parser.rl -o ${CMAKE_CURRENT_BINARY_DIR}/parser.c
  DEPENDS parser.rl
)

target_include_directories(
  evhttpserv
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

include(CheckFunctionExists)
check_function_exists(epoll_wait EPOLL)
check_function_exists(kqueue KQUEUE)

if(KQUEUE)
  target_compile_definitions(evhttpserv PRIVATE "KQUEUE")
endif()

if(EPOLL)
  target_compile_definitions(evhttpserv PRIVATE "EPOLL")
endif()

