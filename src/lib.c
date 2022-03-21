#include <stdlib.h>
#include <assert.h>
#include "lib.h"

void hsh_buffer_init(struct hsh_buffer_s* buffer, int initial_capacity) {
  *buffer = (struct hsh_buffer_s){ 0 };
  buffer->buf = malloc(initial_capacity);
  buffer->capacity = initial_capacity;
}

void hsh_buffer_free(struct hsh_buffer_s* buffer, int64_t* memused) {
  if (buffer->buf) {
    free(buffer->buf);
    *memused -= buffer->capacity;
    buffer->buf = NULL;
  }
}
void hs_token_array_push(struct hs_token_array_s* array, struct hsh_token_s a) {
  if (array->size == array->capacity) {
    array->capacity *= 2;
    array->buf = (struct hsh_token_s*)realloc(array->buf, array->capacity * sizeof(struct hsh_token_s));
    assert(array->buf != NULL);
  }
  array->buf[array->size] = a;
  array->size++;
}

void hs_token_array_init(struct hs_token_array_s* array, int capacity) {
  array->buf = (struct hsh_token_s*)malloc(sizeof(struct hsh_token_s) * capacity);
  assert(array->buf != NULL);
  array->size = 0;
  array->capacity = capacity;
}
