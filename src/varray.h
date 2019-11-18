#ifndef VARRAY_H
#define VARRAY_H

#include <stdlib.h>

#define varray_decl(type) \
  typedef struct { \
    type* buf; \
    int capacity; \
    int size; \
  } varray_##type##_t; \
  void varray_##type##_push(varray_##type##_t* varray, type a); \
  void varray_##type##_init(varray_##type##_t* varray, int capacity);

#define varray_defn(type) \
  void varray_##type##_push(varray_##type##_t* varray, type a) { \
    if (varray->size == varray->capacity) { \
      varray->capacity *= 2; \
      varray->buf = realloc(varray->buf, varray->capacity * sizeof(type)); \
    } \
    varray->buf[varray->size] = a; \
    varray->size++; \
  } \
  void varray_##type##_init(varray_##type##_t* varray, int capacity) { \
    varray->buf = malloc(sizeof(type) * capacity); \
    varray->size = 0; \
    varray->capacity = capacity; \
  }

#define varray_t(type) \
  varray_##type##_t

#define varray_push(type, varray, a) \
  varray_##type##_push(varray, a); 

#define varray_init(type, varray, capacity) \
  varray_##type##_init(varray, capacity);

#endif
