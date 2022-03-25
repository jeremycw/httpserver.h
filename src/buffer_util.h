#ifndef HS_BUFFER_UTIL_H
#define HS_BUFFER_UTIL_H

#include <stdlib.h>

struct hsh_buffer_s;

static inline void _hs_buffer_free(struct hsh_buffer_s *buffer,
                                   int64_t *memused) {
  if (buffer->buf) {
    free(buffer->buf);
    *memused -= buffer->capacity;
    buffer->buf = NULL;
  }
}

#endif
