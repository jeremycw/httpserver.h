.PHONY: test clean

all: httpserver.h

test: http-server

http-server: test/main.c httpserver.h
	$(CC) -O3 -lev test/main.c -o http-server

httpserver.h: httpserver.co
	corc/src/corc < httpserver.co > httpserver.h

corc/src/corc: corc
	cd corc/src; make

corc:
	git clone https://github.com/jeremycw/corc.git

clean:
	@rm httpserver.h http-server
