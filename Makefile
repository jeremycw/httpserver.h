.PHONY: test clean

all: http-server

test: http-server
	./http-server & test/run > results.txt; killall http-server; diff results.txt test/results.txt

http-server: test/main.c httpserver.h
	$(CC) -O3 test/main.c -o http-server

clean:
	@rm http-server
