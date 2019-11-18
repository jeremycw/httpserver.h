#!/usr/bin/env sh

echo "#ifndef HTTPSERVER_H" > httpserver.h
echo "#define HTTPSERVER_H" >> httpserver.h
echo "#define FIBER_IMPL" >> httpserver.h
cat src/fiber.h >> httpserver.h
cat src/varray.h >> httpserver.h
cat src/http_parser.h >> httpserver.h
cat src/http_server.h >> httpserver.h
cat src/http_request.h >> httpserver.h
cat src/http_response.h >> httpserver.h
echo "#ifdef HTTPSERVER_IMPL" >> httpserver.h
cat src/http_server.c >> httpserver.h
cat src/http_request.c >> httpserver.h
cat src/http_response.c >> httpserver.h
cat src/http_parser.c >> httpserver.h
echo "#endif" >> httpserver.h
echo "#endif" >> httpserver.h
