#include "../dep/afs/unboxing/inc/boxing/math/crc64.h"
#include "../src/grow.c"
#include "../src/map_file.c"
#include "../src/unboxing_log.c"
#ifdef _WIN32
#include "../src/win32.h"
#define BYTE_ORDER 1234
#define LITTLE_ENDIAN 1234
#elif defined(__APPLE__)
#else
#include <endian.h>
#endif
#include <inttypes.h>
#ifndef _WIN32
#include <pthread.h>
#endif
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define THREADED
#ifdef THREADED
#define THREAD_COUNT 6
#endif

#include "raw_file.c"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../dep/stb/stb_image_write.h"

static bool generateFrameRaw(const char *const restrict output_path,
                             const RawFileHeader *const restrict header_data,
                             const uint8_t *const restrict data,
                             const size_t data_size,
                             const RawFileFooter *const restrict footer_data) {
#ifdef _WIN32
  uint32_t attr = GetFileAttributesA(output_path);
  if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
    return true;
#else
  struct stat s;
  if (stat(output_path, &s) == 0)
    return true;
#endif
  FILE *fd = fopen(output_path, "w");
  if (!fd)
    return false;
  if (fwrite(header_data, sizeof *header_data, 1, fd) != 1) {
    fprintf(stderr, "%s: failed to write header data\n", output_path);
    fclose(fd);
    return false;
  };
  if (fwrite(data, 1, data_size, fd) != data_size) {
    fprintf(stderr, "%s: failed to write frame data\n", output_path);
    fclose(fd);
    return false;
  };
  if (fwrite(footer_data, sizeof *footer_data, 1, fd) != 1) {
    fprintf(stderr, "%s: failed to write footer data\n", output_path);
    fclose(fd);
    return false;
  };
  fclose(fd);
  return true;
}

static bool generateFramePng(const char *const restrict output_path,
                             const uint8_t *const restrict data,
                             const uint32_t width, const uint32_t height,
                             const uint8_t color_depth,
                             Slice *const output_image) {
#ifdef _WIN32
  uint32_t attr = GetFileAttributesA(output_path);
  if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
    return true;
#else
  struct stat s;
  if (stat(output_path, &s) == 0)
    return true;
#endif

  if (color_depth == 8)
    return stbi_write_png(output_path, width, height, 1, data, width) != 0;

  if (!splat_pixels(data, width, height, color_depth, output_image))
    return false;

  return stbi_write_png(output_path, width, height, 1, output_image->data,
                        width) != 0;
}

typedef struct {
  char output_path[256];
  uint16_t frame_id;
  const RawFileHeader *header_data;
  const uint8_t *frame_data;
  const RawFileFooter *footer_data;
  uint32_t width;
  uint32_t height;
  uint8_t color_depth;
} FrameGenerationJob;

typedef struct {
  enum JobKind {
    JOB_GENERATE_PNG,
    JOB_GENERATE_SINGLE,
    JOB_GENERATE_READ,
    JOB_GENERATE_VALIDATE,
  } job;
  Slice jobs;
  uint16_t count;
  uint16_t offset;
  bool done;
#ifdef THREADED
#ifdef _WIN32
  void *mutex;
#else
  pthread_mutex_t mutex;
#endif
#endif
} PendingFrameGenerationJobList;

#ifdef _WIN32
uint32_t WINAPI frameGeneratorWorker(PendingFrameGenerationJobList *job_list) {
#else
void *frameGeneratorWorker(PendingFrameGenerationJobList *job_list) {
#endif
  // Reasonable initial guess for size
  Slice output_image = {
      .data = malloc(4096 * 2160),
      .size = 4096 * 2160,
  };

  for (;;) {
#ifdef THREADED
#ifdef _WIN32
    if (WaitForSingleObject(job_list->mutex, 0) == WAIT_OBJECT_0) {
#else
    if (pthread_mutex_trylock(&job_list->mutex) == 0) {
#endif
#endif
      if (job_list->offset >= job_list->count) {
        bool done = job_list->done;
#ifdef THREADED
#ifdef _WIN32
        ReleaseMutex(job_list->mutex);
#else
        pthread_mutex_unlock(&job_list->mutex);
#endif
#endif
        if (done) {
          free(output_image.data);
#ifdef _WIN32
          return 0;
#else
        return NULL;
#endif
        } else
          continue;
      }

      enum JobKind activeJob = job_list->job;

      FrameGenerationJob jobs[16];
      uint16_t jobs_left = job_list->count - job_list->offset;
      uint8_t count = min(countof(jobs), jobs_left);
      memcpy(jobs, (FrameGenerationJob *)job_list->jobs.data + job_list->offset,
             sizeof *jobs * count);
      job_list->offset += count;
#ifdef THREADED
#ifdef _WIN32
      ReleaseMutex(job_list->mutex);
#else
      pthread_mutex_unlock(&job_list->mutex);
#endif
#endif
      bool ok = true;
      if (activeJob == JOB_GENERATE_PNG) {
        for (uint8_t j = 0; j < count; j++) {
          ok = ok && generateFramePng(jobs[j].output_path, jobs[j].frame_data,
                                      jobs[j].width, jobs[j].height,
                                      jobs[j].color_depth, &output_image);
        }
      } else if (activeJob == JOB_GENERATE_SINGLE) {
        for (uint8_t j = 0; j < count; j++) {
          size_t frame_size = jobs[j].width * jobs[j].height;
          if (jobs[j].color_depth == 1)
            frame_size >>= 3;
          else if (jobs[j].color_depth == 2)
            frame_size >>= 2;
          else
            assert(jobs[j].color_depth == 8);
          ok = ok && generateFrameRaw(jobs[j].output_path, jobs[j].header_data,
                                      jobs[j].frame_data, frame_size,
                                      jobs[j].footer_data);
        }
      }
      char status[256];
      int len =
          snprintf(status, sizeof status, "%u...%u (%u) (jobs left: %u): %s\n",
                   jobs[0].frame_id, jobs[count - 1].frame_id, count,
                   jobs_left - count, ok ? "OK" : "FAIL");
      fwrite(status, 1, len, stdout);
#ifdef THREADED
    }
#endif
  }
}

// void setup_debug_handlers(void);

int main(int argc, char *argv[]) {
  // setup_debug_handlers();
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <input file(s) (.raw)> <output folder path>\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  dcrc64 *crc = boxing_math_crc64_create_def();

  enum JobKind job_kind =
      (argc >= 4
           ? (strcmp(argv[argc - 1], "split") == 0      ? JOB_GENERATE_SINGLE
              : strcmp(argv[argc - 1], "read") == 0     ? JOB_GENERATE_READ
              : strcmp(argv[argc - 1], "validate") == 0 ? JOB_GENERATE_VALIDATE
                                                        : JOB_GENERATE_PNG)
           : JOB_GENERATE_PNG);
  PendingFrameGenerationJobList job_list = {
      .job = job_kind,
      .jobs = {.data = NULL, .size = 0},
      .count = 0,
      .offset = 0,
      .done = false,
#ifdef THREADED
#ifdef _WIN32
      .mutex = CreateMutexA(NULL, FALSE, NULL),
#else
      .mutex = PTHREAD_MUTEX_INITIALIZER,
#endif
#endif
  };

  for (size_t in_file_idx = 1;
       in_file_idx < (size_t)argc - (job_kind == JOB_GENERATE_PNG ? 1 : 2);
       in_file_idx++) {
#ifdef THREADED
#ifdef _WIN32
    void *threads[THREAD_COUNT];
    for (unsigned i = 0; i < THREAD_COUNT; i++) {
      void *thread = CreateThread(
          NULL, 0, (LPTHREAD_START_ROUTINE)frameGeneratorWorker, NULL, 0, NULL);
      if (thread == NULL) {
        fprintf(stderr, "Failed to create thread\n");
        if (job_list.jobs.data)
          free(job_list.jobs.data);
        boxing_math_crc64_free(crc);
        return EXIT_FAILURE;
      }
      threads[i] = thread;
    }
#else
    pthread_t threads[THREAD_COUNT];
    for (unsigned i = 0; i < THREAD_COUNT; i++) {
      if (pthread_create(&threads[i], NULL,
                         (void *(*)(void *))frameGeneratorWorker, &job_list)) {
        fprintf(stderr, "Failed to create thread\n");
        if (job_list.jobs.data)
          free(job_list.jobs.data);
        boxing_math_crc64_free(crc);
        return EXIT_FAILURE;
      }
    }
#endif
#endif

    const char *folder_path =
        argv[argc - (job_kind == JOB_GENERATE_PNG ? 1 : 2)];

    mkdir(folder_path, 0755);

    const char *input_file = argv[in_file_idx];

    const Slice reel = mapFile(input_file);

    const uint8_t *const ptr = (const uint8_t *)reel.data;
    size_t i = 0;
    while (i < reel.size) {
      const RawFileHeader *const header = (const RawFileHeader *)(ptr + i);
      i += sizeof *header;
      const uint8_t *const data = ptr + i;
      if (header->color_depth == 1)
        i += (header->frame_height * header->frame_width) / 8;
      else if (header->color_depth == 2)
        i += (header->frame_height * header->frame_width) / 4;
      else if (header->color_depth == 8)
        i += (header->frame_height * header->frame_width);
      else {
        fprintf(stderr, "Invalid color depth: %" PRIu8 "\n",
                header->color_depth);
        if (job_list.jobs.data)
          free(job_list.jobs.data);
        unmapFile(reel);
        boxing_math_crc64_free(crc);
        return EXIT_FAILURE;
      }
      const RawFileFooter *const footer = (const RawFileFooter *)(ptr + i);
      i += sizeof *footer;

      if (job_kind == JOB_GENERATE_READ || job_kind == JOB_GENERATE_VALIDATE) {

        printHeader(header);
        fwrite(" ", 1, 1, stdout);
        printFooter(footer);
        if (job_kind != JOB_GENERATE_VALIDATE)
          fwrite("\n", 1, 1, stdout);
      }

      if (job_kind == JOB_GENERATE_VALIDATE) {
        size_t frame_size = header->frame_width * header->frame_height;
        if (header->color_depth == 1)
          frame_size >>= 3;
        else if (header->color_depth == 2)
          frame_size >>= 2;
        else
          assert(header->color_depth == 8);
        boxing_math_crc64_calc_crc(crc, (const char *)header,
                                   sizeof *header + frame_size +
                                       sizeof *footer - 8);
        uint64_t checksum = boxing_math_crc64_get_crc(crc);

        boxing_math_crc64_reset(crc, 0);
        uint64_t checksum_from_file;
        memcpy(&checksum_from_file, &footer->crc, sizeof checksum_from_file);
        bool eq = checksum_from_file ==
#if BYTE_ORDER == LITTLE_ENDIAN
#ifdef _WIN32
                  _byteswap_uint64(checksum)
#else
                  __builtin_bswap64(checksum)
#endif
#else
                  checksum
#endif
            ;
        printf("\x1b[%dm %s= %016" PRIx64 "\x1b[0m\n", eq ? 92 : 91,
               eq ? "=" : "!", checksum);
      } else if (job_kind != JOB_GENERATE_READ) {

#ifdef THREADED
#ifdef _WIN32
        WaitForSingleObject(job_list.mutex, INFINITE);
#else
        pthread_mutex_lock(&job_list.mutex);
#endif
#endif
        if (!grow(&job_list.jobs.data, sizeof(FrameGenerationJob),
                  &job_list.jobs.size, job_list.count + 1)) {
#ifdef THREADED
#ifdef _WIN32
          ReleaseMutex(job_list.mutex);
#else
          pthread_mutex_unlock(&job_list.mutex);
#endif
#endif
          fprintf(stderr, "OOM\n");
          return EXIT_FAILURE;
        }
        FrameGenerationJob *job =
            (FrameGenerationJob *)(job_list.jobs.data) + job_list.count++;
        snprintf(job->output_path, sizeof job->output_path,
                 "%s/%05" PRIu64 ".%s", folder_path, header->frame_id,
                 job_kind == JOB_GENERATE_PNG ? "png" : "raw");
        job->frame_id = header->frame_id;
        job->header_data = header;
        job->frame_data = data;
        job->footer_data = footer;
        job->width = header->frame_width;
        job->height = header->frame_height;
        job->color_depth = header->color_depth;
#ifdef THREADED
#ifdef _WIN32
        ReleaseMutex(job_list.mutex);
#else
        pthread_mutex_unlock(&job_list.mutex);
#endif
#endif
      }
    }

    if (job_kind == JOB_GENERATE_VALIDATE) {
      if (i == reel.size) {
        printf("\x1b[92mFILE SIZE OK\x1b[0m\n");
      } else {
        printf("\x1b[91mFILE SIZE MISMATCH: Total size is %zu, offset after "
               "iterating all frames is: %zu\x1b[0m\n",
               reel.size, i);
      }
    }
#ifdef THREADED
#ifdef _WIN32
    WaitForSingleObject(job_list.mutex, INFINITE);
#else
    pthread_mutex_lock(&job_list.mutex);
#endif
#endif
    job_list.done = true;
#ifdef THREADED
#ifdef _WIN32
    ReleaseMutex(job_list.mutex);
#else
    pthread_mutex_unlock(&job_list.mutex);
#endif
#endif

#ifdef THREADED
#ifdef _WIN32
    WaitForMultipleObjects(THREAD_COUNT, threads, TRUE, INFINITE);
#else
    for (unsigned i = 0; i < THREAD_COUNT; i++)
      pthread_join(threads[i], NULL);
#endif
#else
    frameGeneratorWorker(&job_list);
#endif
    unmapFile(reel);
  }

  fflush(stdout);
  if (job_list.jobs.data)
    free(job_list.jobs.data);
  boxing_math_crc64_free(crc);
  return EXIT_SUCCESS;
}
