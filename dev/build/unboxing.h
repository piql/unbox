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

#ifdef _WIN32
#define UNBOXING_LFLAGS ""
#else
#define UNBOXING_LFLAGS " -lm"
#endif

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
