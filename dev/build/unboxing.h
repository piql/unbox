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
  " -Idep/afs/unboxing/inc"                                                    \
  " -Idep/afs/unboxing/src/unboxer"                                            \
  " -Idep/afs/unboxing/thirdparty/bch"                                         \
  " -Idep/afs/unboxing/thirdparty/glib"                                        \
  " -Idep/afs/unboxing/thirdparty/LDPC-codes"                                  \
  " -Idep/afs/unboxing/thirdparty/reedsolomon"                                 \
  ""

#define UNBOXING_SOURCES                                                       \
  " dep/afs/unboxing/src/base/math_crc32.c"                                    \
  " dep/afs/unboxing/src/base/math_crc64.c"                                    \
  " dep/afs/unboxing/src/codecs/2dpam.c"                                       \
  " dep/afs/unboxing/src/codecs/bchcodec.c"                                    \
  " dep/afs/unboxing/src/codecs/cipher.c"                                      \
  " dep/afs/unboxing/src/codecs/codecbase.c"                                   \
  " dep/afs/unboxing/src/codecs/codecdispatcher.c"                             \
  " dep/afs/unboxing/src/codecs/crc32.c"                                       \
  " dep/afs/unboxing/src/codecs/crc64.c"                                       \
  " dep/afs/unboxing/src/codecs/ftfinterleaving.c"                             \
  " dep/afs/unboxing/src/codecs/interleaving.c"                                \
  " dep/afs/unboxing/src/codecs/ldpccodec.c"                                   \
  " dep/afs/unboxing/src/codecs/modulator.c"                                   \
  " dep/afs/unboxing/src/codecs/packetheader.c"                                \
  " dep/afs/unboxing/src/codecs/reedsolomon.c"                                 \
  " dep/afs/unboxing/src/codecs/symbolconverter.c"                             \
  " dep/afs/unboxing/src/codecs/syncpointinserter.c"                           \
  " dep/afs/unboxing/src/config.c"                                             \
  " dep/afs/unboxing/src/frame/2polinomialsampler.c"                           \
  " dep/afs/unboxing/src/frame/areasampler.c"                                  \
  " dep/afs/unboxing/src/frame/sampler.c"                                      \
  " dep/afs/unboxing/src/frame/trackergpf_1.c"                                 \
  " dep/afs/unboxing/src/frame/trackergpf.c"                                   \
  " dep/afs/unboxing/src/globals.c"                                            \
  " dep/afs/unboxing/src/graphics/border.c"                                    \
  " dep/afs/unboxing/src/graphics/calibrationbar.c"                            \
  " dep/afs/unboxing/src/graphics/component.c"                                 \
  " dep/afs/unboxing/src/graphics/contentcontainer.c"                          \
  " dep/afs/unboxing/src/graphics/genericframe.c"                              \
  " dep/afs/unboxing/src/graphics/genericframefactory.c"                       \
  " dep/afs/unboxing/src/graphics/genericframegpf_1.c"                         \
  " dep/afs/unboxing/src/graphics/label.c"                                     \
  " dep/afs/unboxing/src/graphics/metadatabar.c"                               \
  " dep/afs/unboxing/src/graphics/painter.c"                                   \
  " dep/afs/unboxing/src/graphics/referencebar.c"                              \
  " dep/afs/unboxing/src/graphics/referencepoint.c"                            \
  " dep/afs/unboxing/src/image8.c"                                             \
  " dep/afs/unboxing/src/log.c"                                                \
  " dep/afs/unboxing/src/math/bitutils.c"                                      \
  " dep/afs/unboxing/src/math/dsp.c"                                           \
  " dep/afs/unboxing/src/math/math.c"                                          \
  " dep/afs/unboxing/src/matrix.c"                                             \
  " dep/afs/unboxing/src/metadata.c"                                           \
  " dep/afs/unboxing/src/string.c"                                             \
  " dep/afs/unboxing/src/unboxer.c"                                            \
  " dep/afs/unboxing/src/unboxer/cornermark.c"                                 \
  " dep/afs/unboxing/src/unboxer/datapoints.c"                                 \
  " dep/afs/unboxing/src/unboxer/filter.c"                                     \
  " dep/afs/unboxing/src/unboxer/frametrackerutil.c"                           \
  " dep/afs/unboxing/src/unboxer/frameutil.c"                                  \
  " dep/afs/unboxing/src/unboxer/histogramutils.c"                             \
  " dep/afs/unboxing/src/unboxer/horizontalmeasures.c"                         \
  " dep/afs/unboxing/src/unboxer/sampleutil.c"                                 \
  " dep/afs/unboxing/src/unboxer/syncpoints.c"                                 \
  " dep/afs/unboxing/src/unboxer/unboxerv1.c"                                  \
  " dep/afs/unboxing/src/utils.c"                                              \
  " dep/afs/unboxing/thirdparty/bch/bch.c"                                     \
  " dep/afs/unboxing/thirdparty/glib/g_variant.c"                              \
  " dep/afs/unboxing/thirdparty/glib/ghash.c"                                  \
  " dep/afs/unboxing/thirdparty/glib/glist.c"                                  \
  " dep/afs/unboxing/thirdparty/glib/gvector.c"                                \
  " dep/afs/unboxing/thirdparty/LDPC-codes/alloc.c"                            \
  " dep/afs/unboxing/thirdparty/LDPC-codes/check.c"                            \
  " dep/afs/unboxing/thirdparty/LDPC-codes/dec.c"                              \
  " dep/afs/unboxing/thirdparty/LDPC-codes/distrib.c"                          \
  " dep/afs/unboxing/thirdparty/LDPC-codes/enc.c"                              \
  " dep/afs/unboxing/thirdparty/LDPC-codes/intio.c"                            \
  " dep/afs/unboxing/thirdparty/LDPC-codes/mod2convert.c"                      \
  " dep/afs/unboxing/thirdparty/LDPC-codes/mod2dense.c"                        \
  " dep/afs/unboxing/thirdparty/LDPC-codes/mod2sparse.c"                       \
  " dep/afs/unboxing/thirdparty/LDPC-codes/open.c"                             \
  " dep/afs/unboxing/thirdparty/LDPC-codes/rand.c"                             \
  " dep/afs/unboxing/thirdparty/LDPC-codes/rcode.c"                            \
  " dep/afs/unboxing/thirdparty/reedsolomon/galois_field.c"                    \
  " dep/afs/unboxing/thirdparty/reedsolomon/rs.c"                              \
  ""
