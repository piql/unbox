#include "../src/map_file.c"
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
  for (i = it->i; i < it->data.size; i++)
    if (((char *)it->data.data)[i] == '\n') {
      *line = (Slice){.data = (char *)it->data.data + it->i, .size = i - it->i};
      it->i = i + 1;
      return true;
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
    if (line.size > 0 && ((char *)line.data)[line.size - 1] == '\r')
      line.size -= 1;
    if (it->in_code_block) {
      if (line.size == 3 && strncmp(line.data, "```", 3) == 0) {
        it->in_code_block = false;
        continue;
      }
      *code_line = line;
      return true;
    } else if (line.size == 4 && strncmp(line.data, "```c", 4) == 0)
      it->in_code_block = true;
  }
  return false;
}

bool markdownToC(const char *const markdownInputFile,
                 const char *const cOutputFile) {
  Slice doc = mapFile(markdownInputFile);
  if (!doc.data)
    return false;
  FILE *c = fopen(cOutputFile, "wb");
  if (!c) {
    unmapFile(doc);
    return false;
  }
  MarkdownCodeBlockIteratorC it = {.lit = {.data = doc}};
  Slice line;
  while (nextCodeLine(&it, &line)) {
    if (fwrite(line.data, 1, line.size, c) < line.size ||
        fputc('\n', c) == EOF) {
      fclose(c);
      unmapFile(doc);
      return false;
    }
  }
  fclose(c);
  unmapFile(doc);
  return true;
}
