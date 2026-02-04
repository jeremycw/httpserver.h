add_executable(functional-test-server main.c)
add_executable(functional-test-server-cpp main.cpp)

include(CheckFunctionExists)
check_function_exists(epoll_wait EPOLL)
check_function_exists(kqueue KQUEUE)

if(KQUEUE)
  target_compile_definitions(functional-test-server PRIVATE "KQUEUE")
  target_compile_definitions(functional-test-server-cpp PRIVATE "KQUEUE")
endif()

if(EPOLL)
  target_compile_definitions(functional-test-server PRIVATE "EPOLL")
  target_compile_definitions(functional-test-server-cpp PRIVATE "EPOLL")
endif()
