// Experiment for now, run with:
// gcc src/find_frames.c -fsanitize=address -g
// ./a.out dep/ivm_testdata/reel/png

#include "dir.c"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define countof(x) (sizeof(x) / sizeof *(x))

#include <stdbool.h>
#define max(x, y) ((x) < (y) ? (y) : (x))

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

typedef struct {
  char *string_pool;
  size_t string_pool_count;
  size_t string_pool_capacity;
  uint16_t count;
  uint32_t frames[65529];
} Reel;

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    Reel *reel = malloc(sizeof(Reel));
    memset(reel, 0, sizeof *reel);
    DirIterator it;
    if (dir_start(argv[i], &it)) {
      DirEntry ent;
      while (dir_next(&it, &ent)) {
        // printf("entry: %s\n", ent.name);
        size_t name_len = strlen(ent.name);
        char *name_end = ent.name + strlen(ent.name);
        long id = strtol(ent.name, &name_end, 10);
        if (id >= 0 && id < countof(reel->frames)) {
          grow((void **)&reel->string_pool, 1, &reel->string_pool_capacity,
               reel->string_pool_count + name_len + 1);
          uint32_t str_start = reel->string_pool_count + 1;
          memcpy(reel->string_pool + reel->string_pool_count, ent.name,
                 name_len + 1);
          reel->string_pool_count += name_len + 1;
          reel->frames[id] = str_start;
          reel->count++;
        }
      }
      dir_end(&it);
    }
    uint16_t seen = 0;
    for (uint16_t i = 0; seen < reel->count && i < countof(reel->frames); i++) {
      uint32_t str = reel->frames[i];
      if (!str)
        continue;
      printf("%u: %s\n", i, reel->string_pool + str - 1);
      seen++;
    }
    free(reel->string_pool);
    free(reel);
  }
}
