#include "../dep/afs/src/sha1hash.c"
#include "../src/map_file.c"
#include <assert.h>

int main(int argc, char *argv[]) {
  afs_hash1_state h;
  unsigned char digest[20];
  char digest_str[40];
  if (argc == 1) {
    int r = afs_sha1_init(&h);
    assert(r == CRYPT_OK);
    unsigned char buf[64];
    for (;;) {
      size_t bytes_read = fread(buf, 1, 64, stdin);
      r = afs_sha1_process(&h, buf, (unsigned long)bytes_read);
      assert(r == CRYPT_OK);
      if (bytes_read < 64) {
        int err;
        if ((err = ferror(stdin))) {
          fprintf(stderr, "%s\n", strerror(err));
          return EXIT_FAILURE;
        } else
          break;
      }
    }
    r = afs_sha1_done(&h, digest);
    assert(r == CRYPT_OK);
    r = afs_sha1_hash_to_hex_string(digest, digest_str);
    assert(r == CRYPT_OK);
    fprintf(stdout, "%.*s  -\n", 40, digest_str);
    return EXIT_SUCCESS;
  }
  int r = afs_sha1_init(&h);
  assert(r == CRYPT_OK);
  for (int i = 1; i < argc; i++) {
    Slice file = mapFile(argv[i]);
    if (!file.data) {
      fprintf(stderr, "Failed to read file: %s\n", argv[i]);
      return EXIT_FAILURE;
    }
    r = afs_sha1_process(&h, file.data, (unsigned long)file.size);
    assert(r == CRYPT_OK);
    unmapFile(file);
    r = afs_sha1_done(&h, digest);
    assert(r == CRYPT_OK);
    r = afs_sha1_hash_to_hex_string(digest, digest_str);
    assert(r == CRYPT_OK);
    fprintf(stdout, "%.*s  %s\n", 40, digest_str, argv[i]);
  }
}
