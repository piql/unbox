#include "map_file.c"
#include "unboxing_log.c"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void *memory = NULL;
static size_t used = 0;
#define MEMORY_SIZE (256ul * 1024ul * 1024ul)

static void image_reset(void) { used = 0; }

static void image_deinit(void) { free(memory); }

// Debug image memory allocations
#if 1
#define print_used()
#define image_debug_printf(fmt, ...)
#else
#define image_debug_printf(fmt, ...) printf(fmt, __VA_ARGS__)
#include <stdio.h>
static const char *full = "####################################################"
                          "##########################";
static const char *empty = "..................................................."
                           "...........................";
static void print_used(void) {
  putchar('[');
  unsigned frac = ((float)used / (float)MEMORY_SIZE) * (float)78;
  printf("%.*s", frac, full);
  printf("%.*s", 78 - frac, empty);
  printf("] %9zu/%9zu\n", used, MEMORY_SIZE);
}
#endif

static void image_init(void) {
  memory = malloc(MEMORY_SIZE);
  assert(memory);
  atexit(image_deinit);
}

static void *image_malloc(const size_t size) {
  const size_t mask = 16u - 1u;
  const size_t start = (size_t)memory + used;
  const size_t aligned = (start + mask) & ~mask;
  const size_t offset = aligned - start;
  const size_t aligned_size = size + offset;
  if (used + aligned_size > MEMORY_SIZE) {
    image_debug_printf("image_malloc(%zu): %p\n", size, NULL);
    return NULL;
  }
  used += aligned_size;
  print_used();
  image_debug_printf("image_malloc(%zu): %p\n", size, (void *)aligned);
  return (void *)aligned;
}

static void *image_realloc_sized(void *const p, const size_t old_size,
                                 const size_t new_size) {
  if (new_size == old_size) {
    image_debug_printf("image_realloc_sized(%p, %zu, %zu): %p\n", p, old_size,
                       new_size, p);
    return p;
  }
  if ((size_t)memory + used - old_size == (size_t)p) {
    if (new_size < old_size) {
      used -= old_size - new_size;
      print_used();
      image_debug_printf("image_realloc_sized(%p, %zu, %zu): %p\n", p, old_size,
                         new_size, p);
      return p;
    }
    if (used + new_size - old_size > MEMORY_SIZE) {
      image_debug_printf("image_realloc_sized(%p, %zu, %zu): %p\n", p, old_size,
                         new_size, NULL);
      return NULL;
    }
    used += new_size - old_size;
    print_used();
    image_debug_printf("image_realloc_sized(%p, %zu, %zu): %p\n", p, old_size,
                       new_size, p);
    return p;
  }
  void *const moved = image_malloc(new_size);
  if (moved == NULL) {
    image_debug_printf("image_realloc_sized(%p, %zu, %zu): %p\n", p, old_size,
                       new_size, NULL);
    return NULL;
  }
  memcpy(moved, p, old_size);
  print_used();
  image_debug_printf("image_realloc_sized(%p, %zu, %zu): %p\n", p, old_size,
                     new_size, moved);
  return moved;
}

static void image_free(const void *const p) {
  // can't free without knowing size
  (void)p;
  image_debug_printf("image_free(%p)\n", p);
}

#define STBI_MALLOC(size) image_malloc(size)
#define STBI_REALLOC_SIZED(p, old_size, new_size)                              \
  image_realloc_sized(p, old_size, new_size)
#define STBI_FREE(p) image_free(p)

#define STBI_FAILURE_USERMSG
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
#include "../dep/stb/stb_image.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

typedef struct {
  unsigned char *data;
  int width;
  int height;
} Image;

// Allocating an image registers an atexit() handler to free it. There is no
// unloadImage. Process exit will unload the last loaded image. loading a new
// image will unload the previously loaded image.
static Image loadImage(const char *const restrict path) {
  Slice file = mapFile(path);
  image_debug_printf("file.data: %p\n", file.data);
  if (file.data == NULL)
    return (Image){.data = NULL, .width = 0, .height = 0};
  int width;
  int height;
  if (memory == NULL)
    image_init();
  else
    image_reset();
  unsigned char *data = stbi_load_from_memory(
      (unsigned char *)file.data, (int)file.size, &width, &height, NULL, 1);
  unmapFile(file);
  if (data)
    return (Image){.data = data, .width = width, .height = height};
  boxing_log_args(BoxingLogLevelError, "Failed during loading of %s: %s", path,
                  stbi_failure_reason());
  return (Image){.data = NULL, .width = 0, .height = 0};
}
