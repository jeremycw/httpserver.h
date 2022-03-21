.PHONY: test clean valgrind test-unit test-integration-cpp test-integration

CFLAGS :=-O3 -std=c99
CXXFLAGS :=-O3 -std=c++98
DEBUG_FLAGS :=-g -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all

all: http-server

test: test/integration/run-tests test/integration/results.txt test-unit test-integration test-integration-cpp

test-unit: hs-unit
	test/run-tests unit

test-integration-cpp: hs-integration-cpp
	test/run-tests integration-cpp

test-integration: hs-integration
	test/run-tests integration-c

test-integration-valgrind: hs-integration valgrind-results.txt
	test/run-tests valgrind

hs-integration: test/main.c httpserver.h
	$(CC) $(CFLAGS) -Wall -Wextra -Werror test/main.c -o hs-integration

hs-integration-cpp: test/main.cpp httpserver.h
	$(CXX) $(CXXFLAGS) -Wall -Wextra -Werror test/main.cpp -o hs-integration-cpp

hs-unit: test/unit/main.c test/unit/test_parser.c test/unit/test_server.c src/http_parser.c src/server.c src/lib.c
	$(CC) $(DEBUG_FLAGS) -Wall -Wextra test/unit/munit.c $^ -o hs-unit

httpserver.h: src/httpserver.m4 src/http_parser.c src/http_parser.h
	m4 httpserver.m4 > httpserver.h

src/http_parser.c: src/http_parser.rl
	ragel $<

test/integration/main.cpp: test/integration/main.c
	cp test/integration/main.c test/integration/main.cpp

clean:
	@rm hs-* http-server* *.txt
