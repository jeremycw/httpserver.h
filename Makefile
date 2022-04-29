.PHONY: test clean valgrind test-unit test-integration-cpp test-integration

CFLAGS +=-std=c99 -Wall -Wextra
CXXFLAGS +=-std=c++98
RELEASE_FLAGS :=-O3
DEBUG_FLAGS :=-g -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all

test: test/integration/run-tests test/integration/results.txt test-unit test-integration test-integration-cpp

test-unit: hs-unit
	test/run-tests unit
 
test-integration-cpp: hs-integration-cpp
	test/run-tests integration-cpp
 
test-integration: hs-integration
	test/run-tests integration-c
 
test-integration-valgrind: hs-integration valgrind-results.txt
	test/run-tests valgrind

hs-integration: test/integration/main.c httpserver.h
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) test/integration/main.c -o hs-integration

hs-integration-cpp: test/integration/main.cpp httpserver.h
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) test/main.cpp -o hs-integration-cpp

hs-unit: test/unit/main.c test/unit/test_parser.c src/parser.c src/read_socket.c src/write_socket.c test/unit/test_read_socket.c test/unit/test_write_socket.c
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -DKQUEUE -DHS_UNIT_TEST test/unit/munit.c $^ -o hs-unit

httpserver.h: httpserver.m4 src/*.c src/*.h src/parser.c
	m4 httpserver.m4 > httpserver.h

src/parser.c: src/parser.rl
	ragel $<

test/integration/main.cpp: test/integration/main.c
	cp test/integration/main.c test/integration/main.cpp

check-format:
	clang-format --style=LLVM --dry-run -Werror src/*.c

clean:
	@rm -rf hs-* http-server* *.txt *.o
