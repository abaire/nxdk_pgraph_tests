#ifndef NXDK_PGRAPH_TESTS_TEST_HOST_H
#define NXDK_PGRAPH_TESTS_TEST_HOST_H

#include <ftp_logger.h>
#include <pbkit/pbkit.h>
#include <printf/printf.h>

#include <cstdint>
#include <memory>

#include "nxdk_ext.h"
#include "string"
#include "texture_format.h"
#include "texture_stage.h"
#include "vertex_buffer.h"
#include "xbox_math_types.h"

using namespace XboxMath;

class VertexShaderProgram;
struct Vertex;
class VertexBuffer;

//! The first pgraph 0x3D subchannel that can be used by tests.
//! It appears that this must be exactly one more than the last subchannel configured by pbkit or else it will trigger
//! an exception in xemu.
constexpr uint32_t kNextSubchannel = NEXT_SUBCH;
//! The first pgraph context channel that can be used by tests.
constexpr int32_t kNextContextChannel = 25;

constexpr uint32_t kNoStrideOverride = 0xFFFFFFFF;

//! Maximum address of VRAM
#define VRAM_MAX 0x07FFFFFF
//! Converts a memory address to a VRAM address accessible by the GPU.
#define VRAM_ADDR(x) (reinterpret_cast<uint32_t>(x) & 0x03FFFFFF)
#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

/**
 * Provides utility methods for use by TestSuite subclasses.
 */
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
    V13 = 1 << NV2A_VERTEX_ATTR_13,
    V14 = 1 << NV2A_VERTEX_ATTR_14,
    V15 = 1 << NV2A_VERTEX_ATTR_15,
  };

  static constexpr uint32_t kDefaultVertexFields = POSITION | DIFFUSE | TEXCOORD0;

  //! Enumerates the rendering primitives supported by the NV2A.
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

  enum CombinerSource {
    SRC_ZERO = 0,     // 0
    SRC_C0,           // Constant[0]
    SRC_C1,           // Constant[1]
    SRC_FOG,          // Fog coordinate
    SRC_DIFFUSE,      // Vertex diffuse
    SRC_SPECULAR,     // Vertex specular
    SRC_6,            // ?
    SRC_7,            // ?
    SRC_TEX0,         // Texcoord0
    SRC_TEX1,         // Texcoord1
    SRC_TEX2,         // Texcoord2
    SRC_TEX3,         // Texcoord3
    SRC_R0,           // R0 from the vertex shader
    SRC_R1,           // R1 from the vertex shader
    SRC_SPEC_R0_SUM,  // Specular + R0
    SRC_EF_PROD,      // Combiner param E * F
  };

  enum CombinerDest {
    DST_DISCARD = 0,  // Discard the calculation
    DST_C0,           // Constant[0]
    DST_C1,           // Constant[1]
    DST_FOG,          // Fog coordinate
    DST_DIFFUSE,      // Vertex diffuse
    DST_SPECULAR,     // Vertex specular
    DST_6,            // ?
    DST_7,            // ?
    DST_TEX0,         // Texcoord0
    DST_TEX1,         // Texcoord1
    DST_TEX2,         // Texcoord2
    DST_TEX3,         // Texcoord3
    DST_R0,           // R0 from the vertex shader
    DST_R1,           // R1 from the vertex shader
    DST_SPEC_R0_SUM,  // Specular + R1
    DST_EF_PROD,      // Combiner param E * F
  };

  enum CombinerSumMuxMode {
    SM_SUM = 0,  // ab + cd
    SM_MUX = 1,  // r0.a is used to select cd or ab
  };

  enum CombinerOutOp {
    OP_IDENTITY = 0,           // y = x
    OP_BIAS = 1,               // y = x - 0.5
    OP_SHIFT_LEFT_1 = 2,       // y = x*2
    OP_SHIFT_LEFT_1_BIAS = 3,  // y = (x - 0.5)*2
    OP_SHIFT_LEFT_2 = 4,       // y = x*4
    OP_SHIFT_RIGHT_1 = 6,      // y = x/2
  };

  enum CombinerMapping {
    MAP_UNSIGNED_IDENTITY,  // max(0,x)         OK for final combiner
    MAP_UNSIGNED_INVERT,    // 1 - max(0,x)     OK for final combiner
    MAP_EXPAND_NORMAL,      // 2*max(0,x) - 1   invalid for final combiner
    MAP_EXPAND_NEGATE,      // 1 - 2*max(0,x)   invalid for final combiner
    MAP_HALFBIAS_NORMAL,    // max(0,x) - 1/2   invalid for final combiner
    MAP_HALFBIAS_NEGATE,    // 1/2 - max(0,x)   invalid for final combiner
    MAP_SIGNED_IDENTITY,    // x                invalid for final combiner
    MAP_SIGNED_NEGATE,      // -x               invalid for final combiner
  };

  struct CombinerInput {
    CombinerSource source;
    bool alpha;
    CombinerMapping mapping;
  };

  struct ColorInput : public CombinerInput {
    explicit ColorInput(CombinerSource s, CombinerMapping m = MAP_UNSIGNED_IDENTITY) : CombinerInput() {
      source = s;
      alpha = false;
      mapping = m;
    }
  };

  struct AlphaInput : public CombinerInput {
    explicit AlphaInput(CombinerSource s, CombinerMapping m = MAP_UNSIGNED_IDENTITY) : CombinerInput() {
      source = s;
      alpha = true;
      mapping = m;
    }
  };

  struct ZeroInput : public CombinerInput {
    explicit ZeroInput() : CombinerInput() {
      source = SRC_ZERO;
      alpha = false;
      mapping = MAP_UNSIGNED_IDENTITY;
    }
  };

  struct NegativeOneInput : public CombinerInput {
    explicit NegativeOneInput() : CombinerInput() {
      source = SRC_ZERO;
      alpha = false;
      mapping = MAP_EXPAND_NORMAL;
    }
  };

  struct OneInput : public CombinerInput {
    explicit OneInput() : CombinerInput() {
      source = SRC_ZERO;
      alpha = false;
      mapping = MAP_UNSIGNED_INVERT;
    }
  };

  //! Palette size for palettized surfaces.
  enum PaletteSize {
    PALETTE_32 = 32,
    PALETTE_64 = 64,
    PALETTE_128 = 128,
    PALETTE_256 = 256,
  };

  //! Texture unit modes.
  enum ShaderStageProgram {
    STAGE_NONE = 0,
    STAGE_2D_PROJECTIVE,
    STAGE_3D_PROJECTIVE,
    STAGE_CUBE_MAP,
    STAGE_PASS_THROUGH,
    STAGE_CLIP_PLANE,
    STAGE_BUMPENVMAP,
    STAGE_BUMPENVMAP_LUMINANCE,
    STAGE_BRDF,
    STAGE_DOT_ST,
    STAGE_DOT_ZW,
    STAGE_DOT_REFLECT_DIFFUSE,
    STAGE_DOT_REFLECT_SPECULAR,
    STAGE_DOT_STR_3D,
    STAGE_DOT_STR_CUBE,
    STAGE_DEPENDENT_AR,
    STAGE_DEPENDENT_GB,
    STAGE_DOT_PRODUCT,
    STAGE_DOT_REFLECT_SPECULAR_CONST,
  };

  //! Antialiasing settings for surfaces.
  enum AntiAliasingSetting {
    AA_CENTER_1 = NV097_SET_SURFACE_FORMAT_ANTI_ALIASING_CENTER_1,
    AA_CENTER_CORNER_2 = NV097_SET_SURFACE_FORMAT_ANTI_ALIASING_CENTER_CORNER_2,
    AA_SQUARE_OFFSET_4 = NV097_SET_SURFACE_FORMAT_ANTI_ALIASING_SQUARE_OFFSET_4,
  };

  //! Color formats for surfaces.
  enum SurfaceColorFormat {
    SCF_X1R5G5B5_Z1R5G5B5 = NV097_SET_SURFACE_FORMAT_COLOR_LE_X1R5G5B5_Z1R5G5B5,
    SCF_X1R5G5B5_O1R5G5B5 = NV097_SET_SURFACE_FORMAT_COLOR_LE_X1R5G5B5_O1R5G5B5,
    SCF_R5G6B5 = NV097_SET_SURFACE_FORMAT_COLOR_LE_R5G6B5,
    SCF_X8R8G8B8_Z8R8G8B8 = NV097_SET_SURFACE_FORMAT_COLOR_LE_X8R8G8B8_Z8R8G8B8,
    SCF_X8R8G8B8_O8R8G8B8 = NV097_SET_SURFACE_FORMAT_COLOR_LE_X8R8G8B8_O8R8G8B8,
    SCF_X1A7R8G8B8_Z1A7R8G8B8 = NV097_SET_SURFACE_FORMAT_COLOR_LE_X1A7R8G8B8_Z1A7R8G8B8,
    SCF_X1A7R8G8B8_O1A7R8G8B8 = NV097_SET_SURFACE_FORMAT_COLOR_LE_X1A7R8G8B8_O1A7R8G8B8,
    SCF_A8R8G8B8 = NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8,
    SCF_B8 = NV097_SET_SURFACE_FORMAT_COLOR_LE_B8,
    SCF_G8B8 = NV097_SET_SURFACE_FORMAT_COLOR_LE_G8B8,
  };

  //! Depth buffer formats for surfaces.
  enum SurfaceZetaFormat {
    SZF_Z16 = NV097_SET_SURFACE_FORMAT_ZETA_Z16,
    SZF_Z24S8 = NV097_SET_SURFACE_FORMAT_ZETA_Z24S8
  };

 public:
  TestHost(std::shared_ptr<FTPLogger> ftp_logger, uint32_t framebuffer_width, uint32_t framebuffer_height,
           uint32_t max_texture_width, uint32_t max_texture_height, uint32_t max_texture_depth = 4);
  ~TestHost();

  TextureStage &GetTextureStage(uint32_t stage) { return texture_stage_[stage]; }
  void SetTextureFormat(const TextureFormatInfo &fmt, uint32_t stage = 0);
  void SetDefaultTextureParams(uint32_t stage = 0);
  int SetTexture(SDL_Surface *surface, uint32_t stage = 0);
  int SetVolumetricTexture(const SDL_Surface **surface, uint32_t depth, uint32_t stage = 0);
  int SetRawTexture(const uint8_t *source, uint32_t width, uint32_t height, uint32_t depth, uint32_t pitch,
                    uint32_t bytes_per_pixel, bool swizzle, uint32_t stage = 0);

  int SetPalette(const uint32_t *palette, PaletteSize size, uint32_t stage = 0);
  void SetPaletteSize(PaletteSize size, uint32_t stage = 0);
  void SetTextureStageEnabled(uint32_t stage, bool enabled = true);

  // Set the surface format
  // width and height are treated differently depending on whether swizzle is enabled or not.
  // swizzle = true
  //     width and height must be a power of two
  // swizzle = false
  //     width and height may be arbitrary positive values and will be used to set the clip dimensions
  //
  // Does not take effect until CommitSurfaceFormat is called.
  void SetSurfaceFormat(SurfaceColorFormat color_format, SurfaceZetaFormat depth_format, uint32_t width,
                        uint32_t height, bool swizzle = false, uint32_t clip_x = 0, uint32_t clip_y = 0,
                        uint32_t clip_width = 0, uint32_t clip_height = 0, AntiAliasingSetting aa = AA_CENTER_1);

  // Sets the surface format and commits it immediately.
  void SetSurfaceFormatImmediate(SurfaceColorFormat color_format, SurfaceZetaFormat depth_format, uint32_t width,
                                 uint32_t height, bool swizzle = false, uint32_t clip_x = 0, uint32_t clip_y = 0,
                                 uint32_t clip_width = 0, uint32_t clip_height = 0,
                                 AntiAliasingSetting aa = AA_CENTER_1);

  //! Returns the current surface color format.
  [[nodiscard]] SurfaceColorFormat GetColorBufferFormat() const { return surface_color_format_; }

  //! Returns the current depth buffer format.
  [[nodiscard]] SurfaceZetaFormat GetDepthBufferFormat() const { return depth_buffer_format_; }

  //! Changes the current depth buffer mode into float (true) or fixed integer (false).
  void SetDepthBufferFloatMode(bool enabled);
  //! Returns true if the current depth buffer mode is floating point.
  [[nodiscard]] bool GetDepthBufferFloatMode() const { return depth_buffer_mode_float_; }

  void CommitSurfaceFormat() const;

  //! Returns the maximum width for a texture.
  [[nodiscard]] uint32_t GetMaxTextureWidth() const { return max_texture_width_; }
  //! Returns the maximum height for a texture.
  [[nodiscard]] uint32_t GetMaxTextureHeight() const { return max_texture_height_; }
  //! Returns the maximum depth for a texture.
  [[nodiscard]] uint32_t GetMaxTextureDepth() const { return max_texture_depth_; }
  //! Returns the maximum size in bytes that any single texture can be.
  [[nodiscard]] uint32_t GetMaxSingleTextureSize() const { return max_single_texture_size_; }

  //! Returns the base of texture memory.
  [[nodiscard]] uint8_t *GetTextureMemory() const { return texture_memory_; }
  //! Returns the allocated size of the entire texture memory region.
  [[nodiscard]] uint32_t GetTextureMemorySize() const { return texture_memory_size_; }

  //! Returns the base of texture memory used by the given texture unit (stage).
  [[nodiscard]] uint8_t *GetTextureMemoryForStage(uint32_t stage) const {
    return texture_memory_ + texture_stage_[stage].GetTextureOffset();
  }
  //! Returns the base of the palette memory for an indexed texture used by the given texture unit.
  [[nodiscard]] uint32_t *GetPaletteMemoryForStage(uint32_t stage) const {
    return reinterpret_cast<uint32_t *>(texture_palette_memory_ + texture_stage_[stage].GetPaletteOffset());
  }

  //! Returns the width of the screen, in pixels.
  [[nodiscard]] inline uint32_t GetFramebufferWidth() const { return framebuffer_width_; }
  //! Returns the height of the screen, in pixels.
  [[nodiscard]] inline uint32_t GetFramebufferHeight() const { return framebuffer_height_; }
  //! Returns the width of the screen in pixels as a float.
  [[nodiscard]] inline float GetFramebufferWidthF() const { return static_cast<float>(framebuffer_width_); }
  //! Returns the height of the screen in pixels as a float.
  [[nodiscard]] inline float GetFramebufferHeightF() const { return static_cast<float>(framebuffer_height_); }

  //! Allocates a VertexBuffer large enough to hold the given number of vertices.
  std::shared_ptr<VertexBuffer> AllocateVertexBuffer(uint32_t num_vertices);
  //! Sets the active vertex buffer.
  void SetVertexBuffer(std::shared_ptr<VertexBuffer> buffer);
  //! Returns the active vertex buffer.
  std::shared_ptr<VertexBuffer> GetVertexBuffer() { return vertex_buffer_; }

  //! Clears the active surface and pb_text overlay.
  void Clear(uint32_t argb = 0xFF000000, uint32_t depth_value = 0xFFFFFFFF, uint8_t stencil_value = 0x00) const;
  //! Sets the contents of a rect within the depth/stencil buffer.
  void ClearDepthStencilRegion(uint32_t depth_value, uint8_t stencil_value, uint32_t left = 0, uint32_t top = 0,
                               uint32_t width = 0, uint32_t height = 0) const;
  //! Sets the color of a rect within the active surface.
  void ClearColorRegion(uint32_t argb, uint32_t left = 0, uint32_t top = 0, uint32_t width = 0,
                        uint32_t height = 0) const;
  //! Erases the pb_text overlay.
  static void EraseText();

  //! Note: A number of states are expected to be set before this method is called.
  //! E.g., texture stages, shader states
  //! This is not an exhaustive list and is not necessarily up to date. Prefer to call this just before initiating draw
  //! and be suspect of order dependence if you see results that seem to indicate that settings are being ignored.
  void PrepareDraw(uint32_t argb = 0xFF000000, uint32_t depth_value = 0xFFFFFFFF, uint8_t stencil_value = 0x00);

  void DrawArrays(uint32_t enabled_vertex_fields = kDefaultVertexFields, DrawPrimitive primitive = PRIMITIVE_TRIANGLES);
  void DrawInlineBuffer(uint32_t enabled_vertex_fields = kDefaultVertexFields,
                        DrawPrimitive primitive = PRIMITIVE_TRIANGLES);

  //! Sends vertices as an interleaved array of vertex fields. E.g., [POS_0,DIFFUSE_0,POS_1,DIFFUSE_1,...]
  void DrawInlineArray(uint32_t enabled_vertex_fields = kDefaultVertexFields,
                       DrawPrimitive primitive = PRIMITIVE_TRIANGLES);

  //! Sends vertices via an index array. Index values must be < 0xFFFF and are sent two per command.
  void DrawInlineElements16(const std::vector<uint32_t> &indices, uint32_t enabled_vertex_fields = kDefaultVertexFields,
                            DrawPrimitive primitive = PRIMITIVE_TRIANGLES);

  //! Sends vertices via an index array. Index values are unsigned integers.
  void DrawInlineElements32(const std::vector<uint32_t> &indices, uint32_t enabled_vertex_fields = kDefaultVertexFields,
                            DrawPrimitive primitive = PRIMITIVE_TRIANGLES);

  //! Marks drawing as completed, potentially causing artifacts (framebuffer, z/stencil-buffer) to be saved to disk.
  void FinishDraw(bool allow_saving, const std::string &output_directory, const std::string &suite_name,
                  const std::string &name, bool save_zbuffer = false);

  //! Sets the z clipping range.
  void SetDepthClip(float min, float max) const;

  //! Sets the active vertex shader. Pass `nullptr` to use the fixed function pipeline.
  void SetVertexShaderProgram(std::shared_ptr<VertexShaderProgram> program);
  //! Returns the active vertex shader.
  [[nodiscard]] std::shared_ptr<VertexShaderProgram> GetShaderProgram() const { return vertex_shader_program_; }

  //! Generates a Direct3D-style model view matrix.
  static void BuildD3DModelViewMatrix(matrix4_t &matrix, const vector_t &eye, const vector_t &at, const vector_t &up);

  //! Gets a Direct3D-style matrix suitable for a projection + viewport transform.
  void BuildD3DProjectionViewportMatrix(matrix4_t &result, float fov, float z_near, float z_far) const;

  //! Builds an orthographic projection matrix.
  static void BuildD3DOrthographicProjectionMatrix(matrix4_t &result, float left, float right, float top, float bottom,
                                                   float z_near, float z_far);

  //! Gets a reasonable default model view matrix (camera at z=-7.0f looking at the origin) similar to the one used by
  //! the XDK.
  static void BuildDefaultXDKModelViewMatrix(matrix4_t &matrix);
  //! Gets a reasonable default projection matrix (fov = PI/4, near = 1, far = 200) similar to the one used by the XDK.
  void BuildDefaultXDKProjectionMatrix(matrix4_t &matrix) const;

  //! Returns the inverse composite matrix for the fixed function pipeline.
  [[nodiscard]] const matrix4_t &GetFixedFunctionInverseCompositeMatrix() const {
    return fixed_function_inverse_composite_matrix_;
  }

  //! Set up the viewport and fixed function pipeline matrices to match a default XDK project.
  void SetXDKDefaultViewportAndFixedFunctionMatrices();

  //! Set up the viewport and fixed function pipeline matrices to match the nxdk settings.
  void SetDefaultViewportAndFixedFunctionMatrices();

  //! Projects the given point (on the CPU), placing the resulting screen coordinates into `result`.
  void ProjectPoint(vector_t &result, const vector_t &world_point) const;

  //! Unprojects a point in screenspace into 3D worldspace.
  void UnprojectPoint(vector_t &result, const vector_t &screen_point) const;
  //! Unprojects a point in screenspace into 3D worldspace, setting Z to the given value.
  void UnprojectPoint(vector_t &result, const vector_t &screen_point, float world_z) const;

  //! Toggles whether window clipping considers the rect as inclusive or exclusive.
  static void SetWindowClipExclusive(bool exclusive);
  //! Sets the window clipping region.
  static void SetWindowClip(uint32_t right, uint32_t bottom, uint32_t left = 0, uint32_t top = 0, uint32_t region = 0);

  static void SetViewportOffset(float x, float y, float z, float w);
  static void SetViewportScale(float x, float y, float z, float w);

  void SetFixedFunctionModelViewMatrix(const matrix4_t model_matrix);
  void SetFixedFunctionProjectionMatrix(const matrix4_t projection_matrix);
  [[nodiscard]] inline const matrix4_t &GetFixedFunctionModelViewMatrix() const {
    return fixed_function_model_view_matrix_;
  }
  [[nodiscard]] inline const matrix4_t &GetFixedFunctionProjectionMatrix() const {
    return fixed_function_projection_matrix_;
  }

  [[nodiscard]] float GetWNear() const { return w_near_; }
  [[nodiscard]] float GetWFar() const { return w_far_; }

  //! Start the process of rendering an inline-defined primitive (specified via SetXXXX methods below).
  //! Note that End() must be called to trigger rendering, and that SetVertex() triggers the creation of a vertex.
  void Begin(DrawPrimitive primitive) const;
  //! Triggers the rendering of the primitive specified by the previous call to Begin.
  void End() const;

  //! Trigger creation of a vertex, applying the last set attributes.
  void SetVertex(float x, float y, float z) const;
  //! Trigger creation of a vertex, applying the last set attributes.
  void SetVertex(float x, float y, float z, float w) const;
  //! Trigger creation of a vertex, applying the last set attributes.
  inline void SetVertex(const vector_t pt) const { SetVertex(pt[0], pt[1], pt[2], pt[3]); }

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
  void SetTexCoord0S(int u, int v) const;
  void SetTexCoord0(float s, float t, float p, float q) const;
  void SetTexCoord0S(int s, int t, int p, int q) const;
  void SetTexCoord1(float u, float v) const;
  void SetTexCoord1S(int u, int v) const;
  void SetTexCoord1(float s, float t, float p, float q) const;
  void SetTexCoord1S(int s, int t, int p, int q) const;
  void SetTexCoord2(float u, float v) const;
  void SetTexCoord2S(int u, int v) const;
  void SetTexCoord2(float s, float t, float p, float q) const;
  void SetTexCoord2S(int s, int t, int p, int q) const;
  void SetTexCoord3(float u, float v) const;
  void SetTexCoord3S(int u, int v) const;
  void SetTexCoord3(float s, float t, float p, float q) const;
  void SetTexCoord3S(int s, int t, int p, int q) const;

  //! Returns a human-friendly name for the given DrawPrimitive.
  static std::string GetPrimitiveName(DrawPrimitive primitive);

  //! Returns the current override flag to allow/prevent artifact saving.
  [[nodiscard]] bool GetSaveResults() const { return save_results_; }
  //! Sets the override flag to prevent artifact saving during FinishDraw.
  void SetSaveResults(bool enable = true) { save_results_ = enable; }

  //! Sets the mask used to enable modification of various color channels during rendering.
  void SetColorMask(uint32_t mask = NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE | NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
                                    NV097_SET_COLOR_MASK_RED_WRITE_ENABLE |
                                    NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE) const;

  //! Determines how pixels should be blended with existing pixels during rendering operations.
  void SetBlend(bool enable = true, uint32_t func = NV097_SET_BLEND_EQUATION_V_FUNC_ADD,
                uint32_t sfactor = NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA,
                uint32_t dfactor = NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA) const;

  //! Sets the blend color (and alpha) used by the V_CONSTANT_COLOR and V_CONSTANT_ALPHA blend factors.
  void SetBlendColorConstant(uint32_t color) const;

  //! Sets the alpha reference value (NV097_SET_ALPHA_REF) used by NV097_SET_ALPHA_FUNC.
  void SetAlphaReference(uint32_t color) const;

  //! Sets the alpha function used to mask writes based on alpha values.
  void SetAlphaFunc(bool enable = true, uint32_t func = NV097_SET_ALPHA_FUNC_V_ALWAYS) const;

  //! Sets up the number of enabled color combiners and behavior flags.
  //!
  //! same_factor0 == true will reuse the C0 constant across all enabled stages.
  //! same_factor1 == true will reuse the C1 constant across all enabled stages.
  void SetCombinerControl(int num_combiners = 1, bool same_factor0 = false, bool same_factor1 = false,
                          bool mux_msb = false) const;

  void SetInputColorCombiner(int combiner, CombinerInput a, CombinerInput b = ZeroInput(),
                             CombinerInput c = ZeroInput(), CombinerInput d = ZeroInput()) const {
    SetInputColorCombiner(combiner, a.source, a.alpha, a.mapping, b.source, b.alpha, b.mapping, c.source, c.alpha,
                          c.mapping, d.source, d.alpha, d.mapping);
  }
  void SetInputColorCombiner(int combiner, CombinerSource a_source = SRC_ZERO, bool a_alpha = false,
                             CombinerMapping a_mapping = MAP_UNSIGNED_IDENTITY, CombinerSource b_source = SRC_ZERO,
                             bool b_alpha = false, CombinerMapping b_mapping = MAP_UNSIGNED_IDENTITY,
                             CombinerSource c_source = SRC_ZERO, bool c_alpha = false,
                             CombinerMapping c_mapping = MAP_UNSIGNED_IDENTITY, CombinerSource d_source = SRC_ZERO,
                             bool d_alpha = false, CombinerMapping d_mapping = MAP_UNSIGNED_IDENTITY) const;
  void ClearInputColorCombiner(int combiner) const;
  void ClearInputColorCombiners() const;

  void SetInputAlphaCombiner(int combiner, CombinerInput a, CombinerInput b = ZeroInput(),
                             CombinerInput c = ZeroInput(), CombinerInput d = ZeroInput()) const {
    SetInputAlphaCombiner(combiner, a.source, a.alpha, a.mapping, b.source, b.alpha, b.mapping, c.source, c.alpha,
                          c.mapping, d.source, d.alpha, d.mapping);
  }
  void SetInputAlphaCombiner(int combiner, CombinerSource a_source = SRC_ZERO, bool a_alpha = false,
                             CombinerMapping a_mapping = MAP_UNSIGNED_IDENTITY, CombinerSource b_source = SRC_ZERO,
                             bool b_alpha = false, CombinerMapping b_mapping = MAP_UNSIGNED_IDENTITY,
                             CombinerSource c_source = SRC_ZERO, bool c_alpha = false,
                             CombinerMapping c_mapping = MAP_UNSIGNED_IDENTITY, CombinerSource d_source = SRC_ZERO,
                             bool d_alpha = false, CombinerMapping d_mapping = MAP_UNSIGNED_IDENTITY) const;
  void ClearInputAlphaColorCombiner(int combiner) const;
  void ClearInputAlphaCombiners() const;

  void SetOutputColorCombiner(int combiner, CombinerDest ab_dst = DST_DISCARD, CombinerDest cd_dst = DST_DISCARD,
                              CombinerDest sum_dst = DST_DISCARD, bool ab_dot_product = false,
                              bool cd_dot_product = false, CombinerSumMuxMode sum_or_mux = SM_SUM,
                              CombinerOutOp op = OP_IDENTITY, bool alpha_from_ab_blue = false,
                              bool alpha_from_cd_blue = false) const;
  void ClearOutputColorCombiner(int combiner) const;
  void ClearOutputColorCombiners() const;

  void SetOutputAlphaCombiner(int combiner, CombinerDest ab_dst = DST_DISCARD, CombinerDest cd_dst = DST_DISCARD,
                              CombinerDest sum_dst = DST_DISCARD, bool ab_dot_product = false,
                              bool cd_dot_product = false, CombinerSumMuxMode sum_or_mux = SM_SUM,
                              CombinerOutOp op = OP_IDENTITY) const;
  void ClearOutputAlphaColorCombiner(int combiner) const;
  void ClearOutputAlphaCombiners() const;

  void SetFinalCombiner0Just(CombinerSource d_source, bool d_alpha = false, bool d_invert = false) const {
    SetFinalCombiner0(SRC_ZERO, false, false, SRC_ZERO, false, false, SRC_ZERO, false, false, d_source, d_alpha,
                      d_invert);
  }
  void SetFinalCombiner0(CombinerSource a_source = SRC_ZERO, bool a_alpha = false, bool a_invert = false,
                         CombinerSource b_source = SRC_ZERO, bool b_alpha = false, bool b_invert = false,
                         CombinerSource c_source = SRC_ZERO, bool c_alpha = false, bool c_invert = false,
                         CombinerSource d_source = SRC_ZERO, bool d_alpha = false, bool d_invert = false) const;

  void SetFinalCombiner1Just(CombinerSource g_source, bool g_alpha = false, bool g_invert = false) const {
    SetFinalCombiner1(SRC_ZERO, false, false, SRC_ZERO, false, false, g_source, g_alpha, g_invert);
  }
  void SetFinalCombiner1(CombinerSource e_source = SRC_ZERO, bool e_alpha = false, bool e_invert = false,
                         CombinerSource f_source = SRC_ZERO, bool f_alpha = false, bool f_invert = false,
                         CombinerSource g_source = SRC_ZERO, bool g_alpha = false, bool g_invert = false,
                         bool specular_add_invert_r0 = false, bool specular_add_invert_v1 = false,
                         bool specular_clamp = false) const;

  void SetCombinerFactorC0(int combiner, uint32_t value) const;
  void SetCombinerFactorC0(int combiner, float red, float green, float blue, float alpha) const;
  void SetCombinerFactorC1(int combiner, uint32_t value) const;
  void SetCombinerFactorC1(int combiner, float red, float green, float blue, float alpha) const;

  void SetFinalCombinerFactorC0(uint32_t value) const;
  void SetFinalCombinerFactorC0(float red, float green, float blue, float alpha) const;
  void SetFinalCombinerFactorC1(uint32_t value) const;
  void SetFinalCombinerFactorC1(float red, float green, float blue, float alpha) const;

  //! Sets the type of texture sampling for each texture.
  //!
  //! If you have a totally blank texture, double check that this is set to something other than STAGE_NONE.
  void SetShaderStageProgram(ShaderStageProgram stage_0, ShaderStageProgram stage_1 = STAGE_NONE,
                             ShaderStageProgram stage_2 = STAGE_NONE, ShaderStageProgram stage_3 = STAGE_NONE) const;
  //! Sets the input for shader stage 2 and 3. The value is the 0 based index of the stage whose output should be
  //! linked. E.g., to have stage2 use stage1's input and stage3 use stage2's the params would be (1, 2).
  void SetShaderStageInput(uint32_t stage_2_input = 0, uint32_t stage_3_input = 0) const;

  void SetVertexBufferAttributes(uint32_t enabled_fields);

  //! Overrides the default calculation of stride for a vertex attribute. "0" is special cased by the hardware to cause
  //! all reads for the attribute to be serviced by the first value in the buffer.
  void OverrideVertexAttributeStride(VertexAttribute attribute, uint32_t stride);
  //! Clears any previously set vertex attribute stride override for the given attribute.
  void ClearVertexAttributeStrideOverride(VertexAttribute attribute);
  //! Clears all vertex attribute stride overrides.
  void ClearAllVertexAttributeStrideOverrides() {
    for (auto i = 0; i < 16; ++i) {
      ClearVertexAttributeStrideOverride(static_cast<VertexAttribute>(1 << i));
    }
  }

  //! Set up the control0 register, controlling stencil writing and depth buffer mode.
  void SetupControl0(bool enable_stencil_write = true, bool w_buffered = false) const;

  //! Commit any changes to texture stages (called automatically in PrepareDraw but may be useful to call more
  //! frequently in scenes with multiple draws per clear)
  void SetupTextureStages() const;

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

  //! Returns the maximum possible value that can be stored in the depth surface for the given mode.
  static float MaxDepthBufferValue(uint32_t depth_buffer_format, bool float_mode);

  //! Returns the maximum value for the current depth buffer format.
  [[nodiscard]] float GetMaxDepthBufferValue() const {
    return MaxDepthBufferValue(depth_buffer_format_, depth_buffer_mode_float_);
  }

  //! Rounds the given integer in the same way as nv2a hardware (only remainders >= 9/16th are rounded up).
  static float NV2ARound(float input);

  //! Creates the given directory if it does not already exist.
  static void EnsureFolderExists(const std::string &folder_path);

  //! Inserts a pushbuffer command to await idle.
  static void WaitForGPU() {
    auto p = pb_begin();
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);
  }

  //! Does a busywait
  static void PBKitBusyWait() {
    while (pb_busy()) {
      /* Wait for completion... */
    }
  }

 private:
  //! Update matrices when the depth buffer format changes.
  void HandleDepthBufferFormatChange();
  [[nodiscard]] uint32_t MakeInputCombiner(CombinerSource a_source, bool a_alpha, CombinerMapping a_mapping,
                                           CombinerSource b_source, bool b_alpha, CombinerMapping b_mapping,
                                           CombinerSource c_source, bool c_alpha, CombinerMapping c_mapping,
                                           CombinerSource d_source, bool d_alpha, CombinerMapping d_mapping) const;
  [[nodiscard]] uint32_t MakeOutputCombiner(CombinerDest ab_dst, CombinerDest cd_dst, CombinerDest sum_dst,
                                            bool ab_dot_product, bool cd_dot_product, CombinerSumMuxMode sum_or_mux,
                                            CombinerOutOp op) const;
  static std::string PrepareSaveFile(std::string output_directory, const std::string &filename,
                                     const std::string &ext = ".png");
  static std::string SaveBackBuffer(const std::string &output_directory, const std::string &name);

 private:
  uint32_t framebuffer_width_;
  uint32_t framebuffer_height_;

  uint32_t max_texture_width_;
  uint32_t max_texture_height_;
  uint32_t max_texture_depth_;
  uint32_t max_single_texture_size_{0};

  TextureStage texture_stage_[4];

  bool surface_swizzle_{false};
  SurfaceColorFormat surface_color_format_{SCF_A8R8G8B8};
  SurfaceZetaFormat depth_buffer_format_{SZF_Z24S8};
  bool depth_buffer_mode_float_{false};
  uint32_t surface_clip_x_{0};
  uint32_t surface_clip_y_{0};
  uint32_t surface_clip_width_{640};
  uint32_t surface_clip_height_{480};
  uint32_t surface_width_{640};
  uint32_t surface_height_{480};
  AntiAliasingSetting antialiasing_setting_{AA_CENTER_1};

  std::shared_ptr<VertexShaderProgram> vertex_shader_program_{};

  std::shared_ptr<VertexBuffer> vertex_buffer_{};
  uint8_t *texture_memory_{nullptr};
  uint8_t *texture_palette_memory_{nullptr};
  uint32_t texture_memory_size_{0};

  enum FixedFunctionMatrixSetting {
    MATRIX_MODE_DEFAULT_NXDK,
    MATRIX_MODE_DEFAULT_XDK,
    MATRIX_MODE_USER,
  };
  FixedFunctionMatrixSetting fixed_function_matrix_mode_{MATRIX_MODE_DEFAULT_NXDK};
  matrix4_t fixed_function_model_view_matrix_{};
  matrix4_t fixed_function_projection_matrix_{};
  matrix4_t fixed_function_composite_matrix_{};
  matrix4_t fixed_function_inverse_composite_matrix_{};

  float w_near_{0.f};
  float w_far_{0.f};

  bool save_results_{true};

  std::shared_ptr<FTPLogger> ftp_logger_;

  uint32_t vertex_attribute_stride_override_[16]{
      kNoStrideOverride, kNoStrideOverride, kNoStrideOverride, kNoStrideOverride, kNoStrideOverride, kNoStrideOverride,
      kNoStrideOverride, kNoStrideOverride, kNoStrideOverride, kNoStrideOverride, kNoStrideOverride, kNoStrideOverride,
      kNoStrideOverride, kNoStrideOverride, kNoStrideOverride, kNoStrideOverride};
};

#endif  // NXDK_PGRAPH_TESTS_TEST_HOST_H
