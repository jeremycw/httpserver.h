#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#line 1 "src/api.h"
include(`src/api.h')
#line 1 "src/common.h"
include(`src/common.h')
#line 1 "src/buffer_util.h"
include(`src/buffer_util.h')
#line 1 "src/request_util.h"
include(`src/request_util.h')
#line 1 "src/parser.h"
include(`src/parser.h')
#line 1 "src/read_socket.h"
include(`src/read_socket.h')
#line 1 "src/respond.h"
include(`src/respond.h')
#line 1 "src/server.h"
include(`src/server.h')
#line 1 "src/write_socket.h"
include(`src/write_socket.h')
#line 1 "src/connection.h"
include(`src/connection.h')
#line 1 "src/io_events.h"
include(`src/io_events.h')
#ifdef HTTPSERVER_IMPL
#ifndef HTTPSERVER_IMPL_ONCE
#define HTTPSERVER_IMPL_ONCE
#line 1 "src/api.c"
include(`src/api.c')
#line 1 "src/request_util.c"
include(`src/request_util.c')
#line 1 "src/parser.c"
include(`src/parser.c')
#line 1 "src/read_socket.c"
include(`src/read_socket.c')
#line 1 "src/respond.c"
include(`src/respond.c')
#line 1 "src/server.c"
include(`src/server.c')
#line 1 "src/write_socket.c"
include(`src/write_socket.c')
#line 1 "src/connection.c"
include(`src/connection.c')
#line 1 "src/io_events.c"
include(`src/io_events.c')
#endif
#endif
#endif
