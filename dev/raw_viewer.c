#include "../dep/raylib/src/raylib.h"
#include <stdlib.h>

#include "../src/grow.c"
#include "../src/map_file.c"
#include "raw_file.c"

int main(int argc, char **argv) {
  int screenWidth = 800;
  int screenHeight = 600;
  uint32_t image_width = 4096;
  uint32_t image_height = 2160;
  Slice image_data = {
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
    }
    i += sizeof(RawFileFooter);
  }
  SetTraceLogLevel(LOG_ERROR);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  char title[1024];
  snprintf(title, sizeof title, "Raw Viewer%s%s", argc > 1 ? " - " : "", argc > 1 ? argv[1] : "");
  InitWindow(screenWidth, screenHeight, title);
  SetTargetFPS(60);
  Texture2D tex;
  bool current_position_changed = true;
  while (!WindowShouldClose()) {
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();
    if ((IsKeyPressed(KEY_Q) || IsKeyPressedRepeat(KEY_Q)))
      break;
    if ((IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) &&
        current_position > 0) {
      current_position -= 1;
      current_position_changed = true;
    }
    if ((IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) &&
        current_position < positions.size - 1) {
      current_position += 1;
      current_position_changed = true;
    }
    Image img;

    if (current_position_changed && positions.data) {
      current_position_changed = false;
      size_t pos = ((size_t *)positions.data)[current_position];
      const RawFileHeader *const header = (const RawFileHeader *)pos;
      const uint8_t *data = (const uint8_t *)(pos + sizeof *header);
      if (!splat_pixels(data, header->frame_width, header->frame_height,
                        header->color_depth, &image_data)) {
        fprintf(stderr, "Failed to splat pixels\n");
      }
      image_width = header->frame_width;
      image_height = header->frame_height;
      img = (Image){
          .data = (void *)image_data.data,
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
    }
    EndDrawing();
  }
  UnloadTexture(tex);
  CloseWindow();
  return EXIT_SUCCESS;
}
