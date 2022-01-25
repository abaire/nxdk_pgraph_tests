#ifndef NXDK_PGRAPH_TESTS_TEST_HOST_H
#define NXDK_PGRAPH_TESTS_TEST_HOST_H

#include <pbkit/pbkit.h>
#include <printf/printf.h>

#include <cstdint>
#include <memory>

#include "math3d.h"
#include "nxdk_ext.h"
#include "string"
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
  enum VertexAttribute {
    POSITION = 1 << NV2A_VERTEX_ATTR_POSITION,
    WEIGHT = 1 << NV2A_VERTEX_ATTR_WEIGHT,
    NORMAL = 1 << NV2A_VERTEX_ATTR_NORMAL,
    DIFFUSE = 1 << NV2A_VERTEX_ATTR_DIFFUSE,
    SPECULAR = 1 << NV2A_VERTEX_ATTR_SPECULAR,
    FOG_COORD = 1 << NV2A_VERTEX_ATTR_FOG_COORD,
    POINT_SIZE = 1 << NV2A_VERTEX_ATTR_POINT_SIZE,
    BACK_DIFFUSE = 1 << NV2A_VERTEX_ATTR_BACK_DIFFUSE,
    BACK_SPECULAR = 1 << NV2A_VERTEX_ATTR_BACK_SPECULAR,
    TEXCOORD0 = 1 << NV2A_VERTEX_ATTR_TEXTURE0,
    TEXCOORD1 = 1 << NV2A_VERTEX_ATTR_TEXTURE1,
    TEXCOORD2 = 1 << NV2A_VERTEX_ATTR_TEXTURE2,
    TEXCOORD3 = 1 << NV2A_VERTEX_ATTR_TEXTURE3,
  };

  static constexpr uint32_t kDefaultVertexFields = POSITION | DIFFUSE | TEXCOORD0;

  enum DrawPrimitive {
    PRIMITIVE_POINTS = NV097_SET_BEGIN_END_OP_POINTS,
    PRIMITIVE_LINES = NV097_SET_BEGIN_END_OP_LINES,
    PRIMITIVE_LINE_LOOP = NV097_SET_BEGIN_END_OP_LINE_LOOP,
    PRIMITIVE_LINE_STRIP = NV097_SET_BEGIN_END_OP_LINE_STRIP,
    PRIMITIVE_TRIANGLES = NV097_SET_BEGIN_END_OP_TRIANGLES,
    PRIMITIVE_TRIANGLE_STRIP = NV097_SET_BEGIN_END_OP_TRIANGLE_STRIP,
    PRIMITIVE_TRIANGLE_FAN = NV097_SET_BEGIN_END_OP_TRIANGLE_FAN,
    PRIMITIVE_QUADS = NV097_SET_BEGIN_END_OP_QUADS,
    PRIMITIVE_QUAD_STRIP = NV097_SET_BEGIN_END_OP_QUAD_STRIP,
    PRIMITIVE_POLYGON = NV097_SET_BEGIN_END_OP_POLYGON,
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
  void SetVertexBuffer(std::shared_ptr<VertexBuffer> buffer);
  std::shared_ptr<VertexBuffer> GetVertexBuffer() { return vertex_buffer_; }

  void Clear(uint32_t argb = 0xFF000000, uint32_t depth_value = 0xFFFFFFFF, uint8_t stencil_value = 0x00) const;
  void SetDepthStencilRegion(uint32_t depth_value, uint8_t stencil_value, uint32_t left = 0, uint32_t top = 0,
                             uint32_t width = 0, uint32_t height = 0) const;
  void SetFillColorRegion(uint32_t argb, uint32_t left = 0, uint32_t top = 0, uint32_t width = 0,
                          uint32_t height = 0) const;
  static void EraseText();

  void PrepareDraw(uint32_t argb = 0xFF000000, uint32_t depth_value = 0xFFFFFFFF, uint8_t stencil_value = 0x00);

  void DrawArrays(uint32_t enabled_vertex_fields = kDefaultVertexFields, DrawPrimitive primitive = PRIMITIVE_TRIANGLES);
  void DrawInlineBuffer(uint32_t enabled_vertex_fields = kDefaultVertexFields,
                        DrawPrimitive primitive = PRIMITIVE_TRIANGLES);

  // Sends vertices as an interleaved array of vertex fields. E.g., [POS_0,DIFFUSE_0,POS_1,DIFFUSE_1,...]
  void DrawInlineArray(uint32_t enabled_vertex_fields = kDefaultVertexFields,
                       DrawPrimitive primitive = PRIMITIVE_TRIANGLES);

  // Sends vertices via an index array. Index values must be < 0xFFFF and are sent two per command.
  void DrawInlineElements16(const std::vector<uint32_t> &indices, uint32_t enabled_vertex_fields = kDefaultVertexFields,
                            DrawPrimitive primitive = PRIMITIVE_TRIANGLES);

  // Sends vertices via an index array. Index values are unsigned integers.
  void DrawInlineElements32(const std::vector<uint32_t> &indices, uint32_t enabled_vertex_fields = kDefaultVertexFields,
                            DrawPrimitive primitive = PRIMITIVE_TRIANGLES);

  void FinishDraw(bool allow_saving, const std::string &output_directory, const std::string &name,
                  const std::string &z_buffer_name = "");

  void SetShaderProgram(std::shared_ptr<ShaderProgram> program);
  std::shared_ptr<ShaderProgram> GetShaderProgram() const { return shader_program_; }

  void SetTextureStageEnabled(uint32_t stage, bool enabled = true);

  // Set up the viewport and fixed function pipeline matrices to match a default XDK project.
  void SetXDKDefaultViewportAndFixedFunctionMatrices();

  // Set up the viewport and fixed function pipeline matrices to match the nxdk settings.
  void SetDefaultViewportAndFixedFunctionMatrices();

  void SetViewportOffset(float x, float y, float z, float w) const;
  void SetViewportScale(float x, float y, float z, float w) const;

  void SetFixedFunctionModelViewMatrix(const MATRIX model_matrix);
  void SetFixedFunctionProjectionMatrix(const MATRIX projection_matrix);

  void SetVertex(float x, float y, float z) const;
  void SetVertex(float x, float y, float z, float w) const;
  void SetWeight(float w) const;
  void SetWeight(float w1, float w2, float w3, float w4) const;
  void SetNormal(float x, float y, float z) const;
  void SetNormal3S(int x, int y, int z) const;
  void SetDiffuse(float r, float g, float b, float a) const;
  void SetDiffuse(float r, float g, float b) const;
  void SetDiffuse(uint32_t rgba) const;
  void SetSpecular(float r, float g, float b, float a) const;
  void SetSpecular(float r, float g, float b) const;
  void SetSpecular(uint32_t rgba) const;
  void SetFogCoord(float fc) const;
  void SetPointSize(float ps) const;
  void SetTexCoord0(float u, float v) const;
  void SetTexCoord0i(int u, int v) const;
  void SetTexCoord0(float s, float t, float p, float q) const;
  void SetTexCoord0i(int s, int t, int p, int q) const;
  void SetTexCoord1(float u, float v) const;
  void SetTexCoord1i(int u, int v) const;
  void SetTexCoord1(float s, float t, float p, float q) const;
  void SetTexCoord1i(int s, int t, int p, int q) const;
  void SetTexCoord2(float u, float v) const;
  void SetTexCoord2i(int u, int v) const;
  void SetTexCoord2(float s, float t, float p, float q) const;
  void SetTexCoord2i(int s, int t, int p, int q) const;
  void SetTexCoord3(float u, float v) const;
  void SetTexCoord3i(int u, int v) const;
  void SetTexCoord3(float s, float t, float p, float q) const;
  void SetTexCoord3i(int s, int t, int p, int q) const;

  static std::string GetPrimitiveName(DrawPrimitive primitive);

  bool GetSaveResults() const { return save_results_; }
  void SetSaveResults(bool enable = true) { save_results_ = enable; }

  void SetAlphaBlendEnabled(bool enable = true) const;

  void SetVertexBufferAttributes(uint32_t enabled_fields);

 private:
  void SetupControl0() const;
  void SetupTextureStages() const;
  static void EnsureFolderExists(const std::string &folder_path);
  static std::string PrepareSaveFilePNG(std::string output_directory, const std::string &filename);
  static void SaveBackBuffer(const std::string &output_directory, const std::string &name);
  void SaveZBuffer(const std::string &output_directory, const std::string &name) const;

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

  enum FixedFunctionMatrixSetting {
    MATRIX_MODE_DEFAULT_NXDK,
    MATRIX_MODE_DEFAULT_XDK,
    MATRIX_MODE_USER,
  };
  FixedFunctionMatrixSetting fixed_function_matrix_mode_{MATRIX_MODE_DEFAULT_NXDK};
  MATRIX fixed_function_model_view_matrix_{};
  MATRIX fixed_function_projection_matrix_{};

  bool save_results_{true};
};

#endif  // NXDK_PGRAPH_TESTS_TEST_HOST_H
