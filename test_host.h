#ifndef NXDK_PGRAPH_TESTS_TEST_HOST_H
#define NXDK_PGRAPH_TESTS_TEST_HOST_H

#include <pbkit/pbkit.h>

#include <cstdint>
#include <memory>

#include "texture_format.h"
#include "vertex_buffer.h"

class ShaderProgram;
struct Vertex;
class VertexBuffer;

// The first pgraph 0x3D subchannel that can be used by tests.
// It appears that this must be exactly one more than the last subchannel configured by pbkit or it will trigger an
// exception in xemu.
constexpr uint32_t kNextSubchannel = NEXT_SUBCH;
// The first pgraph context channel that can be used by tests.
constexpr uint32_t kNextContextChannel = 25;

class TestHost {
 public:
  TestHost(uint32_t framebuffer_width, uint32_t framebuffer_height, uint32_t texture_width, uint32_t texture_height);
  ~TestHost();

  void SetTextureFormat(const TextureFormatInfo &fmt);
  int SetTexture(SDL_Surface *gradient_surface);

  void SetDepthBufferFormat(uint32_t fmt);
  uint32_t GetDepthBufferFormat() const { return depth_buffer_format_; }

  void SetDepthBufferFloatMode(bool enabled);
  bool GetDepthBufferFloatMode() const { return depth_buffer_mode_float_; }

  uint32_t GetTextureWidth() const { return texture_width_; }
  uint32_t GetTextureHeight() const { return texture_height_; }

  uint32_t GetFramebufferWidth() const { return framebuffer_width_; }
  uint32_t GetFramebufferHeight() const { return framebuffer_height_; }

  std::shared_ptr<VertexBuffer> AllocateVertexBuffer(uint32_t num_vertices);
  std::shared_ptr<VertexBuffer> GetVertexBuffer() { return vertex_buffer_; }

  void Clear(uint32_t argb = 0xFF000000, uint32_t depth_value = 0xFF000000, uint8_t stencil_value = 0x00) const;
  void SetDepthStencilRegion(uint32_t depth_value, uint8_t stencil_value, uint32_t left = 0, uint32_t top = 0,
                             uint32_t width = 0, uint32_t height = 0) const;
  void SetFillColorRegion(uint32_t argb, uint32_t left = 0, uint32_t top = 0, uint32_t width = 0,
                          uint32_t height = 0) const;
  static void EraseText();

  void PrepareDraw(uint32_t argb = 0xFF000000, uint32_t depth_value = 0xFF000000, uint8_t stencil_value = 0x00);
  void DrawVertices();
  void FinishDraw() const;
  void FinishDrawAndSave(const char *output_directory, const char *name, const char *z_buffer_name = nullptr);

  void SetShaderProgram(std::shared_ptr<ShaderProgram> program);

 private:
  void SetupControl0() const;
  void SetupTextureStages() const;
  static void SaveBackbuffer(const char *output_directory, const char *name);
  void SaveZBuffer(const char *output_directory, const char *name) const;

 private:
  uint32_t framebuffer_width_;
  uint32_t framebuffer_height_;

  TextureFormatInfo texture_format_{};
  uint32_t texture_width_;
  uint32_t texture_height_;

  uint32_t depth_buffer_format_{NV097_SET_SURFACE_FORMAT_ZETA_Z24S8};
  bool depth_buffer_mode_float_{false};
  std::shared_ptr<ShaderProgram> shader_program_{};

  std::shared_ptr<VertexBuffer> vertex_buffer_{};
  uint8_t *texture_memory_{nullptr};
};

#endif  // NXDK_PGRAPH_TESTS_TEST_HOST_H
