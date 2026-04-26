add_executable(fuzz_parser fuzz_parser.c)
set_target_properties(fuzz_parser PROPERTIES SKIP_INCLUDE_WHAT_YOU_USE ON)

if(APPLE)
  target_link_libraries(fuzz_parser PUBLIC "-fsanitize=fuzzer,address")
else()
  target_link_libraries(fuzz_parser PUBLIC "-fsanitize=fuzzer,address,undefined")
endif()

target_compile_definitions(fuzz_parser PRIVATE HTTPSERVER_IMPL)