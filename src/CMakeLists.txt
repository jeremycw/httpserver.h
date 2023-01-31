add_library(
  httpsrv
  api.c connection.c io_events.c ${CMAKE_CURRENT_BINARY_DIR}/parser.c
  read_socket.c request_util.c respond.c server.c write_socket.c
)

set_property(TARGET httpsrv PROPERTY C_STANDARD 99)

target_compile_options(
  httpsrv
  PRIVATE -Wall -Wextra -Werror
  $<$<CONFIG:DEBUG>:-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all>
)

target_link_options(
  httpsrv
  PUBLIC $<$<CONFIG:DEBUG>:-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all>
)

set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/parser.c
  PROPERTIES COMPILE_FLAGS -Wno-unused-variable
)

find_program(M4 m4)
if(NOT M4)
  message(FATAL_ERROR "m4 not found. Please install before continuing.")
endif()

add_custom_target(
  httpserver.h
  ALL
  COMMAND ${M4} -I${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/httpserver.m4 > ${CMAKE_CURRENT_BINARY_DIR}/httpserver.h
  DEPENDS api.h api.c buffer_util.h common.h connection.h connection.c
    io_events.h io_events.c parser.h ${CMAKE_CURRENT_BINARY_DIR}/parser.c
    read_socket.h read_socket.c request_util.h request_util.c respond.h respond.c
    server.h server.c write_socket.h write_socket.c httpserver.m4
)

find_program(RAGEL ragel)
if(NOT RAGEL)
  message(FATAL_ERROR "ragel not found. Please install before continuing.")
endif()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/parser.c
  COMMAND ${RAGEL} ${CMAKE_CURRENT_SOURCE_DIR}/parser.rl -o ${CMAKE_CURRENT_BINARY_DIR}/parser.c
  DEPENDS parser.rl
)

target_include_directories(
  httpsrv
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

include(CheckFunctionExists)
check_function_exists(epoll_wait EPOLL)
check_function_exists(kqueue KQUEUE)

target_compile_definitions(httpsrv PRIVATE $<$<CONFIG:DEBUG>:DEBUG>)

if(KQUEUE)
  target_compile_definitions(httpsrv PRIVATE KQUEUE)
endif()

if(EPOLL)
  target_compile_definitions(httpsrv PRIVATE EPOLL)
endif()

