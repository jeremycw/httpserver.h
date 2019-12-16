.PHONY: test clean valgrind

all: http-server

test: http-server
	./http-server & test/run > results.txt; killall http-server; diff results.txt test/results.txt

valgrind: valgrind-results.txt
	diff valgrind-results.txt test/valgrind.txt

valgrind-results.txt: http-server
	test/valgrind

http-server: test/main.c httpserver.h
	$(CC) -O3 test/main.c -o http-server

clean:
	@rm http-server
