#include "../dep/raylib/src/raylib.h"
#include <errno.h>
#include <stdlib.h>

#include "../src/grow.c"
#include "../src/map_file.c"
#include "raw_file.c"

int main(int argc, char **argv) {
  (void)printHeader;
  (void)printFooter;
  int screenWidth = 800;
  int screenHeight = 600;
  uint32_t image_width = 4096;
  uint32_t image_height = 2160;
  Slice image = {
      .data = malloc(image_width * image_height),
      .size = image_width * image_height,
  };
  Slice positions = {0};
  size_t positions_cap = 0;
  size_t current_position = 0;
  Slice file = {0};
  if (argc > 1) {
    file = mapFile(argv[1]);
  }
  const uint8_t *const ptr = (const uint8_t *)file.data;
  size_t i = 0;
  while (i < file.size) {
    const RawFileHeader *const header = (const RawFileHeader *)(ptr + i);
    size_t position = (size_t)header;
    grow(&positions.data, sizeof position, &positions_cap, positions.size + 1);
    ((size_t *)positions.data)[positions.size++] = position;
    i += sizeof *header;
    if (header->color_depth == 1)
      i += (header->frame_width * header->frame_height) / 8;
    else if (header->color_depth == 2)
      i += (header->frame_width * header->frame_height) / 4;
    else if (header->color_depth == 8)
      i += (header->frame_width * header->frame_height);
    else {
      fprintf(stderr, "Invalid color depth\n");
      unmapFile(file);
      free(positions.data);
      free(image.data);
      return EXIT_FAILURE;
    }
    i += sizeof(RawFileFooter);
  }
  SetTraceLogLevel(LOG_ERROR);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  char title[1024];
  bool show_text = true;
  char search[32] = {0};
  size_t search_idx = 0;
  snprintf(title, sizeof title, "Raw Viewer%s%s", argc > 1 ? " - " : "",
           argc > 1 ? argv[1] : "");
  InitWindow(screenWidth, screenHeight, title);
  SetTargetFPS(60);
  Texture2D tex = {0};
  bool current_position_changed = true;
  bool user_quit = false;
  while (!WindowShouldClose()) {
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    int c = GetCharPressed();
    while (c != '\0') {
      if (c >= '0' && c <= '9' && search_idx < sizeof search - 1) {
        search[search_idx++] = c;
        search[search_idx] = '\0';
      }
      c = GetCharPressed();
    }

    int k = GetKeyPressed();
    int position_change = 0;
    while (k != 0) {
      if (k == KEY_Q)
        user_quit = true;
      else if (k == KEY_LEFT)
        position_change--;
      else if (k == KEY_RIGHT)
        position_change++;
      else if (k == KEY_BACKSPACE)
        search[--search_idx] = '\0';
      else if (k == KEY_ENTER && search_idx != 0) {
        errno = 0;
        size_t entered_pos = (size_t)strtoul(search, NULL, 10);
        if (!errno && entered_pos < positions.size &&
            entered_pos != current_position) {
          current_position = entered_pos;
          current_position_changed = true;
        }
        search_idx = 0;
        search[search_idx] = '\0';
      } else if (k == KEY_SPACE || k == KEY_H)
        show_text = !show_text;
      k = GetKeyPressed();
    }
    if (user_quit)
      break;

    if (IsKeyPressedRepeat(KEY_LEFT))
      position_change--;
    if (IsKeyPressedRepeat(KEY_RIGHT))
      position_change++;

    int clamped_change =
        position_change < 0   ? current_position == 0 ? 0 : position_change
        : position_change > 0 ? current_position >= positions.size - 1
                                    ? positions.size - 1
                                    : position_change
                              : position_change;
    if (clamped_change != 0) {
      current_position_changed = true;
      current_position += clamped_change;
    }

    Image img;

    if (current_position_changed && positions.data) {
      current_position_changed = false;
      size_t pos = ((size_t *)positions.data)[current_position];
      const RawFileHeader *const header = (const RawFileHeader *)pos;
      const uint8_t *data = (const uint8_t *)(pos + sizeof *header);
      if (!splat_pixels(data, header->frame_width, header->frame_height,
                        header->color_depth, &image)) {
        fprintf(stderr, "Failed to splat pixels\n");
        UnloadTexture(tex);
        CloseWindow();
        unmapFile(file);
        free(positions.data);
        free(image.data);
        return EXIT_FAILURE;
      }
      image_width = header->frame_width;
      image_height = header->frame_height;
      img = (Image){
          .data = (void *)image.data,
          .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE,
          .width = image_width,
          .height = image_height,
          .mipmaps = 1,
      };
      UnloadTexture(tex);
      tex = LoadTextureFromImage(img);
      GenTextureMipmaps(&tex);
      SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
    }
    BeginDrawing();
    ClearBackground(RAYWHITE);
    if (positions.data) {
      float tex_scale = min((float)screenWidth / (float)tex.width,
                            (float)screenHeight / (float)tex.height);
      Vector2 offset = {
          .x = (screenWidth - tex.width * tex_scale) / 2,
          .y = (screenHeight - tex.height * tex_scale) / 2,
      };
      DrawTextureEx(tex, offset, 0, tex_scale, WHITE);
      if (show_text) {
        if (search[0] != '\0') {
          DrawText(search, 0, 0, 64, BLACK);
        } else {
          char pos[32];
          snprintf(pos, sizeof pos, "%zu\n", current_position);
          DrawText(pos, 0, 0, 64, GRAY);
        }
      }
    }
    EndDrawing();
  }
  UnloadTexture(tex);
  CloseWindow();
  unmapFile(file);
  free(positions.data);
  free(image.data);
  return EXIT_SUCCESS;
}
