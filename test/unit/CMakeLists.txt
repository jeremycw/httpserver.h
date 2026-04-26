add_executable(
  unit-test-runner
  munit.c test_parser.c test_read_socket.c test_write_socket.c test_respond.c test_request_util.c main.c
)

include(CheckFunctionExists)
check_function_exists(epoll_wait EPOLL)
check_function_exists(kqueue KQUEUE)

if(KQUEUE)
  target_compile_definitions(unit-test-runner PRIVATE "KQUEUE")
endif()

if(EPOLL)
  target_compile_definitions(unit-test-runner PRIVATE "EPOLL")
endif()

target_compile_definitions(unit-test-runner PRIVATE "DEBUG")

target_link_libraries(unit-test-runner PUBLIC httpsrv)
