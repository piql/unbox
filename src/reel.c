// Experiment for now, run with:
// gcc src/find_frames.c -fsanitize=address -g
// ./a.out dep/ivm_testdata/reel/png

#include <stdlib.h>

#include "../dep/unboxing/tests/testutils/src/config_source_4k_controlframe_v7.h"
#include "grow.c"
#include "load_image.c"
#include "types.h"
#include "unboxer_helpers.c"
#include <boxing/config.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tocdata_c.h>

typedef struct {
  const char *directory_path;
  Slice string_pool;
  size_t string_pool_used;
  uint16_t count;
  uint32_t frames[65536];
} Reel;

bool Reel_init(Reel *reel,
               const char *const
                   directory_path // path to directory containing scanned photos
) {
  DIR *d = opendir(directory_path);
  if (!d)
    return false;
  struct dirent *ent;
  while ((ent = readdir(d))) {
    size_t name_len = strlen(ent->d_name);
    char *name_end = ent->d_name + strlen(ent->d_name);
    long id = strtol(ent->d_name, &name_end, 10);
    if (name_end == ent->d_name)
      continue;
    if (id >= 0 && (unsigned long)id < countof(reel->frames)) {
      if (!grow((void **)&reel->string_pool, 1, &reel->string_pool.size,
                reel->string_pool_used + name_len + 1)) {
        closedir(d);
        return false;
      }
      uint32_t str_start = reel->string_pool_used + 1;
      memcpy((char *)reel->string_pool.data + reel->string_pool_used,
             ent->d_name, name_len + 1);
      reel->string_pool_used += name_len + 1;
      reel->frames[id] = str_start;
      reel->count++;
    }
  }
  closedir(d);
  if (!reel->count)
    return false;
  reel->directory_path = directory_path;
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
  free(reel->string_pool.data);
  free(reel);
}

void Reel_reset(Reel *reel) {
  reel->string_pool_used = 0;
  memset(&reel->frames, 0, sizeof reel->frames);
}

Slice Reel_unbox_control_frame(Reel *reel, bool *is_raw) {
  if (!reel->frames[1])
    return Slice_empty;
  boxing_config *config =
      boxing_config_create_from_structure(&config_source_v7);
  if (!config)
    return Slice_empty;
  char buf[4096];
  {
    int r =
        snprintf(buf, sizeof(buf), "%s/%s", reel->directory_path,
                 (const char *)reel->string_pool.data + reel->frames[1] - 1);
    if (r < 0 || r > (int)sizeof(buf)) {
      boxing_config_free(config);
      return Slice_empty;
    }
  }
  Image img = loadImage(buf);
  if (!img.data) {
    boxing_config_free(config);
    return Slice_empty;
  }
  bool use_raw_decoding = img.width == 4096 && img.height == 2160;
  Unboxer unboxer;
  if (UnboxerCreate(config, use_raw_decoding, &unboxer) != UnboxerInitOK) {
    boxing_config_free(config);
    return Slice_empty;
  }
  Slice result;
  if (UnboxerUnbox(&unboxer, img.data, img.width, img.height,
                   BOXING_METADATA_CONTENT_TYPES_CONTROLFRAME,
                   &result) != UnboxOK) {
    UnboxerDestroy(&unboxer);
    boxing_config_free(config);
    return Slice_empty;
  }
  if (is_raw)
    *is_raw = use_raw_decoding;
  UnboxerDestroy(&unboxer);
  boxing_config_free(config);
  // Assume Control Frame ends with \n
  ((char *)result.data)[--result.size] = '\0';
  return result;
}

Slice Reel_unbox_toc(Reel *reel, Unboxer *unboxer, afs_toc_file *toc) {
  char buf[4096];
  Slice toc_contents = Slice_empty;
  for (int f = toc->start_frame; f <= toc->end_frame; f++) {
    if (!reel->frames[f])
      return Slice_empty;
    {
      int r =
          snprintf(buf, sizeof(buf), "%s/%s", reel->directory_path,
                   (const char *)reel->string_pool.data + reel->frames[f] - 1);
      if (r < 0 || r > (int)sizeof(buf))
        return Slice_empty;
    }
    Image frame = loadImage(buf);
    if (!frame.data)
      return Slice_empty;
    Slice toc_contents_chunk;
    if (UnboxerUnbox(unboxer, frame.data, frame.width, frame.height,
                     BOXING_METADATA_CONTENT_TYPES_TOC,
                     &toc_contents_chunk) != UnboxOK)
      return Slice_empty;
    size_t offset = toc_contents.size;
    if (!grow_exact(&toc_contents.data, 1, &toc_contents.size,
                    toc_contents.size + toc_contents_chunk.size)) {
      if (toc_contents.data)
        free(toc_contents.data);
      return Slice_empty;
    }
    memcpy((char *)toc_contents.data + offset, toc_contents_chunk.data,
           toc_contents_chunk.size);
    if (toc_contents_chunk.data)
      free(toc_contents_chunk.data);
  }
  // Assume TOC ends with \n
  ((char *)toc_contents.data)[--toc_contents.size] = '\0';
  return toc_contents;
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
