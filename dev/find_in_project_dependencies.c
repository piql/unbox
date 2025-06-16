/**
 * gcc -g -Wall -Wextra -Wpedantic -Werror dev/find_in_project_dependencies.c -fsanitize=address -o find_project_deps
 * 
 * ./find_project_deps | tee deps
 * 
 * pushd dep/unboxing
 * 
 * find -type f -printf '%P\n' | grep -Fxvf ../../deps > ../../unused
 * 
 * popd
 */

#define _POSIX_C_SOURCE 200809
#include "../src/types.h"
#include "build/unboxing.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printList(const Slice *const list, const size_t list_len) {
  for (size_t i = 0; i < list_len; i++)
    printf("%zu) '%.*s'\n", i, (int)list[i].size, (char *)list[i].data);
}

bool isInList(const Slice *const list, const size_t list_len,
              const Slice value) {
  for (size_t i = 0; i < list_len; i++)
    if (list[i].size == value.size &&
        strncmp(list[i].data, value.data, value.size) == 0)
      return true;
  return false;
}

void addToListDuped(Slice **list, size_t *list_len, Slice value) {
  *list = realloc(*list, sizeof **list * (*list_len + 1));
  value.data = strndup(value.data, value.size);
  (*list)[(*list_len)++] = value;
}

void findAndPrintDependencies(const char *const prefix, const size_t prefix_len,
                              Slice **emitted_files, size_t *emitted_files_len,
                              const Slice source_file) {
  if (source_file.size == 0)
    return;
  if (isInList(*emitted_files, *emitted_files_len, source_file))
    return;
  char buf[4096];
  snprintf(buf, sizeof buf,
           "gcc" UNBOXING_DEFINES UNBOXING_INCLUDES
           " -Idep/unboxing/tests/testutils/inc"
           " \"%.*s%.*s\" -M",
           (int)prefix_len, prefix, (int)source_file.size,
           (char *)source_file.data);
  fprintf(stderr, "\x1b[90mrunning '%s':\x1b[0m\n", buf);
  FILE *gcc = popen(buf, "r");
  char *line = NULL;
  size_t line_len = 0;
  // discard first line ("foo.o: foo.c")
  getline(&line, &line_len, gcc);
  while (getline(&line, &line_len, gcc) >= 0) {
    size_t start = 0;
    for (size_t i = 0; i < strlen(line); i++) {
      if (i != strlen(line) - 1 && line[i] != ' ')
        continue;
      Slice path = {.data = &line[start], .size = i - start};
      start = i + 1;
      if (path.size == 0)
        continue;
      if (((char *)path.data)[path.size - 1] == ':')
        continue;
      if (strncmp(path.data, prefix, prefix_len) != 0)
        continue;
      // cut prefix off path:
      path = (Slice){.data = (char *)path.data + prefix_len,
                     .size = path.size - prefix_len};
      if (source_file.size == path.size &&
          strncmp(source_file.data, path.data, path.size) == 0)
        continue;
      findAndPrintDependencies(prefix, prefix_len, emitted_files,
                               emitted_files_len, path);
      //   printf("checking list for path: '%.*s': %s\n", (int)path.size,
      //          (char *)path.data,
      //          isInList(*emitted_files, *emitted_files_len, path)
      //              ? "is in list"
      //              : "not in list");
      //   printList(*emitted_files, *emitted_files_len);
      if (isInList(*emitted_files, *emitted_files_len, path))
        continue;
      addToListDuped(emitted_files, emitted_files_len, path);
      printf("%.*s\n", (int)path.size, (char *)path.data);
    }
  }
  if (line)
    free(line);
  pclose(gcc);
}

int main(void) {
  const char *prefix = "dep/unboxing/";
  size_t prefix_len = strlen(prefix);
  Slice *emitted_files = NULL;
  size_t emitted_files_len = 0;
  size_t start = 0;
  char *source_files =
      UNBOXING_SOURCES " dep/unboxing/tests/testutils/src/boxing_config.c "
                       "dep/unboxing/tests/testutils/src/unboxingutility.c "
                       "dep/unboxing/tests/unboxer/main.c";
  for (size_t i = 0; i <= strlen(source_files); i++) {
    if (i != strlen(source_files) && source_files[i] != ' ')
      continue;
    Slice source_file = {.data = &source_files[start], .size = i - start};
    start = i + 1;
    // printf("[%zu]: '%.*s'\n", source_file.size, (int)source_file.size,
    //        (char *)source_file.data);
    if (source_file.size == 0)
      continue;
    // cut prefix off source_file:
    source_file = (Slice){.data = (char *)source_file.data + prefix_len,
                          .size = source_file.size - prefix_len};
    findAndPrintDependencies(prefix, prefix_len, &emitted_files,
                             &emitted_files_len, source_file);
    printf("%.*s\n", (int)source_file.size, (char *)source_file.data);
    // if (i > 1000)
    //   break;
  }
  for (size_t i = 0; i < emitted_files_len; i++)
    free(emitted_files[i].data);
  free(emitted_files);
  return EXIT_SUCCESS;
}