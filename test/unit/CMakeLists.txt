add_executable(
  httpsrv-unit
  munit.c test_parser.c test_read_socket.c test_write_socket.c main.c
)

include(CheckFunctionExists)
check_function_exists(epoll_wait EPOLL)
check_function_exists(kqueue KQUEUE)

if(KQUEUE)
  target_compile_definitions(httpsrv-unit PRIVATE "KQUEUE")
endif()

if(EPOLL)
  target_compile_definitions(httpsrv-unit PRIVATE "EPOLL")
endif()

target_link_libraries(httpsrv-unit PUBLIC httpsrv)
