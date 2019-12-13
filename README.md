[![Build Status](https://travis-ci.com/jeremycw/httpserver.h.svg?branch=master)](https://travis-ci.com/jeremycw/httpserver.h)

See `httpserver.h` for API documentation

# Description

httpserver.h is a single header C library for building event driven non-blocking HTTP servers

Supports Linux with epoll and BSD/Mac with kqueue.

# Example

```c
#define HTTPSERVER_IMPL
#include "httpserver.h"

#define RESPONSE "Hello, World!"

void handle_request(struct http_request_s* request) {
  struct http_response_s* response = http_response_init();
  http_response_status(response, 200);
  http_response_header(response, "Content-Type", "text/plain");
  http_response_body(response, RESPONSE, sizeof(RESPONSE) - 1);
  http_respond(request, response);
}

int main() {
  struct http_server_s* server = http_server_init(8080, handle_request);
  http_server_listen(server);
}

```

See full documentation in `httpserver.h`

# Benchmark

I ran some micro-benchmarks with httpserver.h and NGINX serving a simple Hello, World! 
The purpose here was just to get a performance reference point, not to try and make
any statement of superiority since httpserver.h is not a competitor or replacement for
NGINX.

## With `keep-alive`
`ab -k -c 200 -n 100000 http://localhost:8080/`

### NGINX 74441.26 requests/sec
```
Server Software:        nginx/1.14.0
Server Hostname:        localhost
Server Port:            8000

Document Path:          /
Document Length:        13 bytes

Concurrency Level:      200
Time taken for tests:   1.343 seconds
Complete requests:      100000
Failed requests:        0
Keep-Alive requests:    99052
Total transferred:      20995260 bytes
HTML transferred:       1300000 bytes
Requests per second:    74441.26 [#/sec] (mean)
Time per request:       2.687 [ms] (mean)
Time per request:       0.013 [ms] (mean, across all concurrent requests)
Transfer rate:          15262.83 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0  17.8      0    1029
Processing:     0    2   8.8      1     148
Waiting:        0    2   8.8      1     148
Total:          0    3  21.4      1    1150

Percentage of the requests served within a certain time (ms)
  50%      1
  66%      2
  75%      2
  80%      2
  90%      2
  95%      2
  98%      3
  99%     35
 100%   1150 (longest request)
```

### httpserver.h 123907.91 requests/sec
```
Server Software:        
Server Hostname:        localhost
Server Port:            8080

Document Path:          /
Document Length:        13 bytes

Concurrency Level:      200
Time taken for tests:   0.807 seconds
Complete requests:      100000
Failed requests:        0
Keep-Alive requests:    100000
Total transferred:      13400000 bytes
HTML transferred:       1300000 bytes
Requests per second:    123907.91 [#/sec] (mean)
Time per request:       1.614 [ms] (mean)
Time per request:       0.008 [ms] (mean, across all concurrent requests)
Transfer rate:          16214.51 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       4
Processing:     1    2   0.3      2       6
Waiting:        1    2   0.3      2       6
Total:          1    2   0.4      2       6

Percentage of the requests served within a certain time (ms)
  50%      2
  66%      2
  75%      2
  80%      2
  90%      2
  95%      2
  98%      3
  99%      3
 100%      6 (longest request)
```

## With `Connection: close`
`ab -c 200 -n 100000 http://localhost:8080/`

### NGINX 15773.47 requests/sec
```
Server Software:        nginx/1.14.0
Server Hostname:        localhost
Server Port:            8000

Document Path:          /
Document Length:        13 bytes

Concurrency Level:      200
Time taken for tests:   6.340 seconds
Complete requests:      100000
Failed requests:        0
Total transferred:      20500000 bytes
HTML transferred:       1300000 bytes
Requests per second:    15773.47 [#/sec] (mean)
Time per request:       12.680 [ms] (mean)
Time per request:       0.063 [ms] (mean, across all concurrent requests)
Transfer rate:          3157.77 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    4  40.6      3    1123
Processing:     1    8  46.7      4     508
Waiting:        1    7  46.7      3     508
Total:          3   12  64.5      6    1460

Percentage of the requests served within a certain time (ms)
  50%      6
  66%      7
  75%      7
  80%      8
  90%      8
  95%      9
  98%     10
  99%    503
 100%   1460 (longest request)
```

### httpserver.h 27605.45 requests/sec
```
Server Software:        
Server Hostname:        localhost
Server Port:            8080

Document Path:          /
Document Length:        13 bytes

Concurrency Level:      200
Time taken for tests:   3.622 seconds
Complete requests:      100000
Failed requests:        0
Total transferred:      12900000 bytes
HTML transferred:       1300000 bytes
Requests per second:    27605.45 [#/sec] (mean)
Time per request:       7.245 [ms] (mean)
Time per request:       0.036 [ms] (mean, across all concurrent requests)
Transfer rate:          3477.64 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        2    3   0.5      3       6
Processing:     1    4   0.7      4       8
Waiting:        1    3   0.8      3       7
Total:          4    7   0.6      7      11

Percentage of the requests served within a certain time (ms)
  50%      7
  66%      7
  75%      8
  80%      8
  90%      8
  95%      8
  98%      9
  99%      9
 100%     11 (longest request)
```

### NGINX conf

```
worker_processes 1;

http {
    server {
        listen 8000;
        location / {
             add_header Content-Type text/plain;
             return 200 'Hello, World!';
        }
    }
}
```

