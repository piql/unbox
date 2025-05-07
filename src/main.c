#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../dep/stb/stb_image.h"

#include "../dep/afs/thirdparty/minixml/inc/mxml.h"
#include "../dep/unboxing/tests/testutils/src/config_source_4k_controlframe_v7.h"
#include "boxing/config.h"
#include "boxing/unboxer.h"

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

// TODO: fix XML pretty-printing
// static int mxml_whitespace_save_depth = 0;
// static char mxml_whitespace_save_text[256];
// static const char *mxml_whitespace_save(mxml_node_t *xml, int where) {
//   if (where == MXML_WS_BEFORE_OPEN) {
//     memset(mxml_whitespace_save_text, ' ', mxml_whitespace_save_depth * 2);
//     mxml_whitespace_save_text[mxml_whitespace_save_depth * 2] = '\0';
//     return mxml_whitespace_save_text;
//   } else if (where == MXML_WS_BEFORE_CLOSE) {
//     memset(mxml_whitespace_save_text, ' ', mxml_whitespace_save_depth * 2);
//     mxml_whitespace_save_text[mxml_whitespace_save_depth * 2] = '\0';
//     return mxml_whitespace_save_text;
//   } else if (where == MXML_WS_AFTER_OPEN)
//     mxml_whitespace_save_depth++;
//   else if (where == MXML_WS_AFTER_CLOSE)
//     mxml_whitespace_save_depth--;
//   return NULL;
// }

int main(void) {
  int status = EXIT_SUCCESS;
  boxing_config *config =
      boxing_config_create_from_structure(&config_source_v7);
  if (config) {
    boxing_unboxer_parameters params;
    boxing_unboxer_parameters_init(&params);
    params.format = config;
    boxing_unboxer *unboxer = boxing_unboxer_create(&params);
    if (unboxer) {
      gvector data = {
          .buffer = NULL,
          .size = 0,
          .item_size = 1,
          .element_free = NULL,
      };
      boxing_metadata_list *metadata = boxing_metadata_list_create();
      if (metadata) {
        int width;
        int height;
        unsigned char *img_data =
            stbi_load("dat/000001.png", &width, &height, NULL, 1);
        if (img_data) {
          printf("width %d, height %d, data %p\n", width, height, img_data);
          boxing_image8 image = {
              .width = (unsigned)width,
              .height = (unsigned)height,
              .is_owning_data = DFALSE,
              .data = img_data,
          };
          int extract_result = BOXING_UNBOXER_OK;
          enum boxing_unboxer_result decode_result = boxing_unboxer_unbox(
              &data, metadata, &image, unboxer, &extract_result, NULL,
              BOXING_METADATA_CONTENT_TYPES_CONTROLFRAME);
          printf("unbox: extract: %s, decode: %s\n",
                 boxing_unboxer_result_str[extract_result],
                 boxing_unboxer_result_str[decode_result]);
          if (extract_result == BOXING_UNBOXER_OK &&
              decode_result == BOXING_UNBOXER_OK) {
            // End char should be \n, set to '\0'
            ((char *)data.buffer)[data.size - 1] = '\0';
            printf("%.*s\n", data.size, data.buffer);
            mxml_node_t *xml =
                mxmlLoadString(NULL, data.buffer, MXML_NO_CALLBACK);
            if (xml) {
              const char *xml_str = mxmlSaveAllocString(xml, MXML_NO_CALLBACK);
              if (xml_str) {
                printf("%s\n", xml_str);
                free((void *)xml_str);
              }
              mxmlDelete(xml);
            } else {
              fprintf(stderr, "Failed to load xml\n");
              status = EXIT_FAILURE;
            }
            free(data.buffer);
          }
          free(img_data);
        } else {
          fprintf(stderr, "Failed to load image\n");
          status = EXIT_FAILURE;
        }
        boxing_metadata_list_free(metadata);
      } else {
        fprintf(stderr, "Failed to create metadata\n");
        status = EXIT_FAILURE;
      }
      boxing_unboxer_free(unboxer);
    } else {
      fprintf(stderr, "Failed to create unboxer\n");
      status = EXIT_FAILURE;
    }
    boxing_unboxer_parameters_free(&params);
    boxing_config_free(config);
  } else {
    fprintf(stderr, "Failed to create config from structure\n");
    status = EXIT_FAILURE;
  }
  return status;
}
