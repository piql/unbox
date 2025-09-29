#include "types.h"

static Slice mapFile(const char *const restrict path);
static void unmapFile(Slice file);

#ifdef _WIN32
#include <windows.h>

static Slice mapFile(const char *const restrict path) {
  HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file == INVALID_HANDLE_VALUE)
    return Slice_empty;
  LARGE_INTEGER size;
  if (GetFileSizeEx(file, &size) == 0 || size.QuadPart == 0) {
    CloseHandle(file);
    return Slice_empty;
  }
  HANDLE map = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, NULL);
  CloseHandle(file);
  if (map == NULL) {
    return Slice_empty;
  }
  void *data = MapViewOfFile(map, FILE_MAP_READ, 0, 0, (size_t)size.QuadPart);
  CloseHandle(map);
  return data == NULL ? Slice_empty
                      : (Slice){.data = data, .size = (size_t)size.QuadPart};
}

static void unmapFile(Slice file) { UnmapViewOfFile(file.data); }

#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static Slice mapFile(const char *const restrict path) {
  int fd = open(path, O_RDONLY);
  if (fd == -1)
    return Slice_empty;
  struct stat statbuf;
  if (fstat(fd, &statbuf) == -1) {
    close(fd);
    return Slice_empty;
  }
  void *data =
      mmap(NULL, (size_t)statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  return data == MAP_FAILED
             ? Slice_empty
             : (Slice){.data = data, .size = (size_t)statbuf.st_size};
}
static void unmapFile(Slice file) { munmap(file.data, file.size); }
#endif
