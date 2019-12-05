.PHONY: test clean

all: httpserver.h

test: http-server

http-server: test/main.c httpserver.h
	$(CC) -O3 -lev test/main.c -o http-server

clean:
	@rm http-server
