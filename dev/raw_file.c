#include "../src/types.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  uint64_t frame_id;
  uint32_t frame_height;
  uint32_t frame_width;
  uint8_t color_depth;
  uint8_t version;
  uint8_t colors_per_channel;
  uint8_t reserved_[13];
} RawFileHeader;

static void printHeader(const RawFileHeader *const h) {
  char buf[256];
  int len = snprintf(buf, sizeof buf,
                     "Header#%05" PRIu64 " v%" PRIu8 " %" PRIu32 "x%" PRIu32
                     " %" PRIu8 "bpp %" PRIu8 "cpc",
                     h->frame_id, h->version, h->frame_width, h->frame_height,
                     h->color_depth, h->colors_per_channel);
  fwrite(buf, 1, (size_t)len, stdout);
}

typedef struct {
  uint8_t reserved_[24];
  uint8_t crc[8];
} RawFileFooter;

static int writeCRC64(const unsigned char *restrict const crc,
                      char *restrict const out) {
  return snprintf(out, 17,
                  "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8
                  "%02" PRIx8 "%02" PRIx8 "%02" PRIx8,
                  crc[0], crc[1], crc[2], crc[3], crc[4], crc[5], crc[6],
                  crc[7]);
}

static void printFooter(const RawFileFooter *const f) {
  char buf[256];
  int len = snprintf(buf, sizeof buf, "Footer ");
  len += writeCRC64(f->crc, buf + len);
  fwrite(buf, 1, (size_t)len, stdout);
}

static bool splat_pixels(const uint8_t *const restrict data,
                         const uint32_t width, const uint32_t height,
                         const uint8_t color_depth, Slice *const output_image) {
  const size_t frame_size = width * height;
  if (output_image->size < frame_size) {
    void *const new_output_image = realloc(output_image->data, frame_size);
    if (!new_output_image)
      return false;
    output_image->data = new_output_image;
    output_image->size = frame_size;
  }

  uint8_t *const out_data = (uint8_t *)output_image->data;
  if (color_depth == 8)
    memcpy(out_data, data, frame_size);
  else if (color_depth == 2) {
    for (unsigned i = 0; i < frame_size >> 2; i++) {
      uint32_t x = data[i];
      x |= x << 6;
      x |= x << 12;
      x &= 0x03030303;
      x *= 85;
      ((uint32_t *)out_data)[i] = x;
    }
  } else if (color_depth == 1) {
    for (unsigned i = 0; i < frame_size >> 3; i++) {
      out_data[i * 8 + 0] = ((data[i] & (1 << 0)) >> 0) * 255;
      out_data[i * 8 + 1] = ((data[i] & (1 << 1)) >> 1) * 255;
      out_data[i * 8 + 2] = ((data[i] & (1 << 2)) >> 2) * 255;
      out_data[i * 8 + 3] = ((data[i] & (1 << 3)) >> 3) * 255;
      out_data[i * 8 + 4] = ((data[i] & (1 << 4)) >> 4) * 255;
      out_data[i * 8 + 5] = ((data[i] & (1 << 5)) >> 5) * 255;
      out_data[i * 8 + 6] = ((data[i] & (1 << 6)) >> 6) * 255;
      out_data[i * 8 + 7] = ((data[i] & (1 << 7)) >> 7) * 255;
    }
  } else
    return false;
  return true;
}
