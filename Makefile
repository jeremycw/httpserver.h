.PHONY: test clean valgrind

CFLAGS :=-O3 -std=c99
CXXFLAGS :=-O3 -std=c++98

all: http-server

test: http-server test/run-tests test-unit test/results.txt http-server-cpp
	test/run-tests test

valgrind: http-server valgrind-results.txt
	test/run-tests valgrind

http-server: test/main.c httpserver.h http_parser.c
	$(CC) $(CFLAGS) -Wall -Wextra -Werror test/main.c -o http-server

test-unit: test/test.c httpserver.h http_parser.c
	$(CC) $(CFLAGS) -Wall -Wextra -Werror test/munit.c test/test.c -o test-unit

http-server-cpp: test/main.cpp httpserver.h http_parser.c
	$(CXX) $(CXXFLAGS) -Wall -Wextra -Werror test/main.cpp -o http-server-cpp

http_parser.c: http_parser.rl
	ragel $<

test/main.cpp: test/main.c
	cp test/main.c test/main.cpp

clean:
	@rm http-server http-server-cpp *.txt test-unit
