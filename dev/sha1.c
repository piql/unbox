#include <stdint.h>
#include <string.h>

typedef struct {
  uint64_t total_size;
  uint32_t h[5];
  uint8_t trailing[64];
} SHA1;

static SHA1 SHA1_init(void) {
  return (SHA1){
      .total_size = 0,
      .h =
          {
              0x67452301,
              0xEFCDAB89,
              0x98BADCFE,
              0x10325476,
              0xC3D2E1F0,
          },
  };
}

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static void SHA1_compress(SHA1 *h, unsigned char *blocks, uint64_t count) {
  // printf("\nSHA1_compress(h, blocks, %lu)\n          ", count);
  uint32_t w[80];
  for (uint64_t block_index = 0; block_index < count; block_index++) {
    uint32_t a = h->h[0];
    uint32_t b = h->h[1];
    uint32_t c = h->h[2];
    uint32_t d = h->h[3];
    uint32_t e = h->h[4];
    unsigned char *block = blocks + block_index * 64;
    for (uint8_t i = 0; i < 16; i++) {
      w[i] = (((uint32_t)(block[(i << 2) + 0]) << 0x18) |
              ((uint32_t)(block[(i << 2) + 1]) << 0x10) |
              ((uint32_t)(block[(i << 2) + 2]) << 0x08) |
              ((uint32_t)(block[(i << 2) + 3]) << 0x00));
    }

    for (uint8_t i = 16; i < 80; i++) {
      uint32_t x = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
      w[i] = ROTATE_LEFT(x, 1);
    }

    for (uint8_t i = 0; i < 80; i++) {
      uint32_t f;
      uint32_t k;
      {
        if (i < 20) {
          f = (b & c) | ((~b) & d);
          k = 0x5A827999;
        } else if (i < 40) {
          f = b ^ c ^ d;
          k = 0x6ED9EBA1;
        } else if (i < 60) {
          f = (b & c) ^ (b & d) ^ (c & d);
          k = 0x8F1BBCDC;
        } else {
          f = b ^ c ^ d;
          k = 0xCA62C1D6;
        }
      }

      uint32_t temp = ROTATE_LEFT(a, 5) + f + e + k + w[i];
      e = d;
      d = c;
      c = ROTATE_LEFT(b, 30);
      b = a;
      a = temp;
    }
    h->h[0] += a;
    h->h[1] += b;
    h->h[2] += c;
    h->h[3] += d;
    h->h[4] += e;
  }
}

static void SHA1_update(SHA1 *h, unsigned char *message, uint64_t size) {
  uint64_t trailing_size = h->total_size % 64;
  uint64_t offset = 0;
  if (trailing_size) {
    // has trailer
    uint64_t size_to_copy =
        size < 64 - trailing_size ? size : 64 - trailing_size;
    memcpy(h->trailing + trailing_size, message, size_to_copy);
    h->total_size += size_to_copy;
    if (h->total_size % 64 == 0)
      SHA1_compress(h, h->trailing, 1);
    offset = size_to_copy;
  }
  uint64_t blocks = (size - offset) / 64;
  if (blocks)
    SHA1_compress(h, message + offset, blocks);
  uint64_t processed_bytes = blocks * 64;
  h->total_size += processed_bytes;
  uint64_t r = (size - offset) % 64; // < 64
  if (r) {
    memcpy(h->trailing, message + offset + processed_bytes, r);
    h->total_size += r;
  }
}

#include <arpa/inet.h>

static void SHA1_digest(SHA1 *h, uint32_t output[5]) {
  uint8_t w[128] = {0};
  uint64_t r = h->total_size % 64;

  if (r)
    memcpy(w, h->trailing, r);
  w[r++] = 0x80;
  while (r % 64 != 56)
    w[r++] = 0;
  uint64_t size_in_bits = h->total_size * 8;
  // printf("r: %lu\n", r);
  w[r++] = size_in_bits >> 0x38;
  w[r++] = size_in_bits >> 0x30;
  w[r++] = size_in_bits >> 0x28;
  w[r++] = size_in_bits >> 0x20;
  w[r++] = size_in_bits >> 0x18;
  w[r++] = size_in_bits >> 0x10;
  w[r++] = size_in_bits >> 0x08;
  w[r++] = size_in_bits >> 0x00;
  SHA1_compress(h, w, r / 64);
  for (uint8_t i = 0; i < 5; i++)
    output[i] = htonl(h->h[i]);
  *h = SHA1_init();
}

// CLI tool

#include "../src/map_file.c"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc == 1) {
    SHA1 h = SHA1_init();
    unsigned char buf[64];
    for (;;) {
      size_t bytes_read = fread(buf, 1, 64, stdin);
      SHA1_update(&h, buf, bytes_read);
      if (bytes_read < 64) {
        int err;
        if ((err = ferror(stdin))) {
          fprintf(stderr, "%s\n", strerror(err));
          return EXIT_FAILURE;
        } else
          break;
      }
    }
    uint32_t digest[5];
    SHA1_digest(&h, digest);
    for (uint8_t i = 0; i < sizeof(digest); i++)
      fprintf(stdout, "%02x", ((unsigned char *)digest)[i]);
    fputs("  -\n", stdout);
    return EXIT_SUCCESS;
  }
  SHA1 h = SHA1_init();
  uint32_t digest[5];
  for (int i = 1; i < argc; i++) {
    Slice file = mapFile(argv[i]);
    if (!file.data) {
      fprintf(stderr, "Failed to read file: %s\n", argv[i]);
      return EXIT_FAILURE;
    }
    SHA1_update(&h, file.data, file.size);
    unmapFile(file);
    SHA1_digest(&h, digest);
    for (uint8_t i = 0; i < sizeof(digest); i++)
      fprintf(stdout, "%02x", ((unsigned char *)digest)[i]);
    fprintf(stdout, "  %s\n", argv[i]);
  }
  return EXIT_SUCCESS;
}

// static void SHA1_print(SHA1 *h) {
//   uint32_t array[5] = {0};
//   SHA1_digest(h, array);
//   for (unsigned i = 0; i < sizeof(array); i++) {
//     printf("%02x", ((unsigned char *)array)[i]);
//   }
// }

// static void SHA1_test_len(const char *s, size_t len, const char *expected,
//                           const char *replacement_string) {
//   SHA1 h = SHA1_init();
//   SHA1_update(&h, (unsigned char *)s, len);
//   printf("SHA1(\"%s\")\n        : ",
//          replacement_string ? replacement_string : s);
//   SHA1_print(&h);
//   printf("\nexpected: %s\n", expected);
// }

// void SHA1_test(const char *s, const char *expected,
//                const char *replacement_string) {
//   return SHA1_test_len(s, strlen(s), expected, replacement_string);
// }

// int main(void) {
//   SHA1_test("", "da39a3ee5e6b4b0d3255bfef95601890afd80709", NULL);
//   SHA1_test("The quick brown fox jumps over the lazy dog",
//             "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12", NULL);
//   SHA1_test("The quick brown fox jumps over the lazy cog",
//             "de9f2c7fd25e1b3afad3e85a0bd17d9b100db4b3", NULL);
//   Slice self = mapFile("src/main.c");
//   SHA1_test_len(self.data, self.size,
//                 "bf12418a9b0d923a49e31b64d505f1089f929698", "src/main.c");
//   unmapFile(self);

//   printf("split test\n");
//   SHA1 h = SHA1_init();
//   SHA1_update(&h, (unsigned char *)"The quick brown fox ", 20);
//   SHA1_update(&h, "jumps over the lazy dog", 23);
//   printf("SHA1(\"%s\")\n        : ",
//          "The quick brown fox \" + \"jumps over the lazy dog");
//   SHA1_print(&h);
//   printf("\nexpected: %s\n", "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
//   Slice file = mapFile("../../../../Documents/reel.raw");
//   uint32_t randoms[] = {
//       489,  1981, 1944, 1381, 2843, 3404, 1603, 2354, 1562, 2907, 283,
//       3937, 661,  2049, 2904, 1358, 2616, 1294, 2416, 918,  2147, 2977,
//       595,  1333, 3799, 2572, 1548, 2398, 2719, 1507, 53,   1182, 3561,
//       2595, 1007, 876, 1201, 1696, 3583, 3938, 1798, 236,  2906, 2075,
//       3603, 782,  1165, 3621, 329,  1153, 745,  3547, 1799, 3005, 3326,
//       2432, 3645, 3808, 2379, 3387, 3817, 2024, 936,  256,  873,  2198,
//       1073, 2502, 28,   2852, 717,  2682, 2096, 3202, 481,  2586, 1522,
//       3966, 3858, 1929, 1715, 283,  1514, 2562, 1609, 2614, 3315, 1891,
//       2244, 3319, 927,  232,  2225, 2121, 3645, 491, 561,  1712, 3196,
//       136};
//   size_t bytes_hashed = 0;
//   SHA1 h2 = SHA1_init();
//   uint8_t r = 0;
//   while (bytes_hashed < file.size) {
//     uint32_t cur = min(randoms[r], file.size - bytes_hashed);
//     r = (r + 1) % 100;
//     // if (r % 8 == 0)
//     //   printf("file   [%zu..%zu] (%u)\n(total: %zu)\n", bytes_hashed,
//     //          bytes_hashed + cur, cur, file.size);
//     SHA1_update(&h2, file.data + bytes_hashed, cur);
//     bytes_hashed += cur;
//   }
//   SHA1_print(&h2);
//   unmapFile(file);
//   return 0;
// }
