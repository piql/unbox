#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

typedef struct {
#ifdef _WIN32
  HANDLE handle;
  WIN32_FIND_DATAA file_data;
#else
  DIR *dir;
#endif
} DirIterator;

typedef struct {
  char name[256];
} DirEntry;

static bool dir_start(const char *const path, DirIterator *const out) {
#ifdef _WIN32
  char buf[4096];
  int result = snprintf(buf, sizeof buf, "%s\\*", path);
  if (result < 0 || (unsigned)result > sizeof buf)
    return false;
  WIN32_FIND_DATAA file_data;
  HANDLE h = FindFirstFileA(buf, &file_data);
  if (h == INVALID_HANDLE_VALUE)
    return false;
  *out = (DirIterator){.handle = h, .file_data = file_data};
  return true;
#else
  DIR *dir = opendir(path);
  if (!dir)
    return false;
  *out = (DirIterator){.dir = dir};
  return true;
#endif
}

static bool dir_next(DirIterator *const it, DirEntry *const out) {
#ifdef _WIN32
  if (it->handle == INVALID_HANDLE_VALUE)
    return false;
  size_t name_len = strlen(it->file_data.cFileName);
  if (name_len > 255)
    return false;
  memcpy(out->name, &it->file_data.cFileName, name_len);
  out->name[name_len] = '\0';
  if (!FindNextFile(it->handle, &it->file_data)) {
    FindClose(it->handle);
    it->handle = INVALID_HANDLE_VALUE;
  }
  return true;
#else
  struct dirent *entry = readdir(it->dir);
  if (!entry)
    return false;
  size_t name_len = strlen(entry->d_name);
  if (name_len > 255)
    return false;
  memcpy(out->name, entry->d_name, name_len);
  out->name[name_len] = '\0';
  return true;
#endif
}

static void dir_end(DirIterator *const it) {
#ifdef _WIN32
  if (it->handle != INVALID_HANDLE_VALUE)
    FindClose(it->handle);
#else
  closedir(it->dir);
#endif
}
