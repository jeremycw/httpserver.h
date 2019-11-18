.PHONY: test

all: httpserver.h

http-server: test/main.c httpserver.h
	$(CC) -O3 -lev test/main.c -o http-server

httpserver.h: src/http_server.c $(wildcard: src/*.c) $(wildcard: src/*.h)
	./concat.sh

src/http_server.c: src/http_server.co corc/src/corc
	corc/src/corc < src/http_server.co > src/http_server.c

corc/src/corc: corc
	cd corc/src; make

corc:
	git clone https://github.com/jeremycw/corc.git

