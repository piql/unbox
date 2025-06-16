#if 0
: <<E
// clang-format off
E
set -e

# Shell script for simpler building with ./build.c

# Flags:
#   -DRELEASE - Build in release mode
#   -DASAN - Enable address sanitizer (When not release mode)
#   -DTARGET_WINDOWS - Build for Windows (Requires `zig` in path)

echo -e "\x1b[90mgcc -o $(basename $0 .c) "$@" $(basename $0)\x1b[0m"
gcc -o $(basename $0 .c) "$@" $(basename $0)
echo -e "\x1b[90m./$(basename $0 .c)\x1b[0m"
./$(basename $0 .c)
exit $?

// clang-format on
#endif

#include "dev/doctest_util.c"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef _WIN32
#define CC "cl.exe /nologo"
#define CC_DEFINES " -DWIN32"
#define CFLAGS ""
#define TARGET_WINDOWS
#elif defined(__APPLE__)
#define CC "clang"
#define CC_DEFINES ""
#define CFLAGS                                                                 \
  " -g -fsanitize=undefined -Wall -Wextra -Wpedantic -Werror -std=c99"
#define TARGET_MACOS
#elif defined(RELEASE) && !defined(TARGET_WINDOWS)
#define CC "gcc"
#define CC_DEFINES ""
#define CFLAGS " -O3 -s"
#elif defined(TARGET_MACOS)
#define CC "zig cc"
#define CC_DEFINES ""
#define CFLAGS                                                                 \
  " -g -fsanitize=undefined -Wall -Wextra -Wpedantic -Werror -target "         \
  "x86_64-macos"
#elif !defined(TARGET_WINDOWS)
#define CC "gcc"
#define CC_DEFINES ""
#ifdef ASAN
#define CFLAGS                                                                 \
  " -g -fsanitize=address -Wall -Wextra -Wpedantic -Werror -std=c99"
#else
#define CFLAGS " -g -Wall -Wextra -Wpedantic -Werror -std=c99"
#endif
#elif defined(TCC)
#define CC "tcc"
#define CC_DEFINES " -DSTBI_NO_SIMD"
#define CFLAGS " -I$(dirname $(which tcc))/include -L$(dirname $(which tcc))"
#elif defined(ZIGCC)
#define CC "zig cc"
#define CC_DEFINES ""
#define CFLAGS                                                                 \
  " -g -fsanitize=undefined -Wall -Wextra -Wpedantic -Werror -std=c99"
#elif defined(RELEASE) && defined(TARGET_WINDOWS)
#define CC "zig cc"
#define CC_DEFINES ""
#define CFLAGS " -O3 -s -target x86_64-windows"
#elif defined(TARGET_WINDOWS)
#define CC "zig cc"
#define CC_DEFINES ""
#define CFLAGS                                                                 \
  " -g -fsanitize=undefined -Wall -Wextra -Wpedantic -Werror -target "         \
  "x86_64-windows"
#else
#error unreachable
#endif

#include "dev/build/util.c" // uses TARGET_WINDOWS

#include "dev/build/afs.h"
#include "dev/build/mxml.h"
#include "dev/build/rpmalloc.h"
#include "dev/build/unboxing.h"

#define SYSTEM_WITH_LOG(cmd)                                                   \
  (printf("\x1b[90m%s\x1b[0m\n", cmd), fflush(stdout), system(cmd))

#define RUN(exe) SYSTEM_WITH_LOG(exe)

#define DEFINES CC_DEFINES UNBOXING_DEFINES
#define INCLUDES UNBOXING_INCLUDES AFS_INCLUDES MXML_INCLUDES
#ifdef _WIN32
#define LFLAGS ""
#else
#define LFLAGS UNBOXING_LFLAGS
#endif
#ifdef RELEASE
#define SOURCES RPMALLOC_SOURCES UNBOXING_SOURCES AFS_SOURCES MXML_SOURCES
#else
#define SOURCES UNBOXING_SOURCES AFS_SOURCES MXML_SOURCES
#endif

#ifdef _WIN32
#define COMPILE(cc, defines, includes, sources, output, cflags, lflags)        \
  SYSTEM_WITH_LOG(cc defines includes sources lflags cflags                    \
                  " /link /out:" output)
#else
#define COMPILE(cc, defines, includes, sources, output, cflags, lflags)        \
  SYSTEM_WITH_LOG(cc defines includes sources lflags cflags " -o " output)
#endif

bool has_arg(int argc, char *argv[], const char *arg) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], arg) == 0)
      return true;
  }
  return false;
}

#ifdef TARGET_WINDOWS
#define BIN_EXT ".exe"
#else
#define BIN_EXT ""
#endif

#ifdef _WIN32
#define BUILD_STMT(cc, defines, includes, sources, output, cflags, lflags,     \
                   status_variable)                                            \
  status_variable =                                                            \
      COMPILE(cc, defines, includes, sources, output BIN_EXT, cflags, lflags); \
  RUN("del *.obj");                                                            \
  if (status_variable != 0)                                                    \
    return status_variable;                                                    \
  RUN("mt.exe -nologo -manifest dev/build/UTF8.manifest "                      \
      "-outputresource:" output BIN_EXT ";#1");
#else
#define BUILD_STMT(cc, defines, includes, sources, output, cflags, lflags,     \
                   status_variable)                                            \
  do {                                                                         \
    status_variable = COMPILE(cc, defines, includes, sources, output BIN_EXT,  \
                              cflags, lflags);                                 \
    if (status_variable != 0)                                                  \
      return status_variable;                                                  \
  } while (0)
#endif

int main(int argc, char *argv[]) {
  { // Setup
    mkdir("out", 0755);
    mkdir("out/exe", 0755);

#ifndef RELEASE
    mkdir(".vscode", 0755);
    writeVSCodeInfo(INCLUDES, DEFINES);
#endif
  }

  { // Generate doc example program
    Slice doc = mapFile("doc/DETAILED.md");
    MarkdownCodeBlockIteratorC it = {.lit = {.data = doc, .i = 0},
                                     .in_code_block = false};
    size_t n = 0;
    Slice line;
    FILE *c = fopen("dev/doc_example_program.c", "wb");
    while (nextCodeLine(&it, &line)) {
      fwrite(line.data, 1, line.size, c);
      fputc('\n', c);
    }
    fclose(c);
    unmapFile(doc);
  }

  { // Perform builds
    int cc_status;
    BUILD_STMT(CC, "", "", " dev/sha1.c", "out/exe/sha1", CFLAGS, "",
               cc_status);
    BUILD_STMT(CC, DEFINES, INCLUDES " -Idep/unboxing/tests/testutils/src",
               SOURCES " dev/doc_example_program.c",
               "out/exe/doc_example_program", CFLAGS, LFLAGS, cc_status);
    BUILD_STMT(CC, DEFINES, "", " dev/raw_file_to_png.c",
               "out/exe/raw_file_to_png", CFLAGS, "", cc_status);
    BUILD_STMT(CC, DEFINES, INCLUDES, SOURCES " src/main.c", "out/exe/unbox",
               CFLAGS, LFLAGS, cc_status);
  }
  return EXIT_SUCCESS;
}
