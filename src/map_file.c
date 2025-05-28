#include <stddef.h>

typedef struct {
  // NULL if anything failed
  void *data;
  size_t size;
} FileMap;

static FileMap mapFile(const char *const restrict path);

#ifdef _WIN32
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static FileMap mapFile(const char *const restrict path) {
  int fd = open(path, O_RDONLY);
  if (fd == -1)
    return (FileMap){.data = NULL, .size = 0};
  struct stat statbuf;
  if (fstat(fd, &statbuf) == -1) {
    close(fd);
    return (FileMap){.data = NULL, .size = 0};
  }
  void *data = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  return data == MAP_FAILED ? (FileMap){.data = NULL, .size = 0}
                            : (FileMap){.data = data, .size = statbuf.st_size};
}
static void unmapFile(FileMap *file) { munmap(file->data, file->size); }
#endif
