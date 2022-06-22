add_executable(httpsrv-int main.c)

include(CheckFunctionExists)
check_function_exists(epoll_wait EPOLL)
check_function_exists(kqueue KQUEUE)

if(KQUEUE)
  target_compile_definitions(httpsrv-int PRIVATE "KQUEUE")
endif()

if(EPOLL)
  target_compile_definitions(httpsrv-int PRIVATE "EPOLL")
endif()

target_link_libraries(httpsrv-int PUBLIC httpsrv)
