// httpserver.h has been automatically generated from httpserver.m4 and the
// source files under /src
#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#line 1 "api.h"
include(`api.h')
#line 1 "common.h"
include(`common.h')
#line 1 "buffer_util.h"
include(`buffer_util.h')
#line 1 "request_util.h"
include(`request_util.h')
#line 1 "parser.h"
include(`parser.h')
#line 1 "read_socket.h"
include(`read_socket.h')
#line 1 "respond.h"
include(`respond.h')
#line 1 "server.h"
include(`server.h')
#line 1 "write_socket.h"
include(`write_socket.h')
#line 1 "connection.h"
include(`connection.h')
#line 1 "io_events.h"
include(`io_events.h')
#ifdef HTTPSERVER_IMPL
#ifndef HTTPSERVER_IMPL_ONCE
#define HTTPSERVER_IMPL_ONCE
#line 1 "api.c"
include(`api.c')
#line 1 "request_util.c"
include(`request_util.c')
#line 1 "parser.c"
include(`parser.c')
#line 1 "read_socket.c"
include(`read_socket.c')
#line 1 "respond.c"
include(`respond.c')
#line 1 "server.c"
include(`server.c')
#line 1 "write_socket.c"
include(`write_socket.c')
#line 1 "connection.c"
include(`connection.c')
#line 1 "io_events.c"
include(`io_events.c')
#endif
#endif
#endif
