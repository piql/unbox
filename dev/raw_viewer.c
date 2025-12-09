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
  int non_fullscreen_width = screenWidth;
  int non_fullscreen_height = screenHeight;
  Vector2 non_fullscreen_position = {0, 0};
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
  bool invert = false;
  char search[32] = {0};
  size_t search_idx = 0;
  snprintf(title, sizeof title, "Raw Viewer%s%s", argc > 1 ? " - " : "",
           argc > 1 ? argv[1] : "");
  InitWindow(screenWidth, screenHeight, title);
  SetTargetFPS(60);

  const char *const invertFragmentShader =
      "#version 330 core\n"
      "in vec2 fragTexCoord;\n"
      "out vec4 fragColor;\n"
      "uniform sampler2D texture0;\n"
      "void main() {\n"
      "  vec4 color = texture(texture0, fragTexCoord);\n"
      "  fragColor = vec4(1.0 - color.rgb, 1.0);\n"
      "}\n";

  Shader invertShader = LoadShaderFromMemory(NULL, invertFragmentShader);
  if (!IsShaderValid(invertShader)) {
    fprintf(stderr, "Failed to load shader\n");
    CloseWindow();
    unmapFile(file);
    free(positions.data);
    free(image.data);
    return EXIT_FAILURE;
  }
  Texture2D tex = {0};
  bool user_quit = false;
  int exit_code = EXIT_SUCCESS;
  float fixed_scale = 1;
  bool use_fixed_scale = false;
  Vector2 pan = {0, 0};
  while (!WindowShouldClose()) {
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();
    if (!IsWindowFullscreen()) {
      non_fullscreen_width = screenWidth;
      non_fullscreen_height = screenHeight;
      non_fullscreen_position = GetWindowPosition();
    }
    float natural_scale = min((float)screenWidth / (float)tex.width,
                              (float)screenHeight / (float)tex.height);

    int c = GetCharPressed();
    while (c != '\0') {
      if (c >= '0' && c <= '9' && search_idx < sizeof search - 1) {
        search[search_idx++] = (char)c;
        search[search_idx] = '\0';
      }
      if (c == '+') {
        if (!use_fixed_scale)
          fixed_scale = natural_scale;
        fixed_scale = clamp(fixed_scale + (0.1f * fixed_scale), 0.1f, 100);
        use_fixed_scale = true;
      }
      if (c == '-') {
        if (!use_fixed_scale)
          fixed_scale = natural_scale;
        fixed_scale = clamp(fixed_scale - (0.1f * fixed_scale), 0.1f, 100);
        use_fixed_scale = true;
      }
      c = GetCharPressed();
    }

    int k = GetKeyPressed();
    size_t next_position = current_position;
    while (k != 0) {
      if (k == KEY_Q)
        user_quit = true;
      else if (k == KEY_LEFT && next_position > 0)
        next_position--;
      else if (k == KEY_RIGHT && next_position + 1 < positions.size)
        next_position++;
      else if (k == KEY_BACKSPACE && search_idx != 0)
        search[--search_idx] = '\0';
      else if (k == KEY_ENTER && search_idx != 0) {
        errno = 0;
        size_t entered_pos = (size_t)strtoul(search, NULL, 10);
        if (!errno) {
          next_position =
              min(entered_pos, positions.size ? positions.size - 1 : 0);
        }
        search_idx = 0;
        search[search_idx] = '\0';
      } else if (k == KEY_SPACE || k == KEY_H)
        show_text = !show_text;
      else if (k == KEY_I)
        invert = !invert;
      else if (k == KEY_F || k == KEY_F11) {
        if (!IsWindowFullscreen()) {
          // Going into fullscreen, set appropriate resolution
          int monitor = GetCurrentMonitor();
          int monitor_width = GetMonitorWidth(monitor);
          int monitor_height = GetMonitorHeight(monitor);
          SetWindowSize(monitor_width, monitor_height);
          ToggleFullscreen();
        } else {
          ToggleFullscreen();
          SetWindowSize(non_fullscreen_width, non_fullscreen_height);
          SetWindowPosition((int)non_fullscreen_position.x,
                            (int)non_fullscreen_position.y);
        }
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
      } else if (k == KEY_R) {
        use_fixed_scale = false;
        pan = (Vector2){0, 0};
      }
      k = GetKeyPressed();
    }
    if (user_quit)
      break;

    if (IsKeyPressedRepeat(KEY_LEFT) && next_position > 0)
      next_position--;
    if (IsKeyPressedRepeat(KEY_RIGHT) && next_position + 1 < positions.size)
      next_position++;

    Image img;

    if ((tex.id == 0 || current_position != next_position) && positions.data) {
      current_position = next_position;
      size_t pos = ((size_t *)positions.data)[current_position];
      const RawFileHeader *const header = (const RawFileHeader *)pos;
      const uint8_t *data = (const uint8_t *)(pos + sizeof *header);
      if (!splat_pixels(data, header->frame_width, header->frame_height,
                        header->color_depth, &image)) {
        fprintf(stderr, "Failed to splat pixels\n");
        exit_code = EXIT_FAILURE;
        break;
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
      natural_scale = min((float)screenWidth / (float)tex.width,
                          (float)screenHeight / (float)tex.height);
    }

    float mousewheel = GetMouseWheelMove();
    if (nonzeroish_float(mousewheel)) {
      if (!use_fixed_scale)
        fixed_scale = natural_scale;
      fixed_scale =
          clamp(fixed_scale + ((0.1f * mousewheel) * fixed_scale), 0.1f, 100);
      use_fixed_scale = true;
    }

    if (fixed_scale < natural_scale) {
      fixed_scale = natural_scale;
      use_fixed_scale = false;
    }

    Vector2 mousemove = GetMouseDelta();
    if ((nonzeroish_float(mousemove.x) || nonzeroish_float(mousemove.y)) &&
        IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      pan.x += mousemove.x;
      pan.y += mousemove.y;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    if (positions.data) {
      float tex_scale = use_fixed_scale ? fixed_scale : natural_scale;
      SetTextureFilter(tex, use_fixed_scale ? TEXTURE_FILTER_POINT
                                            : TEXTURE_FILTER_TRILINEAR);
      Vector2 offset = {
          .x = ((screenWidth - tex.width * tex_scale) / 2) + pan.x,
          .y = ((screenHeight - tex.height * tex_scale) / 2) + pan.y,
      };
      if (invert)
        BeginShaderMode(invertShader);
      DrawTextureEx(tex, offset, 0, tex_scale, WHITE);
      if (invert)
        EndShaderMode();
      if (show_text) {
        if (search[0] != '\0') {
          DrawText(search, 0, 0, 64, BLACK);
        } else {
          char pos[32];
          snprintf(pos, sizeof pos, "%zu\n", current_position);
          DrawText(pos, 0, 0, 64, GRAY);
          if (current_position == 0) {
            const char *const help_text = "Q - quit\n"
                                          "+ - zoom in\n"
                                          "- - zoom out\n"
                                          "R - reset zoom\n"
                                          "H - toggle text\n"
                                          "I - invert image\n"
                                          "F - fullscreen\n";
            DrawText(help_text, 0, 128, 48, GRAY);
          }
        }
      }
    } else {
      DrawText("Run this program with\na reel.raw file argument\n\nraw_viewer reel.raw", 0, 0, 64,
               GRAY);
    }
    EndDrawing();
  }
  UnloadTexture(tex);
  UnloadShader(invertShader);
  CloseWindow();
  unmapFile(file);
  free(positions.data);
  free(image.data);
  return exit_code;
}
