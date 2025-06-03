#include "types.h"
#include <stdbool.h>
#include <stdlib.h>

static bool grow_exact(void **ptr, size_t size, size_t *cap, size_t new_cap) {
  if (*cap >= new_cap)
    return true;
  void *new_ptr = realloc(*ptr, new_cap * size);
  if (!new_ptr)
    return false;
  *ptr = new_ptr;
  *cap = new_cap;
  return true;
}

// returns true if there is enough capacity
static bool grow(void **ptr, size_t size, size_t *cap, size_t required_cap) {
  if (*cap >= required_cap)
    return true;
  size_t new_cap = max(16, *cap);
  while (new_cap < required_cap)
    new_cap *= 2;
  return grow_exact(ptr, size, cap, new_cap);
}
