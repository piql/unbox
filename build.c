#if 0
: <<E
// clang-format off
E

# Shell script for simpler building with ./build.c

# Flags:
#   -DRELEASE - Build in release mode
#   -DTEST - Test (run unbox)
#   -DASAN - Enable address sanitizer (When not release mode)
#   -DTARGET_WINDOWS - Build for Windows (Requires `zig` in path)

echo -e "\x1b[90mgcc -o $(basename $0 .c) "$@" $(basename $0)\x1b[0m"
set -e
gcc -o $(basename $0 .c) "$@" $(basename $0)
echo -e "\x1b[90m./$(basename $0 .c)\x1b[0m"
./$(basename $0 .c)
exit $?

// clang-format on
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef _WIN32
#define CC "cl.exe /nologo"
#define CC_DEFINES " -DWIN32"
#define CFLAGS ""
#define TARGET_WINDOWS
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

#ifdef _WIN32
#define RAND_FILE_DEFINE " -DRAND_FILE=\"\"\"randfile\"\"\""
#else
#define RAND_FILE_DEFINE " -DRAND_FILE='\"randfile\"'"
#endif

#ifdef RELEASE
#define UNBOXING_DEFINES RAND_FILE_DEFINE
#else
#define UNBOXING_DEFINES " -D_DEBUG" RAND_FILE_DEFINE
#endif

#define UNBOXING_LINK " -lm"

#define UNBOXING_INCLUDES                                                      \
  " -Idep/unboxing/inc"                                                        \
  " -Idep/unboxing/src/unboxer"                                                \
  " -Idep/unboxing/thirdparty/bch"                                             \
  " -Idep/unboxing/thirdparty/glib"                                            \
  " -Idep/unboxing/thirdparty/LDPC-codes"                                      \
  " -Idep/unboxing/thirdparty/reedsolomon"                                     \
  ""

#define UNBOXING_SOURCES                                                       \
  " dep/unboxing/src/base/math_crc32.c"                                        \
  " dep/unboxing/src/base/math_crc64.c"                                        \
  " dep/unboxing/src/codecs/2dpam.c"                                           \
  " dep/unboxing/src/codecs/bchcodec.c"                                        \
  " dep/unboxing/src/codecs/cipher.c"                                          \
  " dep/unboxing/src/codecs/codecbase.c"                                       \
  " dep/unboxing/src/codecs/codecdispatcher.c"                                 \
  " dep/unboxing/src/codecs/crc32.c"                                           \
  " dep/unboxing/src/codecs/crc64.c"                                           \
  " dep/unboxing/src/codecs/ftfinterleaving.c"                                 \
  " dep/unboxing/src/codecs/interleaving.c"                                    \
  " dep/unboxing/src/codecs/ldpccodec.c"                                       \
  " dep/unboxing/src/codecs/modulator.c"                                       \
  " dep/unboxing/src/codecs/packetheader.c"                                    \
  " dep/unboxing/src/codecs/reedsolomon.c"                                     \
  " dep/unboxing/src/codecs/symbolconverter.c"                                 \
  " dep/unboxing/src/codecs/syncpointinserter.c"                               \
  " dep/unboxing/src/config.c"                                                 \
  " dep/unboxing/src/frame/2polinomialsampler.c"                               \
  " dep/unboxing/src/frame/areasampler.c"                                      \
  " dep/unboxing/src/frame/sampler.c"                                          \
  " dep/unboxing/src/frame/trackergpf_1.c"                                     \
  " dep/unboxing/src/frame/trackergpf.c"                                       \
  " dep/unboxing/src/globals.c"                                                \
  " dep/unboxing/src/graphics/border.c"                                        \
  " dep/unboxing/src/graphics/calibrationbar.c"                                \
  " dep/unboxing/src/graphics/component.c"                                     \
  " dep/unboxing/src/graphics/contentcontainer.c"                              \
  " dep/unboxing/src/graphics/genericframe.c"                                  \
  " dep/unboxing/src/graphics/genericframefactory.c"                           \
  " dep/unboxing/src/graphics/genericframegpf_1.c"                             \
  " dep/unboxing/src/graphics/label.c"                                         \
  " dep/unboxing/src/graphics/metadatabar.c"                                   \
  " dep/unboxing/src/graphics/painter.c"                                       \
  " dep/unboxing/src/graphics/referencebar.c"                                  \
  " dep/unboxing/src/graphics/referencepoint.c"                                \
  " dep/unboxing/src/image8.c"                                                 \
  " dep/unboxing/src/log.c"                                                    \
  " dep/unboxing/src/math/bitutils.c"                                          \
  " dep/unboxing/src/math/dsp.c"                                               \
  " dep/unboxing/src/math/math.c"                                              \
  " dep/unboxing/src/matrix.c"                                                 \
  " dep/unboxing/src/metadata.c"                                               \
  " dep/unboxing/src/string.c"                                                 \
  " dep/unboxing/src/unboxer.c"                                                \
  " dep/unboxing/src/unboxer/cornermark.c"                                     \
  " dep/unboxing/src/unboxer/datapoints.c"                                     \
  " dep/unboxing/src/unboxer/filter.c"                                         \
  " dep/unboxing/src/unboxer/frametrackerutil.c"                               \
  " dep/unboxing/src/unboxer/frameutil.c"                                      \
  " dep/unboxing/src/unboxer/histogramutils.c"                                 \
  " dep/unboxing/src/unboxer/horizontalmeasures.c"                             \
  " dep/unboxing/src/unboxer/sampleutil.c"                                     \
  " dep/unboxing/src/unboxer/syncpoints.c"                                     \
  " dep/unboxing/src/unboxer/unboxerv1.c"                                      \
  " dep/unboxing/src/utils.c"                                                  \
  " dep/unboxing/thirdparty/bch/bch.c"                                         \
  " dep/unboxing/thirdparty/glib/g_variant.c"                                  \
  " dep/unboxing/thirdparty/glib/ghash.c"                                      \
  " dep/unboxing/thirdparty/glib/glist.c"                                      \
  " dep/unboxing/thirdparty/glib/gvector.c"                                    \
  " dep/unboxing/thirdparty/LDPC-codes/alloc.c"                                \
  " dep/unboxing/thirdparty/LDPC-codes/check.c"                                \
  " dep/unboxing/thirdparty/LDPC-codes/dec.c"                                  \
  " dep/unboxing/thirdparty/LDPC-codes/distrib.c"                              \
  " dep/unboxing/thirdparty/LDPC-codes/enc.c"                                  \
  " dep/unboxing/thirdparty/LDPC-codes/intio.c"                                \
  " dep/unboxing/thirdparty/LDPC-codes/mod2convert.c"                          \
  " dep/unboxing/thirdparty/LDPC-codes/mod2dense.c"                            \
  " dep/unboxing/thirdparty/LDPC-codes/mod2sparse.c"                           \
  " dep/unboxing/thirdparty/LDPC-codes/open.c"                                 \
  " dep/unboxing/thirdparty/LDPC-codes/rand.c"                                 \
  " dep/unboxing/thirdparty/LDPC-codes/rcode.c"                                \
  " dep/unboxing/thirdparty/reedsolomon/galois_field.c"                        \
  " dep/unboxing/thirdparty/reedsolomon/rs.c"                                  \
  ""

#define MXML_INCLUDES " -Idep/afs/thirdparty/minixml/inc"
#define MXML_SOURCES                                                           \
  " dep/afs/thirdparty/minixml/src/mxml-attr.c"                                \
  " dep/afs/thirdparty/minixml/src/mxml-entity.c"                              \
  " dep/afs/thirdparty/minixml/src/mxml-file.c"                                \
  " dep/afs/thirdparty/minixml/src/mxml-get.c"                                 \
  " dep/afs/thirdparty/minixml/src/mxml-node.c"                                \
  " dep/afs/thirdparty/minixml/src/mxml-private.c"                             \
  " dep/afs/thirdparty/minixml/src/mxml-search.c"                              \
  " dep/afs/thirdparty/minixml/src/mxml-string.c"                              \
  ""

#define AFS_INCLUDES                                                           \
  " -Idep/afs/inc"                                                             \
  ""

#define AFS_SOURCES                                                            \
  " dep/afs/src/administrativemetadata.c"                                      \
  " dep/afs/src/controldata.c"                                                 \
  " dep/afs/src/controlframe/boxingformat.c"                                   \
  " dep/afs/src/framerange.c"                                                  \
  " dep/afs/src/frameranges.c"                                                 \
  " dep/afs/src/technicalmetadata.c"                                           \
  " dep/afs/src/toc/previewlayoutdefinition.c"                                 \
  " dep/afs/src/toc/previewlayoutdefinitions.c"                                \
  " dep/afs/src/toc/previewsection.c"                                          \
  " dep/afs/src/toc/previewsections.c"                                         \
  " dep/afs/src/tocdata.c"                                                     \
  " dep/afs/src/tocdatafilemetadata.c"                                         \
  " dep/afs/src/tocdatafilemetadatasource.c"                                   \
  " dep/afs/src/tocdatareel.c"                                                 \
  " dep/afs/src/tocdatareels.c"                                                \
  " dep/afs/src/tocfile.c"                                                     \
  " dep/afs/src/tocfilepreview.c"                                              \
  " dep/afs/src/tocfilepreviewpage.c"                                          \
  " dep/afs/src/tocfiles.c"                                                    \
  " dep/afs/src/tocmetadata.c"                                                 \
  " dep/afs/src/tocmetadatasource.c"                                           \
  " dep/afs/src/xmlutils.c"                                                    \
  ""

#define RPMALLOC_SOURCES                                                       \
  " dep/rpmalloc/rpmalloc/rpmalloc.c"                                          \
  ""

#define SYSTEM_WITH_LOG(cmd) (printf("\x1b[90m%s\x1b[0m\n", cmd), system(cmd))

#define RUN(exe) SYSTEM_WITH_LOG(exe)

#define DEFINES CC_DEFINES UNBOXING_DEFINES
#define INCLUDES UNBOXING_INCLUDES AFS_INCLUDES MXML_INCLUDES
#ifdef _WIN32
#define LFLAGS ""
#else
#define LFLAGS UNBOXING_LINK
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

int main(int argc, char *argv[]) {
  mkdir("out", 0755);
  mkdir("out/exe", 0755);
#ifndef RELEASE
  mkdir(".vscode", 0755);
  writeVSCodeInfo(INCLUDES, DEFINES);
#endif

  int cc_status;
  cc_status = COMPILE(CC, "", "", " dev/raw_file_to_png.c",
                      "out/exe/raw_file_to_png" BIN_EXT, CFLAGS, "");
  if (cc_status != 0)
    return cc_status;
#ifdef _WIN32
  RUN("mt.exe -nologo -manifest dev/build/unbox.manifest "
      "-outputresource:out/exe/raw_file_to_png.exe;#1");
#endif
  cc_status = COMPILE(CC, DEFINES, INCLUDES, SOURCES " src/main.c",
                      "out/exe/unbox" BIN_EXT, CFLAGS, LFLAGS);
#ifdef _WIN32
  RUN("del *.obj");
#endif
  if (cc_status != 0)
    return cc_status;
#ifdef _WIN32
  RUN("mt.exe -nologo -manifest dev/build/unbox.manifest "
      "-outputresource:out/exe/unbox.exe;#1");
#else
  RUN("date; ls -lAh --color=always out/exe");
#endif
  if (has_arg(argc, argv, "run") ||
#ifdef TEST
      true
#else
      false
#endif
  ) {
#ifdef TARGET_WINDOWS
    return RUN("wine out/exe/unbox" BIN_EXT
               " dep/ivm_testdata/reel/png out/data");
#else
    return RUN("./out/exe/unbox" BIN_EXT " dep/ivm_testdata/reel/png out/data");
#endif
  }
  return EXIT_SUCCESS;
}
