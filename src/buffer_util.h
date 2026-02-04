#ifndef HS_BUFFER_UTIL_H
#define HS_BUFFER_UTIL_H

#include <stdlib.h>

#ifndef HTTPSERVER_IMPL
#include "common.h"
#endif

static inline void _hs_buffer_free(struct hsh_buffer_s *buffer,
                                   int64_t *memused) {
  if (buffer->buf) {
    free(buffer->buf);
    *memused -= buffer->capacity;
    buffer->buf = NULL;
  }
}

#endif
