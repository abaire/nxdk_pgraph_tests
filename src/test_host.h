#ifndef NXDK_PGRAPH_TESTS_TEST_HOST_H
#define NXDK_PGRAPH_TESTS_TEST_HOST_H

#include <debug_output.h>
#include <ftp_logger.h>
#include <pbkit/pbkit.h>
#include <printf/printf.h>

#include <cstdint>
#include <memory>

#include "nv2astate.h"
#include "nxdk_ext.h"
#include "pushbuffer.h"
#include "string"
#include "texture_format.h"
#include "texture_stage.h"
#include "vertex_buffer.h"
#include "xbox_math_types.h"

using namespace XboxMath;
using namespace PBKitPlusPlus;

namespace PBKitPlusPlus {
class VertexShaderProgram;
struct Vertex;
class VertexBuffer;
}  // namespace PBKitPlusPlus

/**
 * Provides utility methods for use by TestSuite subclasses.
 */
class TestHost : public NV2AState {
 public:
  TestHost(std::shared_ptr<FTPLogger> ftp_logger, uint32_t framebuffer_width, uint32_t framebuffer_height,
           uint32_t max_texture_width, uint32_t max_texture_height, uint32_t max_texture_depth = 4);

  //! Marks drawing as completed, potentially causing artifacts (framebuffer, z/stencil-buffer) to be saved to disk.
  void FinishDraw(bool allow_saving, const std::string &output_directory, const std::string &suite_name,
                  const std::string &name, bool save_zbuffer = false);

  //! Returns the current override flag to allow/prevent artifact saving.
  [[nodiscard]] bool GetSaveResults() const { return save_results_; }
  //! Sets the override flag to prevent artifact saving during FinishDraw.
  void SetSaveResults(bool enable = true) { save_results_ = enable; }

  //! Saves the given texture to the filesystem as a PNG file.
  static std::string SaveTexture(const std::string &output_directory, const std::string &name, const uint8_t *texture,
                                 uint32_t width, uint32_t height, uint32_t pitch, uint32_t bits_per_pixel,
                                 SDL_PixelFormatEnum format);
  //! Saves the given region of memory as a flat binary file.
  static std::string SaveRawTexture(const std::string &output_directory, const std::string &name,
                                    const uint8_t *texture, uint32_t width, uint32_t height, uint32_t pitch,
                                    uint32_t bits_per_pixel);
  //! Saves the Z/Stencil buffer to the filesystem/
  [[nodiscard]] std::string SaveZBuffer(const std::string &output_directory, const std::string &name) const;

  //! Creates the given directory if it does not already exist.
  static void EnsureFolderExists(const std::string &folder_path);

  //! Returns an X coordinate sufficient to center a primitive with the given width within the framebuffer.
  inline float CenterX(float item_width, bool pixel_align = true) {
    float ret = (GetFramebufferWidthF() - item_width) * 0.5f;
    if (pixel_align) {
      return floorf(ret);
    }

    return ret;
  }

  //! Returns a Y coordinate sufficient to center a primitive with the given height within the framebuffer.
  inline float CenterY(float item_height, bool pixel_align = true) {
    float ret = (GetFramebufferHeightF() - item_height) * 0.5f;
    if (pixel_align) {
      return floorf(ret);
    }
    return ret;
  }

 private:
  static std::string PrepareSaveFile(std::string output_directory, const std::string &filename,
                                     const std::string &ext = ".png");
  static std::string SaveBackBuffer(const std::string &output_directory, const std::string &name);

 private:
  bool save_results_{true};

  std::shared_ptr<FTPLogger> ftp_logger_;
};

#endif  // NXDK_PGRAPH_TESTS_TEST_HOST_H
