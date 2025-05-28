#include <stddef.h>

typedef struct {
  // NULL if anything failed
  void *data;
  size_t size;
} FileMap;

static FileMap mapFile(const char *const restrict path);
static void unmapFile(FileMap *file);

#ifdef _WIN32
#include <windows.h>

static FileMap mapFile(const char *const restrict path) {
  HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file == INVALID_HANDLE_VALUE)
    return (FileMap){.data = NULL, .size = 0};
  LARGE_INTEGER size;
  if (GetFileSizeEx(file, &size) == 0 || size.QuadPart == 0) {
    CloseHandle(file);
    return (FileMap){.data = NULL, .size = 0};
  }
  HANDLE map = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, NULL);
  CloseHandle(file);
  if (map == NULL) {
    return (FileMap){.data = NULL, .size = 0};
  }
  void *data = MapViewOfFile(map, FILE_MAP_READ, 0, 0, size.QuadPart);
  CloseHandle(map);
  return data == NULL ? (FileMap){.data = NULL, .size = 0}
                      : (FileMap){.data = data, .size = size.QuadPart};
}

static void unmapFile(FileMap *file) { UnmapViewOfFile(file->data); }

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
