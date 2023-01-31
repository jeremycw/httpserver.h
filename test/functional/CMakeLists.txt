add_executable(functional-test-server main.c)

include(CheckFunctionExists)
check_function_exists(epoll_wait EPOLL)
check_function_exists(kqueue KQUEUE)

if(KQUEUE)
  target_compile_definitions(functional-test-server PRIVATE "KQUEUE")
endif()

if(EPOLL)
  target_compile_definitions(functional-test-server PRIVATE "EPOLL")
endif()

target_link_libraries(functional-test-server PUBLIC httpsrv)
