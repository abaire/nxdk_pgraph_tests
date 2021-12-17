#ifndef NXDK_PGRAPH_TESTS_TEST_HOST_H
#define NXDK_PGRAPH_TESTS_TEST_HOST_H

#include <pbkit/pbkit.h>

#include <cstdint>
#include <memory>

#include "nxdk_ext.h"
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
  enum VertexFormatElements {
    POSITION = 1 << NV2A_VERTEX_ATTR_POSITION,
    TEXCOORD0 = 1 << NV2A_VERTEX_ATTR_TEXTURE0,
    NORMAL = 1 << NV2A_VERTEX_ATTR_NORMAL,
    DIFFUSE = 1 << NV2A_VERTEX_ATTR_DIFFUSE,
  };

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
  void DrawVertices(uint32_t elements = 0xFFFFFFFF);
  void FinishDraw() const;
  void FinishDrawAndSave(const char *output_directory, const char *name, const char *z_buffer_name = nullptr);

  void SetShaderProgram(std::shared_ptr<ShaderProgram> program);

  void SetTextureStageEnabled(uint32_t stage, bool enabled = true) {
    assert(stage < 4 && "Only 4 texture stages are supported.");
    assert(stage == 0 && "Only 1 texture stage is fully implemented.");
    texture_stage_enabled_[stage] = enabled;
  }

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

  bool texture_stage_enabled_[4]{true, false, false, false};

  uint32_t depth_buffer_format_{NV097_SET_SURFACE_FORMAT_ZETA_Z24S8};
  bool depth_buffer_mode_float_{false};
  std::shared_ptr<ShaderProgram> shader_program_{};

  std::shared_ptr<VertexBuffer> vertex_buffer_{};
  uint8_t *texture_memory_{nullptr};
};

#endif  // NXDK_PGRAPH_TESTS_TEST_HOST_H
