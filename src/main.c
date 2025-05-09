#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../dep/stb/stb_image.h"

#include "../dep/unboxing/tests/testutils/src/config_source_4k_controlframe_v7.h"
#include <boxing/config.h>
#include <boxing/unboxer.h>
#include <mxml.h>

#include "unboxing_log.c"

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
  boxing_config *config;
  boxing_unboxer_parameters parameters;
  boxing_unboxer *unboxer;
  boxing_metadata_list *metadata;
} Unboxer;

enum UnboxerInitStatus { UnboxerInitOK, UnboxerInitFailed };
static enum UnboxerInitStatus UnboxerCreate(Unboxer *out) {
  enum UnboxerInitStatus status = UnboxerInitOK;
  boxing_config *config =
      boxing_config_create_from_structure(&config_source_v7);
  if (config) {
    boxing_unboxer_parameters parameters;
    boxing_unboxer_parameters_init(&parameters);
    if (parameters.pre_filter.coeff) {
      parameters.format = config;
      boxing_unboxer *unboxer = boxing_unboxer_create(&parameters);
      if (unboxer) {
        boxing_metadata_list *metadata = boxing_metadata_list_create();
        if (metadata) {
          *out = (Unboxer){
              .config = config,
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
    boxing_config_free(config);
  }
  return UnboxerInitFailed;
}

static void UnboxerDestroy(Unboxer *unboxer) {
  boxing_metadata_list_free(unboxer->metadata);
  boxing_unboxer_free(unboxer->unboxer);
  boxing_unboxer_parameters_free(&unboxer->parameters);
  boxing_config_free(unboxer->config);
}

enum UnboxerUnboxStatus { UnboxOK, UnboxFailed };
static enum UnboxerUnboxStatus UnboxerUnbox(Unboxer *unboxer,
                                            const char *image_path,
                                            char **result, size_t *result_len) {
  int width;
  int height;
  unsigned char *img_data = stbi_load(image_path, &width, &height, NULL, 1);
  if (img_data) {
    boxing_log_args(BoxingLogLevelAlways, "width %d, height %d, data %p", width,
                    height, img_data);
    boxing_image8 image = {
        .width = (unsigned)width,
        .height = (unsigned)height,
        .is_owning_data = DFALSE,
        .data = img_data,
    };
    int extract_result = BOXING_UNBOXER_OK;
    gvector data = {
        .buffer = NULL,
        .size = 0,
        .item_size = 1,
        .element_free = NULL,
    };
    enum boxing_unboxer_result decode_result = boxing_unboxer_unbox(
        &data, unboxer->metadata, &image, unboxer->unboxer, &extract_result,
        NULL, BOXING_METADATA_CONTENT_TYPES_CONTROLFRAME);
    free(img_data);
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
  }
  return UnboxFailed;
}

static void log_mxml_err(void *cbdata, const char *message) {
  boxing_log(BoxingLogLevelError, message);
}

static const char *getElementText(mxml_node_t *top, const char *element_name) {
  mxml_node_t *element =
      mxmlFindElement(top, top, element_name, NULL, NULL, MXML_DESCEND_ALL);
  return element ? mxmlGetOpaque(element) : NULL;
}

int main(void) {
  int status = EXIT_SUCCESS;
  Unboxer unboxer;
  if (UnboxerCreate(&unboxer) == UnboxerInitOK) {
    char *data = NULL;
    size_t len = 0;
    if (UnboxerUnbox(&unboxer, "dat/000001.png", &data, &len) == UnboxOK) {
      mxml_options_t *options = mxmlOptionsNew();
      if (options) {
        mxmlOptionsSetTypeValue(options, MXML_TYPE_OPAQUE);
        mxmlOptionsSetErrorCallback(options, log_mxml_err, NULL);
        mxml_node_t *xml = mxmlLoadString(NULL, options, data);
        if (xml) {
          boxing_log(BoxingLogLevelAlways, "");
          boxing_log_args(BoxingLogLevelAlways, "Reel ID: %s",
                          getElementText(xml, "ReelId"));
          boxing_log_args(BoxingLogLevelAlways, "Print Reel ID: %s",
                          getElementText(xml, "PrintReelId"));
          boxing_log_args(BoxingLogLevelAlways, "Title: %s",
                          getElementText(xml, "Title"));
          boxing_log_args(BoxingLogLevelAlways, "Description: %s",
                          getElementText(xml, "Description"));
          boxing_log_args(BoxingLogLevelAlways, "Creator: %s",
                          getElementText(xml, "Creator"));
          boxing_log_args(BoxingLogLevelAlways, "CreationDate: %s",
                          getElementText(xml, "CreationDate"));
          mxmlDelete(xml);
        } else {
          boxing_log(BoxingLogLevelError, "Failed to load xml");
          status = EXIT_FAILURE;
        }
        mxmlOptionsDelete(options);
      } else {
        boxing_log(BoxingLogLevelError, "Failed to allocate mxml options");
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
  return status;
}
