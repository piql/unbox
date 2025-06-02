#include "types.h"
#include "unboxing_log.c"
#include <boxing/unboxer.h>
#include <inttypes.h>
#include <stdbool.h>

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

static const char *const boxing_metadata_type_str[] = {
    "ENDOFDATA",   "JOBID",     "FRAMENUMBER",       "FILEID",
    "FILESIZE",    "DATACRC",   "DATASIZE",          "SYMBOLSPERPIXEL",
    "CONTENTTYPE", "CIPHERKEY", "CONTENTSYMBOLSIZE", "LASTTYPE",
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
             Slice *result) {
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
  GHashTableIter it;
  g_hash_table_iter_init(&it, unboxer->metadata);
  void *k;
  void *v;
  char metadata_buf[4096];
  size_t off = 0;
  while (g_hash_table_iter_next(&it, &k, &v)) {
    boxing_metadata_type type = (boxing_metadata_type) * (uint16_t *)k;
    boxing_metadata_item *item = (boxing_metadata_item *)v;
    off += snprintf(metadata_buf + off, sizeof(metadata_buf) - off,
                    "%s%s: ", off ? "\x1b[0m, " : "Metadata: ",
                    boxing_metadata_type_str[type]);
    switch (type) {
    case BOXING_METADATA_TYPE_FRAMENUMBER:
      off += snprintf(metadata_buf + off, sizeof(metadata_buf) - off,
                      "\x1b[92m%u", ((boxing_metadata_item_u32 *)item)->value);
      break;
    case BOXING_METADATA_TYPE_DATACRC:
      off += snprintf(metadata_buf + off, sizeof(metadata_buf) - off,
                      "\x1b[93m%" PRIx64,
                      ((boxing_metadata_item_u64 *)item)->value);
      break;
    case BOXING_METADATA_TYPE_DATASIZE:
      off += snprintf(metadata_buf + off, sizeof(metadata_buf) - off,
                      "\x1b[92m%u", ((boxing_metadata_item_u32 *)item)->value);
      break;
    case BOXING_METADATA_TYPE_SYMBOLSPERPIXEL:
      off += snprintf(metadata_buf + off, sizeof(metadata_buf) - off,
                      "\x1b[92m%u", ((boxing_metadata_item_u16 *)item)->value);
      break;
    case BOXING_METADATA_TYPE_CONTENTTYPE:
      off +=
          snprintf(metadata_buf + off, sizeof(metadata_buf) - off, "\x1b[96m%s",
                   BOXING_METADATA_CONTENT_TYPE_STR(
                       ((boxing_metadata_item_u16 *)item)->value));
      break;
    case BOXING_METADATA_TYPE_CIPHERKEY:
      off += snprintf(metadata_buf + off, sizeof(metadata_buf) - off,
                      "\x1b[93m%x", ((boxing_metadata_item_u32 *)item)->value);
      break;
    case BOXING_METADATA_TYPE_CONTENTSYMBOLSIZE:
      off += snprintf(metadata_buf + off, sizeof(metadata_buf) - off,
                      "\x1b[92m%u", ((boxing_metadata_item_u16 *)item)->value);
      break;
    default:
      off += snprintf(metadata_buf + off, sizeof(metadata_buf) - off,
                      "\x1b[33m(TODO: format this type, size: %d)",
                      item->base.size);
    }
  }
  boxing_log_args(BoxingLogLevelAlways, "%.*s\x1b[0m", (int)off, metadata_buf);
  boxing_log_args(BoxingLogLevelAlways,
                  "unbox: extract: %s%s\x1b[0m, decode: %s%s\x1b[0m",
                  extract_result == BOXING_UNBOXER_OK         ? "\x1b[92m"
                  : extract_result == BOXING_UNBOXER_SPLICING ? "\x1b[93m"
                                                              : "\x1b[91m",
                  boxing_unboxer_result_str[extract_result],
                  decode_result == BOXING_UNBOXER_OK         ? "\x1b[92m"
                  : decode_result == BOXING_UNBOXER_SPLICING ? "\x1b[93m"
                                                             : "\x1b[91m",
                  boxing_unboxer_result_str[decode_result]);
  if (extract_result == BOXING_UNBOXER_OK &&
      decode_result == BOXING_UNBOXER_OK) {
    // End char should be \n, set to '\0'
    if (data.buffer && data.size) {
      ((char *)data.buffer)[data.size - 1] = '\0';
      *result = (Slice){.data = data.buffer, .size = data.size - 1};
    } else {
      *result = (Slice){.data = NULL, .size = 0};
    }
    return UnboxOK;
  }
  if (data.buffer)
    free(data.buffer);
  return UnboxFailed;
}
