#include "../src/grow.c"
#include "../src/map_file.c"
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
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

typedef struct {
  uint64_t frame_id;
  uint32_t frame_height;
  uint32_t frame_width;
  uint8_t color_depth;
  uint8_t version;
  uint8_t colors_per_channel;
  uint8_t reserved_[13];
} RawFileHeader;

static void printHeader(const RawFileHeader *const h) {
  char buf[256];
  int len = snprintf(buf, sizeof buf,
                     "Header#%05" PRIu64 " v%" PRIu8 " %" PRIu32 "x%" PRIu32
                     " %" PRIu8 "bpp %" PRIu8 "cpc",
                     h->frame_id, h->version, h->frame_width, h->frame_height,
                     h->color_depth, h->colors_per_channel);
  fwrite(buf, 1, len, stdout);
}

typedef struct {
  uint8_t reserved_[24];
  uint8_t crc[8];
} RawFileFooter;

static void printFooter(const RawFileFooter *const f) {
  char buf[256];
  int len = snprintf(buf, sizeof buf,
                     "Footer "
                     "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8 "%02" PRIx8
                     "%02" PRIx8 "%02" PRIx8 "%02" PRIx8,
                     f->crc[0], f->crc[1], f->crc[2], f->crc[3], f->crc[4],
                     f->crc[5], f->crc[6], f->crc[7]);
  fwrite(buf, 1, len, stdout);
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../dep/stb/stb_image_write.h"

static bool generateFramePng(const char *const restrict output_path,
                             const uint8_t *const restrict data,
                             const uint32_t width, const uint32_t height,
                             const uint8_t color_depth,
                             Slice *const output_image) {
#ifdef _WIN32
  DWORD attr = GetFileAttributesA(output_path);
  if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
    return true;
#else
  struct stat s;
  if (stat(output_path, &s) == 0)
    return true;
#endif

  const size_t frame_size = width * height;
  if (output_image->size < frame_size) {
    void *const new_output_image = realloc(output_image->data, frame_size);
    if (!new_output_image)
      return false;
    output_image->data = new_output_image;
    output_image->size = frame_size;
  }

  uint8_t *const out_data = (uint8_t *)output_image->data;
  if (color_depth == 8)
    memcpy(out_data, data, frame_size);
  else if (color_depth == 2) {
    for (unsigned i = 0; i < frame_size >> 2; i++) {
#if BYTE_ORDER == LITTLE_ENDIAN
      uint32_t x = data[i];
      x |= x << 6;
      x |= x << 12;
      x &= 0x03030303;
      x *= 85;
      ((uint32_t *)out_data)[i] = x;
#else
      out_data[i * 4 + 0] = ((data[i] & (3 << 0)) >> 0) * 85;
      out_data[i * 4 + 1] = ((data[i] & (3 << 2)) >> 2) * 85;
      out_data[i * 4 + 2] = ((data[i] & (3 << 4)) >> 4) * 85;
      out_data[i * 4 + 3] = ((data[i] & (3 << 6)) >> 6) * 85;
#endif
    }
  } else if (color_depth == 1) {
    for (unsigned i = 0; i < frame_size / 8; i++) {
      out_data[i * 8 + 0] = ((data[i] & (1 << 0)) >> 0) * 255;
      out_data[i * 8 + 1] = ((data[i] & (1 << 1)) >> 1) * 255;
      out_data[i * 8 + 2] = ((data[i] & (1 << 2)) >> 2) * 255;
      out_data[i * 8 + 3] = ((data[i] & (1 << 3)) >> 3) * 255;
      out_data[i * 8 + 4] = ((data[i] & (1 << 4)) >> 4) * 255;
      out_data[i * 8 + 5] = ((data[i] & (1 << 5)) >> 5) * 255;
      out_data[i * 8 + 6] = ((data[i] & (1 << 6)) >> 6) * 255;
      out_data[i * 8 + 7] = ((data[i] & (1 << 7)) >> 7) * 255;
    }
  } else {
    return false;
  }

  return stbi_write_png(output_path, width, height, 1, out_data, width) != 0;
}

typedef struct {
  char output_path[256];
  uint16_t frame_id;
  const uint8_t *frame_data;
  uint32_t width;
  uint32_t height;
  uint8_t color_depth;
} FrameGenerationJob;

typedef struct {
  Slice jobs;
  uint16_t count;
  uint16_t offset;
  bool done;
#ifdef THREADED
#ifdef _WIN32
  HANDLE mutex;
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
      for (uint8_t j = 0; j < count; j++) {
        ok = ok && generateFramePng(jobs[j].output_path, jobs[j].frame_data,
                                    jobs[j].width, jobs[j].height,
                                    jobs[j].color_depth, &output_image);
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

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <input file (.raw)> <output folder path>\n",
            argv[0]);
    return EXIT_FAILURE;
  }
  PendingFrameGenerationJobList job_list = {
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

#ifdef THREADED
#ifdef _WIN32
  HANDLE threads[THREAD_COUNT];
  for (unsigned i = 0; i < THREAD_COUNT; i++) {
    HANDLE thread = CreateThread(NULL, 0, frameGeneratorWorker, NULL, 0, NULL);
    if (thread == NULL) {
      fprintf(stderr, "Failed to create thread\n");
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
      return EXIT_FAILURE;
    }
  }
#endif
#endif

  const char *input_file = argv[1];
  const char *folder_path = argv[2];

#ifdef _WIN32
  mkdir(folder_path);
#else
  mkdir(folder_path, 0755);
#endif
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
      fprintf(stderr, "Invalid color depth: %" PRIu8 "\n", header->color_depth);
      if (job_list.jobs.data)
        free(job_list.jobs.data);
      unmapFile(reel);
      return EXIT_FAILURE;
    }
    const RawFileFooter *const footer = (const RawFileFooter *)(ptr + i);
    i += sizeof *footer;

    (void)printHeader;
    (void)printFooter;
    // printHeader(header);
    // fwrite(" ", 1, 1, stdout);
    // printFooter(footer);
    // fwrite("\n", 1, 1, stdout);

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
    snprintf(job->output_path, sizeof job->output_path, "%s/%05" PRIu64 ".png",
             folder_path, header->frame_id);
    job->frame_id = header->frame_id;
    job->frame_data = data;
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

  fflush(stdout);
  if (job_list.jobs.data)
    free(job_list.jobs.data);
  unmapFile(reel);
  return EXIT_SUCCESS;
}
