.PHONY: test clean valgrind test-unit test-functional-cpp test-functional

CFLAGS :=-O3 -std=c99
CXXFLAGS :=-O3 -std=c++98

all: http-server

test: test/run-tests test/results.txt test-unit test-functional test-functional-cpp

test-unit: http-server-unit
	test/run-tests unit

test-functional-cpp: http-server-cpp
	test/run-tests functional-cpp

test-functional: http-server
	test/run-tests functional-c

valgrind: http-server valgrind-results.txt
	test/run-tests valgrind

http-server: test/main.c httpserver.h http_parser.c
	$(CC) $(CFLAGS) -Wall -Wextra -Werror test/main.c -o http-server

http-server-unit: test/test.c httpserver.h http_parser.c
	$(CC) -g -Wall -Wextra -Werror test/munit.c test/test.c -o http-server-unit

http-server-cpp: test/main.cpp httpserver.h http_parser.c
	$(CXX) $(CXXFLAGS) -Wall -Wextra -Werror test/main.cpp -o http-server-cpp

http_parser.c: http_parser.rl
	ragel $<

test/main.cpp: test/main.c
	cp test/main.c test/main.cpp

clean:
	@rm http-server* *.txt
