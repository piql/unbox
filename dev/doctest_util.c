#include "../src/map_file.c"
#include "build/unboxing.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  Slice data;
  size_t i;
} LineIterator;

bool nextLine(LineIterator *it, Slice *line) {
  if (it->i >= it->data.size)
    return false;
  size_t i;
  for (i = it->i; i < it->data.size; i++) {
    if (((char *)it->data.data)[i] == '\n') {
      *line = (Slice){.data = (char *)it->data.data + it->i, .size = i - it->i};
      it->i = i + 1;
      return true;
    }
  }
  if (it->i == i)
    return false;
  *line = (Slice){.data = (char *)it->data.data + it->i, .size = i - it->i};
  it->i = i + 1;
  return true;
}

typedef struct {
  LineIterator lit;
  bool in_code_block;
} MarkdownCodeBlockIteratorC;

bool nextCodeLine(MarkdownCodeBlockIteratorC *it, Slice *code_line) {
  Slice line;
  while (nextLine(&it->lit, &line)) {
    if (it->in_code_block) {
      if (line.size == 3 && strncmp(line.data, "```", 3) == 0) {
        it->in_code_block = false;
        continue;
      }
      *code_line = line;
      printf("it { i = %zu, in_code_block = %s }\n", it->lit.i,
             it->in_code_block ? "true" : "false");
      return true;
    } else if (line.size == 4 && strncmp(line.data, "```c", 4) == 0)
      it->in_code_block = true;
  }
  printf("it { i = %zu, in_code_block = %s }\n", it->lit.i,
         it->in_code_block ? "true" : "false");
  return false;
}
