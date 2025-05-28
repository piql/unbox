#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "map_file.c"

static void *memory = NULL;
static size_t used = 0;
#define MEMORY_SIZE (256ul * 1024ul * 1024ul)

static void image_reset(void) { used = 0; }

static void image_deinit(void) { free(memory); }

static void image_init(void) {
  memory = malloc(MEMORY_SIZE);
  assert(memory);
  atexit(image_deinit);
}

static void *image_malloc(const size_t size) {
  if (memory == NULL)
    image_init();
  const size_t mask = 16u - 1u;
  const size_t start = (size_t)memory + used;
  const size_t aligned = (start + mask) & ~mask;
  const size_t offset = aligned - start;
  const size_t aligned_size = size + offset;
  if (used + aligned_size > MEMORY_SIZE)
    return NULL;
  used += aligned_size;
  return (void *)aligned;
}

static void *image_realloc_sized(void *const p, const size_t old_size,
                                 const size_t new_size) {
  if (new_size == old_size)
    return p;
  if (memory == NULL)
    image_init();
  if ((size_t)memory + used - old_size == (size_t)p) {
    if (new_size < old_size) {
      used -= old_size - new_size;
      return p;
    }
    if (used + new_size - old_size > MEMORY_SIZE)
      return NULL;
    used += new_size - old_size;
    return p;
  }
  void *const moved = image_malloc(new_size);
  if (moved == NULL)
    return NULL;
  memcpy(moved, p, old_size);
  return moved;
}

static void image_free(const void *const p) {
  // can't free without knowing size
  (void)p;
}

#define STBI_MALLOC(size) image_malloc(size)
#define STBI_REALLOC_SIZED(p, old_size, new_size)                              \
  image_realloc_sized(p, old_size, new_size)
#define STBI_FREE(p) image_free(p)

#define STB_IMAGE_IMPLEMENTATION
#include "../dep/stb/stb_image.h"

typedef struct {
  unsigned char *data;
  int width;
  int height;
} Image;

// Allocating an image registers an atexit() handler to free it. There is no
// unloadImage. Process exit will unload the last loaded image. loading a new
// image will unload the previously loaded image.
Image loadImage(const char *const restrict path) {
  FileMap file = mapFile(path);
  if (file.data == NULL)
    return (Image){.data = NULL, .width = 0, .height = 0};
  int width;
  int height;
  image_reset();
  unsigned char *data = stbi_load_from_memory(
      (unsigned char *)file.data, (int)file.size, &width, &height, NULL, 1);
  unmapFile(&file);
  return data == NULL ? (Image){.data = NULL, .width = 0, .height = 0}
                      : (Image){.data = data, .width = width, .height = height};
}
