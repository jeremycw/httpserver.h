cmake_minimum_required(VERSION 3.13)

# set the project name
project(httpserver)

add_subdirectory(src)

add_subdirectory(test/unit)
add_subdirectory(test/integration)
