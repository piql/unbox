#include "load_image.c"
#include "unboxing_log.c"
#include <boxing/config.h>
#include <boxing/unboxer.h>
#include <controldata.h>
#include <mxml.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <tocdata_c.h>

#include "../dep/unboxing/tests/testutils/src/config_source_4k_controlframe_v7.h"

static const char *const boxing_unboxer_result_str[] = {
    "OK",
    "METADATA_ERROR",
    "BORDER_TRACKING_ERROR",
    "DATA_DECODE_ERROR",
    "CRC_MISMATCH_ERROR",
    "CONFIG_ERROR",
    "PROCESS_CALLBACK_ABORT",
    "INPUT_DATA_ERROR",
    "SPLICING",
};

typedef struct {
  boxing_unboxer_parameters parameters;
  boxing_unboxer *unboxer;
  boxing_metadata_list *metadata;
} Unboxer;

enum UnboxerInitStatus { UnboxerInitOK, UnboxerInitFailed };
static enum UnboxerInitStatus UnboxerCreate(boxing_config *config, bool is_raw,
                                            Unboxer *out) {
  boxing_unboxer_parameters parameters;
  boxing_unboxer_parameters_init(&parameters);
  parameters.is_raw = is_raw;
  if (parameters.pre_filter.coeff) {
    parameters.format = config;
    boxing_unboxer *unboxer = boxing_unboxer_create(&parameters);
    if (unboxer) {
      boxing_metadata_list *metadata = boxing_metadata_list_create();
      if (metadata) {
        *out = (Unboxer){
            .parameters = parameters,
            .unboxer = unboxer,
            .metadata = metadata,
        };
        return UnboxerInitOK;
      }
      boxing_unboxer_free(unboxer);
    }
    boxing_unboxer_parameters_free(&parameters);
  }
  return UnboxerInitFailed;
}

static void UnboxerDestroy(Unboxer *unboxer) {
  boxing_metadata_list_free(unboxer->metadata);
  boxing_unboxer_free(unboxer->unboxer);
  boxing_unboxer_parameters_free(&unboxer->parameters);
}

enum UnboxerUnboxStatus { UnboxOK, UnboxFailed };
static enum UnboxerUnboxStatus
UnboxerUnbox(Unboxer *unboxer, uint8_t *image_data, uint32_t width,
             uint32_t height,
             boxing_metadata_content_types fallback_metadata_content_type,
             char **result, size_t *result_len) {
  boxing_log_args(BoxingLogLevelAlways, "width %d, height %d, data %p", width,
                  height, image_data);
  boxing_image8 image = {
      .width = (unsigned)width,
      .height = (unsigned)height,
      .is_owning_data = DFALSE,
      .data = image_data,
  };
  int extract_result = BOXING_UNBOXER_OK;
  gvector data = {
      .buffer = NULL,
      .size = 0,
      .item_size = 1,
      .element_free = NULL,
  };
  enum boxing_unboxer_result decode_result = boxing_unboxer_unbox(
      &data, unboxer->metadata, &image, unboxer->unboxer, &extract_result, NULL,
      fallback_metadata_content_type);
  boxing_log_args(BoxingLogLevelAlways, "unbox: extract: %s, decode: %s",
                  boxing_unboxer_result_str[extract_result],
                  boxing_unboxer_result_str[decode_result]);
  if (extract_result == BOXING_UNBOXER_OK &&
      decode_result == BOXING_UNBOXER_OK) {
    // End char should be \n, set to '\0'
    ((char *)data.buffer)[data.size - 1] = '\0';
    *result = data.buffer;
    *result_len = data.size - 1;
    return UnboxOK;
  }
  if (data.buffer)
    free(data.buffer);
  return UnboxFailed;
}

static bool writePathSegment(const char *const restrict input, char *scratch,
                             const size_t scratch_len,
                             unsigned *restrict cursor) {
  const size_t input_len = strlen(input);
  for (size_t i = 0; i < *cursor; i++)
    scratch[i] = input[i];
  size_t j;
  for (j = *cursor; j < input_len && j < scratch_len; j++) {
    if (input[j] == '/') {
      *cursor = j + 1;
      return true;
    }
    scratch[j] = input[j];
  }
  scratch[j] = '\0';
  return false;
}

void ensurePathExists(const char *const restrict path) {
  char buf[256] = {0};
  unsigned cursor = 0;
  for (;;) {
    if (writePathSegment(path, buf, sizeof(buf), &cursor)) {
#ifdef _WIN32
      mkdir(buf);
#else
      mkdir(buf, 0755);
#endif
    } else
      break;
  }
}

static void printReelInformation(afs_administrative_metadata *md) {
  boxing_log(BoxingLogLevelAlways, "");
  boxing_log_args(BoxingLogLevelAlways, "Reel ID: %s", md->reel_id);
  boxing_log_args(BoxingLogLevelAlways, "Print Reel ID: %s", md->print_reel_id);
  boxing_log_args(BoxingLogLevelAlways, "Title: %s", md->title);
  boxing_log_args(BoxingLogLevelAlways, "Description: %s", md->description);
  boxing_log_args(BoxingLogLevelAlways, "Creator: %s", md->creator);
  boxing_log_args(BoxingLogLevelAlways, "Creation Date: %s", md->creation_date);
  boxing_log(BoxingLogLevelAlways, "");
}

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[]) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
#endif
  int status = EXIT_SUCCESS;
  Image control_frame =
      loadImage(argc > 1 ? argv[1] : "dep/ivm_testdata/reel/png/000001.png");
  if (control_frame.data == NULL) {
    boxing_log(BoxingLogLevelError, "Failed to read control frame");
    return EXIT_FAILURE;
  }
  boxing_config *config =
      boxing_config_create_from_structure(&config_source_v7);
  if (config) {
    Unboxer unboxer;
    if (UnboxerCreate(
            config, control_frame.width == 4096 && control_frame.height == 2160,
            &unboxer) == UnboxerInitOK) {
      char *data = NULL;
      size_t len = 0;
      if (UnboxerUnbox(&unboxer, control_frame.data, control_frame.width,
                       control_frame.height,
                       BOXING_METADATA_CONTENT_TYPES_CONTROLFRAME, &data,
                       &len) == UnboxOK) {
        afs_control_data *ctl = afs_control_data_create();
        if (afs_control_data_load_string(ctl, data)) {
          printReelInformation(ctl->administrative_metadata);
          Image frame = loadImage("dep/ivm_testdata/reel/png/000556.png");
          if (frame.data) {
            Unboxer data_unboxer;
            if (UnboxerCreate(
                    ctl->technical_metadata->afs_content_boxing_format->config,
                    frame.width == 4096 && frame.height == 2160,
                    &data_unboxer) == UnboxerInitOK) {
              char *data = NULL;
              size_t len = 0;
              if (UnboxerUnbox(&data_unboxer, frame.data, frame.width,
                               frame.height, BOXING_METADATA_CONTENT_TYPES_TOC,
                               &data, &len) == UnboxOK) {
                afs_toc_data *toc = afs_toc_data_create();
                if (afs_toc_data_load_string(toc, data)) {
                  afs_toc_data_reel *reel =
                      afs_toc_data_reels_get_reel(toc->reels, 0);
                  unsigned files = afs_toc_data_reel_file_count(reel);
                  for (unsigned i = 0; i < files; i++) {
                    afs_toc_file *file =
                        afs_toc_data_reel_get_file_by_index(reel, i);
                    printf("%d[%d]..%d[%d] %s (%s)\n", file->start_frame,
                           file->start_byte, file->end_frame, file->end_byte,
                           file->name, file->checksum);
                    Image data_frame =
                        loadImage("dep/ivm_testdata/reel/png/000557.png");
                    if (data_frame.data) {
                      char *data = NULL;
                      size_t len = 0;
                      if (UnboxerUnbox(&data_unboxer, data_frame.data,
                                       data_frame.width, data_frame.height,
                                       BOXING_METADATA_CONTENT_TYPES_DATA,
                                       &data, &len) == UnboxOK) {
                        ensurePathExists(file->name);
                        FILE *output_file = fopen(file->name, "w+b");
                        if (output_file) {
                          fwrite(data + file->start_byte, 1,
                                 file->end_byte + 1 - file->start_byte,
                                 output_file);
                          fclose(output_file);
#ifndef _WIN32
                          char path[4096];
                          sprintf(path, "sha1sum %s", file->name);
                          system(path);
#endif
                        } else {
                          boxing_log(BoxingLogLevelError,
                                     "Failed to open output file");
                          status = EXIT_FAILURE;
                        }
                        free(data);
                      } else {
                        boxing_log(BoxingLogLevelError,
                                   "Failed to unbox data frame");
                        status = EXIT_FAILURE;
                      }
                    } else {
                      boxing_log(BoxingLogLevelError,
                                 "Failed to load data frame");
                      status = EXIT_FAILURE;
                    }
                    if (status == EXIT_FAILURE)
                      break;
                  }
                  afs_toc_data_free(toc);
                } else {
                  boxing_log(BoxingLogLevelError, "Failed to parse toc");
                  status = EXIT_FAILURE;
                }
                free(data);
              } else {
                boxing_log(BoxingLogLevelError, "Failed to unbox toc");
                status = EXIT_FAILURE;
              }
              UnboxerDestroy(&data_unboxer);
            } else {
              boxing_log(BoxingLogLevelError,
                         "Failed to initialize data_unboxer");
              status = EXIT_FAILURE;
            }
          } else {
            boxing_log(BoxingLogLevelError, "Failed to load frame image");
            status = EXIT_FAILURE;
          }
          afs_control_data_free(ctl);
        } else {
          boxing_log(BoxingLogLevelError, "Failed to load afs control data");
          status = EXIT_FAILURE;
        }
        free(data);
      } else {
        boxing_log(BoxingLogLevelError, "Failed to unbox");
        status = EXIT_FAILURE;
      }
      UnboxerDestroy(&unboxer);
    } else {
      boxing_log(BoxingLogLevelError, "Failed to initialize unboxer");
      status = EXIT_FAILURE;
    }
    boxing_config_free(config);
  } else {
    boxing_log(BoxingLogLevelError, "Failed to load config_structure");
    status = EXIT_FAILURE;
  }
  return status;
}
