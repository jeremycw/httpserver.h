add_executable(integration-test-server main.c)

include(CheckFunctionExists)
check_function_exists(epoll_wait EPOLL)
check_function_exists(kqueue KQUEUE)

if(KQUEUE)
  target_compile_definitions(integration-test-server PRIVATE "KQUEUE")
endif()

if(EPOLL)
  target_compile_definitions(integration-test-server PRIVATE "EPOLL")
endif()

target_link_libraries(integration-test-server PUBLIC httpsrv)
