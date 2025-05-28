// Experiment for now, run with:
// gcc src/find_frames.c -fsanitize=address -g
// ./a.out dep/ivm_testdata/reel/png

#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define min(x, y) ((y) < (x) ? (y) : (x))
#define max(x, y) ((x) < (y) ? (y) : (x))
#define clamp(x, lo, hi) ((x) < (lo) ? (lo) : (hi) < (x) ? (hi) : (x))
#define countof(x) (sizeof(x) / sizeof *(x))

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

bool Reel_init(Reel *reel,
               const char *const
                   directory_path // path to directory containing scanned photos
) {
  DIR *d = opendir(directory_path);
  if (!d)
    return false;
  struct dirent *ent;
  while (ent = readdir(d)) {
    size_t name_len = strlen(ent->d_name);
    char *name_end = ent->d_name + strlen(ent->d_name);
    long id = strtol(ent->d_name, &name_end, 10);
    if (name_end == ent->d_name)
      continue;
    if (id >= 0 && id < countof(reel->frames)) {
      if (!grow((void **)&reel->string_pool, 1, &reel->string_pool_capacity,
                reel->string_pool_count + name_len + 1)) {
        closedir(d);
        return false;
      }
      uint32_t str_start = reel->string_pool_count + 1;
      memcpy(reel->string_pool + reel->string_pool_count, ent->d_name,
             name_len + 1);
      reel->string_pool_count += name_len + 1;
      reel->frames[id] = str_start;
      reel->count++;
    }
  }
  closedir(d);
  return true;
}

Reel *Reel_create(void) {
  Reel *reel = malloc(sizeof(Reel));
  if (!reel)
    return NULL;
  memset(reel, 0, sizeof *reel);
  return reel;
}

void Reel_destroy(Reel *reel) {
  free(reel->string_pool);
  free(reel);
}

void Reel_reset(Reel *reel) {
  reel->string_pool_count = 0;
  memset(&reel->frames, 0, sizeof reel->frames);
}

/*
int main(int argc, char *argv[]) {
  Reel *reel = Reel_create();
  if (!reel)
    return EXIT_FAILURE;
  for (int i = 1; i < argc; i++) {
    Reel_reset(reel);
    printf("arg %d: %s\n", i, argv[i]);
    if (Reel_init(reel, argv[i])) {
      uint16_t seen = 0;
      for (uint16_t i = 0; seen < reel->count && i < countof(reel->frames);
           i++) {
        uint32_t str = reel->frames[i];
        if (!str)
          continue;
        // printf("%u: %s\n", i, reel->string_pool + str - 1);
        seen++;
      }
    }
  }
  Reel_destroy(reel);
  return EXIT_SUCCESS;
}
*/
