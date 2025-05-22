#include <stddef.h>
#include <stdio.h>
#include <string.h>

struct IncDefIterator {
  const char *s;
  char flag; // 'I' or 'D'
  size_t len;
  size_t i;
};
static const char *nextIncDef(struct IncDefIterator *it, size_t *len_out) {
  size_t len = 0;
  const char *start = NULL;
  char found_dash_i = 0;
  while (it->i < it->len) {
    const char c = it->s[it->i++];
    if (found_dash_i == 2) {
      if (c != ' ')
        len++;
      else if (len > 0) {
        *len_out = len;
        return start;
      } else
        found_dash_i = 0;
    }
    if (found_dash_i == 1 && c == it->flag) {
      start = it->s + it->i;
      found_dash_i++;
      continue;
    }
    if (found_dash_i == 0 && c == '-') {
      found_dash_i++;
      continue;
    }
  }
  if (len > 0) {
    *len_out = len;
    return start;
  }
  return NULL;
}

static void writeAsJSONString(FILE *f, const char *s, size_t len) {
  size_t i = 0;
  while (i < len) {
    const char c = s[i++];
    if (c == '"')
      fputs("\\\"", f);
    else
      fputc(c, f);
  }
}

static void writeIncDefJSONRows(FILE *f, const char *incdefs,
                                size_t incdefs_len, char flag) {
  struct IncDefIterator it = {
      .s = incdefs, .flag = flag, .len = incdefs_len, .i = 0};
  size_t len;
  const char *incdef = nextIncDef(&it, &len);
  fputs("        \"", f);
  writeAsJSONString(f, incdef, len);
  fputc('"', f);
  while ((incdef = nextIncDef(&it, &len))) {
    fputs(",\n        \"", f);
    writeAsJSONString(f, incdef, len);
    fputc('"', f);
  }
}

static void writeVSCodeInfo(const char *includes, const char *defines) {
  FILE *f = fopen(".vscode/c_cpp_properties.json", "w");

  fputs("{\n  \"configurations\": [\n    {\n      \"name\": \"Linux\",\n      "
        "\"includePath\": [\n",
        f);

  writeIncDefJSONRows(f, includes, strlen(includes), 'I');

  fputs("\n      ],\n      \"defines\": [\n", f);

  writeIncDefJSONRows(f, defines, strlen(defines), 'D');

  fputs("\n      ]\n    }\n  ],\n  \"version\": 4\n}\n", f);

  fclose(f);
}
