#include "build/util.c"
#include "doctest_util.c"

#ifdef _WIN32
#include "../src/win32.h"
#else
#include <unistd.h>
#endif

#ifndef INCLUDES
#define INCLUDES
#endif

int main(void) {
  const char *includes[] = {
#define X(PATH) #PATH,
      INCLUDES
#undef X
  };
  char cwd[4096];
  memset(cwd, 0, sizeof cwd);
#ifdef _WIN32
  GetCurrentDirectoryA(sizeof cwd, cwd);
#else
  char *r = getcwd(cwd, sizeof cwd);
  (void)r;
#endif
  size_t cwd_len = strlen(cwd);
  cwd[cwd_len++] = '/';
  cwd[cwd_len] = '\0';
  for (size_t i = 0; i < countof(includes); i++) {
    if (strncmp(cwd, includes[i], cwd_len) == 0) {
      includes[i] = includes[i] + cwd_len;
    }
  }
  mkdir(".vscode", 0755);
  writeVSCodeInfo(includes, countof(includes), NULL, 0);
  return !markdownToC("doc/DETAILED.md", "dev/doc_example_program.c");
}
