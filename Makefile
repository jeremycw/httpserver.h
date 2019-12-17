.PHONY: test clean valgrind

all: http-server

test: test-results.txt
	 diff test-results.txt test/results.txt

valgrind: valgrind-results.txt
	diff valgrind-results.txt test/valgrind.txt

test-results.txt: http-server
	./http-server & test/run > test-results.txt; killall http-server;

valgrind-results.txt: http-server
	test/valgrind

http-server: test/main.c httpserver.h
	$(CC) -O3 -Wall -Wextra -Werror test/main.c -o http-server

clean:
	@rm http-server *.txt
