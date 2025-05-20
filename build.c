#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "dev/build/util.c"

#if 1
#define CC "gcc"
#define DEFINES ""
#define CFLAGS " -g -fsanitize=address"
#elif 1
#define CC "tcc"
#define DEFINES " -DSTBI_NO_SIMD"
#define CFLAGS " -I$(dirname $(which tcc))/include -L$(dirname $(which tcc))"
#else
#define CC "zig cc"
#define DEFINES ""
#define TARGET_WINDOWS
#define CFLAGS " -g -target x86_64-windows"
#endif

#if 0
#define MXML_INCLUDES " -Idep/mxml"
#define MXML_SOURCES                                                           \
  " dep/mxml/mxml-attr.c"                                                      \
  " dep/mxml/mxml-file.c"                                                      \
  " dep/mxml/mxml-get.c"                                                       \
  " dep/mxml/mxml-node.c"                                                      \
  " dep/mxml/mxml-options.c"                                                   \
  " dep/mxml/mxml-private.c"                                                   \
  " dep/mxml/mxml-search.c"                                                    \
  ""
#else
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
#endif

#define UNBOXING_DEFINES " -D_DEBUG -DRAND_FILE='\"randfile\"'"

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

#define SYSTEM_WITH_LOG(cmd)                                                   \
  printf("\x1b[90m%s\x1b[0m\n", cmd);                                          \
  system(cmd)

#define COMPILE(sources, output)                                               \
  SYSTEM_WITH_LOG(                                                             \
      CC DEFINES UNBOXING_DEFINES UNBOXING_INCLUDES UNBOXING_SOURCES           \
          UNBOXING_LINK MXML_INCLUDES MXML_SOURCES " " sources CFLAGS          \
                                                   " -o " output)

#define RUN(exe) SYSTEM_WITH_LOG(exe)

int main(int argc, char *argv[]) {
  mkdir("out", 0755);
  mkdir("out/exe", 0755);
#ifdef TARGET_WINDOWS
  SYSTEM_WITH_LOG("cp dep/mxml/vcnet/config.h dep/mxml/config.h");
#else
  SYSTEM_WITH_LOG("cd dep/mxml; ./configure");
#endif
  writeVSCodeInfo(MXML_INCLUDES UNBOXING_INCLUDES, DEFINES UNBOXING_DEFINES);
  COMPILE("src/main.c", "out/exe/unbox");
#ifdef TARGET_WINDOWS
  RUN("wine out/exe/unbox");
#else
  char run_cmd[4096];
  if (argc > 1)
    snprintf(run_cmd, 4096, "%s %s", "./out/exe/unbox", argv[1]);
  else
    snprintf(run_cmd, 4096, "%s", "./out/exe/unbox");
  RUN(run_cmd);
#endif
  return EXIT_SUCCESS;
}
