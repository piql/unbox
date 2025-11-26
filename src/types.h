#ifndef UNBOX_CLI_TYPES_H
#define UNBOX_CLI_TYPES_H

#ifdef _WIN32
#include <stdlib.h>
#else
#ifndef min
#define min(x, y) ((y) < (x) ? (y) : (x))
#endif
#ifndef max
#define max(x, y) ((x) < (y) ? (y) : (x))
#endif
#endif
#define clamp(x, lo, hi) ((x) < (lo) ? (lo) : (hi) < (x) ? (hi) : (x))
#define countof(x) (sizeof(x) / sizeof *(x))
#define absolute_float(x) ((x) < 0 ? -(x) : (x))
#define nonzeroish_float(x) (absolute_float(x) > 1e-6f)

#include <stddef.h>

typedef struct {
  void *data;
  size_t size;
} Slice;

static const Slice Slice_empty = {.data = NULL, .size = 0};

#include <string.h>

#define sliceof(s) ((Slice){.data = s, .size = countof(s)})
#define str(s) ((Slice){.data = s, .size = strlen(s)})

#endif
