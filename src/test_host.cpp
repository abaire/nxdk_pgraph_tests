#include "test_host.h"

#include <SDL.h>
#include <SDL_image.h>
#include <fpng/src/fpng.h>
#include <strings.h>

#include <cmath>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#include <windows.h>
#pragma clang diagnostic pop

#include <texture_generator.h>
#include <xboxkrnl/xboxkrnl.h>

#include <algorithm>
#include <utility>

#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "pushbuffer.h"
#include "shaders/vertex_shader_program.h"
#include "vertex_buffer.h"
#include "xbox_math_d3d.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"
#include "xbox_math_util.h"

using namespace XboxMath;

#define SAVE_Z_AS_PNG

#define MAX_FILE_PATH_SIZE 248
#define MAX_FILENAME_SIZE 42

TestHost::TestHost(std::shared_ptr<FTPLogger> ftp_logger, uint32_t framebuffer_width, uint32_t framebuffer_height,
                   uint32_t max_texture_width, uint32_t max_texture_height, uint32_t max_texture_depth)
    : NV2AState(framebuffer_width, framebuffer_height, max_texture_width, max_texture_height, max_texture_depth),
      ftp_logger_{std::move(ftp_logger)} {}

void TestHost::EnsureFolderExists(const std::string &folder_path) {
  if (folder_path.length() > MAX_FILE_PATH_SIZE) {
    ASSERT(!"Folder Path is too long.");
  }

  char buffer[MAX_FILE_PATH_SIZE + 1] = {0};
  const char *path_start = folder_path.c_str();
  const char *slash = strchr(path_start, '\\');
  slash = strchr(slash + 1, '\\');

  while (slash) {
    strncpy(buffer, path_start, slash - path_start);
    if (!CreateDirectory(buffer, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
      ASSERT(!"Failed to create output directory.");
    }

    slash = strchr(slash + 1, '\\');
  }

  // Handle case where there was no trailing slash.
  if (!CreateDirectory(path_start, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
    ASSERT(!"Failed to create output directory.");
  }
}

// Returns the full output filepath including the filename
// Creates output directory if it does not exist
std::string TestHost::PrepareSaveFile(std::string output_directory, const std::string &filename,
                                      const std::string &extension) {
  if (filename.length() > MAX_FILENAME_SIZE) {
    PrintMsg("Filename '%s' > %d characters\n", filename.c_str(), MAX_FILENAME_SIZE);
    ASSERT(!"File name is too long");
  }

  EnsureFolderExists(output_directory);

  output_directory += "\\";
  output_directory += filename;
  output_directory += extension;

  if (output_directory.length() > MAX_FILE_PATH_SIZE) {
    PrintMsg("File save path '%s' > %d characters\n", output_directory.c_str(), MAX_FILE_PATH_SIZE);
    ASSERT(!"Full save file path is too long.");
  }

  return output_directory;
}

std::string TestHost::SaveBackBuffer(const std::string &output_directory, const std::string &name) {
  auto target_file = PrepareSaveFile(output_directory, name);

  auto buffer = pb_agp_access(pb_back_buffer());
  auto width = static_cast<int>(pb_back_buffer_width());
  auto height = static_cast<int>(pb_back_buffer_height());
  auto pitch = static_cast<int>(pb_back_buffer_pitch());

  // FIXME: Support 16bpp surfaces
  ASSERT((pitch == width * 4) && "Expected packed 32bpp surface");

  // Swizzle color channels ARGB -> ABGR
  unsigned int num_pixels = width * height;
  uint32_t *pre_enc_buf = (uint32_t *)malloc(num_pixels * 4);
  ASSERT(pre_enc_buf && "Failed to allocate pre-encode buffer");
  for (unsigned int i = 0; i < num_pixels; i++) {
    uint32_t c = static_cast<uint32_t *>(buffer)[i];
    pre_enc_buf[i] = (c & 0xff00ff00) | ((c >> 16) & 0xff) | ((c & 0xff) << 16);
  }

  std::vector<uint8_t> out_buf;
  if (!fpng::fpng_encode_image_to_memory((void *)pre_enc_buf, width, height, 4, out_buf)) {
    ASSERT(!"Failed to encode PNG image");
  }
  free(pre_enc_buf);

  FILE *pFile = fopen(target_file.c_str(), "wb");
  ASSERT(pFile && "Failed to open output PNG image");
  auto bytes_remaining = out_buf.size();
  auto data = out_buf.data();
  while (bytes_remaining > 0) {
    auto bytes_written = fwrite(data, 1, bytes_remaining, pFile);
    if (!bytes_written) {
      ASSERT(!"Failed to write output PNG image");
    }
    data += bytes_written;
    bytes_remaining -= bytes_written;
  }
  if (fclose(pFile)) {
    ASSERT(!"Failed to close output PNG image");
  }

  return target_file;
}

std::string TestHost::SaveZBuffer(const std::string &output_directory, const std::string &name) const {
  uint32_t depth = depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? 16 : 32;
#ifdef SAVE_Z_AS_PNG
  auto format =
      depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? SDL_PIXELFORMAT_RGB565 : SDL_PIXELFORMAT_ARGB8888;
  return SaveTexture(output_directory, name, pb_depth_stencil_buffer(), framebuffer_width_, framebuffer_height_,
                     pb_depth_stencil_pitch(), depth, format);
#else
  return SaveRawTexture(output_directory, name, pb_depth_stencil_buffer(), framebuffer_width_, framebuffer_height_,
                        framebuffer_width_ * 4, depth);
#endif
}

std::string TestHost::SaveTexture(const std::string &output_directory, const std::string &name, const uint8_t *texture,
                                  uint32_t width, uint32_t height, uint32_t pitch, uint32_t bits_per_pixel,
                                  SDL_PixelFormatEnum format) {
  auto target_file = PrepareSaveFile(output_directory, name);

  auto buffer = pb_agp_access(const_cast<void *>(static_cast<const void *>(texture)));
  auto size = pitch * height;

  PrintMsg("Saving to %s. Size: %lu. Pitch %lu.\n", target_file.c_str(), size, pitch);

  SDL_Surface *surface =
      SDL_CreateRGBSurfaceWithFormatFrom((void *)buffer, static_cast<int>(width), static_cast<int>(height),
                                         static_cast<int>(bits_per_pixel), static_cast<int>(pitch), format);

  if (IMG_SavePNG(surface, target_file.c_str())) {
    PrintMsg("Failed to save PNG file '%s'\n", target_file.c_str());
    ASSERT(!"Failed to save PNG file.");
  }

  SDL_FreeSurface(surface);

  return target_file;
}

std::string TestHost::SaveRawTexture(const std::string &output_directory, const std::string &name,
                                     const uint8_t *texture, uint32_t width, uint32_t height, uint32_t pitch,
                                     uint32_t bits_per_pixel) {
  auto target_file = PrepareSaveFile(output_directory, name, ".raw");

  auto buffer = static_cast<uint8_t *>(pb_agp_access(const_cast<void *>(static_cast<const void *>(texture))));
  const uint32_t bytes_per_pixel = (bits_per_pixel >> 3);
  const uint32_t populated_pitch = width * bytes_per_pixel;
  const auto size = populated_pitch * height;

  PrintMsg("Saving to %s. Size: %lu. Pitch %lu.\n", target_file.c_str(), size, pitch);

  FILE *f = fopen(target_file.c_str(), "wb");
  ASSERT(f && "Failed to open raw texture output file.");

  for (uint32_t y = 0; y < height; ++y) {
    auto written = fwrite(buffer, populated_pitch, 1, f);
    ASSERT(written == 1 && "Failed to write row to raw texture output file.");
    buffer += pitch;
  }

  fclose(f);

  return target_file;
}

void TestHost::FinishDraw(bool allow_saving, const std::string &output_directory, const std::string &suite_name,
                          const std::string &name, bool save_zbuffer) {
  bool perform_save = allow_saving && save_results_;
  if (!perform_save) {
    pb_printat(0, 55, (char *)"ns");
    pb_draw_text_screen();
  }

  PBKitBusyWait();

  if (perform_save) {
    // TODO: See why waiting for tiles to be non-busy results in the screen not updating anymore.
    // In theory this should wait for all tiles to be rendered before capturing.
    pb_wait_for_vbl();

    auto output_path = SaveBackBuffer(output_directory, name);

    if (ftp_logger_) {
      auto remote_filename = suite_name + "::" + output_path.substr(output_directory.length() + 1);
      ftp_logger_->QueuePutFile(output_path, remote_filename);
    }

    if (save_zbuffer) {
      std::string z_buffer_name = name + "_ZB";
      auto z_buffer_output_path = SaveZBuffer(output_directory, z_buffer_name);

      if (ftp_logger_) {
        auto remote_filename = suite_name + "::" + z_buffer_output_path.substr(output_directory.length() + 1);
        ftp_logger_->QueuePutFile(z_buffer_output_path, remote_filename);
      }
    }
  }

  /* Swap buffers (if we can) */
  while (pb_finished()) {
    /* Not ready to swap yet */
  }
}

std::string TestHost::GetDrawPrimitiveName(DrawPrimitive primitive) {
  switch (primitive) {
    case PRIMITIVE_POINTS:
      return "Points";
    case PRIMITIVE_LINES:
      return "Lines";
    case PRIMITIVE_LINE_LOOP:
      return "LineLoop";
    case PRIMITIVE_LINE_STRIP:
      return "LineStrip";
    case PRIMITIVE_TRIANGLES:
      return "Tris";
    case PRIMITIVE_TRIANGLE_STRIP:
      return "TriStrip";
    case PRIMITIVE_TRIANGLE_FAN:
      return "TriFan";
    case PRIMITIVE_QUADS:
      return "Quads";
    case PRIMITIVE_QUAD_STRIP:
      return "QuadStrip";
    case PRIMITIVE_POLYGON:
      return "Poly";
  }
}
