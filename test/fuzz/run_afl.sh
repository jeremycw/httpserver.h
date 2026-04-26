#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
FUZZ_SRC="$PROJECT_DIR/test/fuzz/afl_parser.c"
FUZZ_BIN="$BUILD_DIR/afl_parser"
SEED_DIR="$PROJECT_DIR/test/fuzz/seeds"
OUTPUT_DIR="$PROJECT_DIR/fuzz_output"

echo "=== AFL Fuzzing Setup for httpserver.h ==="

if [ ! -f "$FUZZ_SRC" ]; then
  echo "Error: afl_parser.c not found"
  exit 1
fi

if ! command -v afl-clang &> /dev/null && ! command -v afl-gcc &> /dev/null; then
  echo "AFL not found. Installing..."
  if command -v brew &> /dev/null; then
    brew install afl++
  else
    echo "Please install AFL++ manually: https://github.com/AFLplusplus/AFLplusplus"
    exit 1
  fi
fi

echo "Building with AFL instrumentation..."
cd "$BUILD_DIR"

CC=${CC:-afl-clang}
CFLAGS="-g -O2 -DKQUEUE -I $PROJECT_DIR/build/src -I $PROJECT_DIR/src"

$CC $CFLAGS -o afl_parser "$FUZZ_SRC" \
  $PROJECT_DIR/src/api.c \
  $PROJECT_DIR/src/connection.c \
  $PROJECT_DIR/src/io_events.c \
  $PROJECT_DIR/src/read_socket.c \
  $PROJECT_DIR/src/request_util.c \
  $PROJECT_DIR/src/respond.c \
  $PROJECT_DIR/src/server.c \
  $PROJECT_DIR/src/write_socket.c \
  parser.c \
  -lpthread

echo " AFL binary built: $FUZZ_BIN"

if [ ! -d "$SEED_DIR" ]; then
  echo "Creating seed directory..."
  mkdir -p "$SEED_DIR"

  cat > "$SEED_DIR/get_simple.txt" << EOF
GET / HTTP/1.1
Host: localhost

EOF

  cat > "$SEED_DIR/post_simple.txt" << EOF
POST /api/data HTTP/1.1
Host: localhost
Content-Type: application/json
Content-Length: 13

{"key": "value"}
EOF

  cat > "$SEED_DIR/chunked.txt" << EOF
POST /upload HTTP/1.1
Host: localhost
Transfer-Encoding: chunked

5
hello
0

EOF

  cat > "$SEED_DIR/malformed.txt" << EOF
GET / HTTP/1.1
Host: 

EOF

  echo "Created seed inputs in $SEED_DIR"
fi

mkdir -p "$OUTPUT_DIR"

echo ""
echo "=== Starting AFL fuzzing ==="
echo "Output directory: $OUTPUT_DIR"
echo "Seeds: $SEED_DIR"
echo ""
echo "To run fuzzing:"
echo "  afl-fuzz -i $SEED_DIR -o $OUTPUT_DIR -- $FUZZ_BIN @@"
echo ""
echo "Or with parallel fuzzing on multiple cores:"
echo "  afl-fuzz -i $SEED_DIR -o $OUTPUT_DIR -M main -- $FUZZ_BIN @@"
echo "  afl-fuzz -i $SEED_DIR -o $OUTPUT_DIR -S secondary -- $FUZZ_BIN @@"
echo ""

read -p "Start fuzzing now? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
  afl-fuzz -i "$SEED_DIR" -o "$OUTPUT_DIR -- $FUZZ_BIN @@
fi