#include "reel.c"
#include "types.h"
#include "unboxing_log.c"
#include <boxing/config.h>
#include <boxing/math/crc64.h>
#include <boxing/unboxer.h>
#include <controldata.h>
#include <inttypes.h>
#include <mxml.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static bool writePathSegment(const char *const restrict input, Slice scratch,
                             unsigned *restrict cursor) {
  const size_t input_len = strlen(input);
  for (size_t i = 0; i < *cursor; i++)
    ((char *)scratch.data)[i] = input[i];
  size_t j;
  for (j = *cursor; j < input_len && j < scratch.size; j++) {
    if (input[j] == '/') {
      *cursor = j + 1;
      return true;
    }
    ((char *)scratch.data)[j] = input[j];
  }
  ((char *)scratch.data)[j] = '\0';
  return false;
}

void ensurePathExists(const char *const restrict path) {
  char buf[256] = {0};
  unsigned cursor = 0;
  for (;;) {
    if (writePathSegment(path, sliceof(buf), &cursor)) {
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

static bool writeEntireFile(const char *const restrict file_path, Slice data) {
  FILE *f = fopen(file_path, "w");
  size_t total_written = 0;
  while (total_written < data.size) {
    size_t written = fwrite((const char *)data.data + total_written, 1,
                            data.size - total_written, f);
    if (written == 0) {
      fclose(f);
      return false;
    }
    total_written += written;
  }
  fclose(f);
  return true;
}

#ifdef _WIN32
#include <windows.h>
#endif

static bool unboxAndOutputFiles(Reel *reel, Unboxer *unboxer,
                                Slice toc_contents,
                                const char *const restrict output_folder) {
  afs_toc_data *toc = afs_toc_data_create();
  if (!toc)
    return false;
  if (!afs_toc_data_load_string(toc, toc_contents.data)) {
    afs_toc_data_free(toc);
    return false;
  }
  afs_toc_data_reel *data_reel = afs_toc_data_reels_get_reel(toc->reels, 0);
  unsigned files = afs_toc_data_reel_file_count(data_reel);
  char buf[4096];
  for (unsigned i = 0; i < files; i++) {
    afs_toc_file *file = afs_toc_data_reel_get_file_by_index(data_reel, i);
    if (!(file->types & AFS_TOC_FILE_TYPE_DIGITAL)) {
      printf("skipping non-digital file: %s\n", file->name);
      continue;
    }
    printf("%d[%d]..%d[%d] (size: %" PRId64 ") %s (%s)\n", file->start_frame,
           file->start_byte, file->end_frame, file->end_byte, file->size,
           file->name, file->checksum);

    char output_file_path[4096];
    snprintf(output_file_path, sizeof output_file_path, "%s/%s", output_folder,
             file->name);
    ensurePathExists(output_file_path);
    FILE *output_file = fopen(output_file_path, "w+b");

    if (!output_file) {
      afs_toc_data_free(toc);
      return false;
    }

    size_t bytes_written = 0;
    size_t bytes_to_skip = file->start_byte;
    for (int f = file->start_frame; f <= file->end_frame; f++) {
      if (!reel->frames[f]) {
        afs_toc_data_free(toc);
        return false;
      }
      Slice frame_contents;
      sprintf(buf, "%s/%s", reel->directory_path,
              (const char *)reel->string_pool.data + reel->frames[f] - 1);
      Image data_frame = loadImage(buf);
      if (!data_frame.data) {
        afs_toc_data_free(toc);
        return false;
      }

      if (UnboxerUnbox(unboxer, data_frame.data, data_frame.width,
                       data_frame.height, BOXING_METADATA_CONTENT_TYPES_DATA,
                       &frame_contents) != UnboxOK) {
        afs_toc_data_free(toc);
        return false;
      }
      if (frame_contents.size) {
        size_t start = 0;
        if (bytes_to_skip)
          start = min(frame_contents.size, bytes_to_skip);

        if (start < frame_contents.size) {
          size_t bytes_to_write =
              min(frame_contents.size - start, file->size - bytes_written);
          fwrite((const char *)frame_contents.data + start, 1, bytes_to_write,
                 output_file);
          free(frame_contents.data);
          bytes_written += bytes_to_write;
        }
        bytes_to_skip -= start;
      }
    }
    fclose(output_file);
    boxing_unboxer_reset(unboxer->unboxer);
#ifndef _WIN32
    char path[4104];
    sprintf(path, "sha1sum %s", output_file_path);
    system(path);
#endif
  }
  afs_toc_data_free(toc);
  return true;
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
#endif
  if (argc < 3) {
    boxing_log_args(
        BoxingLogLevelError,
        "Usage: %s <input folder with scanned images> <output folder to "
        "place unboxed files>\n",
        argv[0]);
    return EXIT_FAILURE;
  }

  const char *input_folder = argv[1];
  const char *output_folder = argv[2];

  int status = EXIT_SUCCESS;
  dcrc64 *dcrc64 = boxing_math_crc64_create_def();
  if (!dcrc64) {
    boxing_log(BoxingLogLevelError, "Failed to create CRC64 instance");
    return EXIT_FAILURE;
  }
  Reel *reel = Reel_create();
  if (!reel) {
    boxing_math_crc64_free(dcrc64);
    boxing_log(BoxingLogLevelError, "Failed to create reel");
    return EXIT_FAILURE;
  }
  if (!Reel_init(reel, input_folder)) {
    Reel_destroy(reel);
    boxing_math_crc64_free(dcrc64);
    boxing_log(
        BoxingLogLevelError,
        "Failed to init reel (Maybe no frames found in the current folder?)");
    return EXIT_FAILURE;
  }
  bool use_raw_decoding;
  Slice control_frame_contents =
      Reel_unbox_control_frame(reel, &use_raw_decoding);
  if (control_frame_contents.data) {
    printf("%.*s\n", (int)control_frame_contents.size,
           (char *)control_frame_contents.data);
    afs_control_data *ctl = afs_control_data_create();
    if (afs_control_data_load_string(
            ctl, (const char *)control_frame_contents.data)) {
      printReelInformation(ctl->administrative_metadata);
      Unboxer unboxer;
      if (UnboxerCreate(
              ctl->technical_metadata->afs_content_boxing_format->config,
              use_raw_decoding, &unboxer) == UnboxerInitOK) {
        Slice toc_contents;
        bool toc_contents_cached = false;
        uint64_t crc = boxing_math_crc64_calc_crc(
            dcrc64, control_frame_contents.data, control_frame_contents.size);
        boxing_math_crc64_reset(dcrc64, POLY_CRC_64);
        mkdir(output_folder, 0755);
        char cachefile_path[4096];
        snprintf(cachefile_path, sizeof(cachefile_path),
                 "%s/toc_%" PRIx64 ".xml", output_folder, crc);
        printf("checking for: %s\n", cachefile_path);
        Slice cached_toc_contents = mapFile(cachefile_path);
        if (cached_toc_contents.data) {
          toc_contents = cached_toc_contents;
          toc_contents_cached = true;
        } else {
          if (afs_toc_files_get_tocs_count(ctl->technical_metadata->afs_tocs) >
              0) {
            afs_toc_file *toc_file =
                afs_toc_files_get_toc(ctl->technical_metadata->afs_tocs, 0);
            toc_contents = Reel_unbox_toc(reel, &unboxer, toc_file);
            // ignore failing to write cache
            if (toc_contents.data)
              writeEntireFile(cachefile_path, toc_contents);
          } else {
            boxing_log(BoxingLogLevelError,
                       "No TOCs found in control frame data");
            status = EXIT_FAILURE;
          }
        }
        if (toc_contents.data) {
          if (!unboxAndOutputFiles(reel, &unboxer, toc_contents,
                                   output_folder)) {
            boxing_log(BoxingLogLevelError, "Failed to unbox / output files");
            status = EXIT_FAILURE;
          }
          if (toc_contents_cached)
            unmapFile(toc_contents);
          else
            free(toc_contents.data);
        } else {
          boxing_log(BoxingLogLevelError, "Failed to unbox TOC");
          status = EXIT_FAILURE;
        }
        UnboxerDestroy(&unboxer);
      } else {
        boxing_log(BoxingLogLevelError, "Failed to create unboxer");
        status = EXIT_FAILURE;
      }
    } else {
      boxing_log(BoxingLogLevelError, "Failed to load control data");
      status = EXIT_FAILURE;
    }
    afs_control_data_free(ctl);
    free(control_frame_contents.data);
  } else {
    boxing_log(BoxingLogLevelError, "Failed to unbox control frame");
    status = EXIT_FAILURE;
  }
  Reel_destroy(reel);
  boxing_math_crc64_free(dcrc64);
  printf("\x1b[%dm%s\x1b[0m\n", status ? 91 : 92, status ? "FAILED" : "OK");
  return status;
}
