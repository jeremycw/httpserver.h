enable_testing()

cmake_minimum_required(VERSION 3.13)

# set the project name
project(httpserver.h)

add_subdirectory(src)

add_subdirectory(test/unit)
add_subdirectory(test/functional)

add_test(NAME Unit COMMAND httpsrv-unit)

