#ifndef NXDK_PGRAPH_TESTS_TEXTURE_STAGE_H
#define NXDK_PGRAPH_TESTS_TEXTURE_STAGE_H

#include <pbkit/pbkit.h>
#include <printf/printf.h>

#include "texture_format.h"
#include "xbox_math_types.h"

using namespace XboxMath;

// Sets up an nv2a texture stage.
class TextureStage {
 public:
  enum ConvolutionKernel {
    K_QUINCUNX = 1,
    K_GAUSSIAN_3 = 2,
  };

  enum MinFilter {
    MIN_BOX_LOD0 = 1,
    MIN_TENT_LOD0,
    MIN_BOX_NEARESTLOD,
    MIN_TENT_NEARESTLOD,
    MIN_BOX_TENT_LOD,
    MIN_TENT_TENT_LOD,
    MIN_CONVOLUTION_2D_LOD0,
  };

  enum MagFilter {
    MAG_BOX_LOD0 = 1,
    MAG_TENT_LOD0 = 2,
    MAG_CONVOLUTION_2D_LOD0 = 4,
  };

  enum WrapMode { WRAP_REPEAT = 1, WRAP_MIRROR, WRAP_CLAMP_TO_EDGE, WRAP_BORDER, WRAP_CLAMP_TO_EDGE_OGL };

  enum TexGen {
    TG_DISABLE = NV097_SET_TEXGEN_S_DISABLE,
    TG_EYE_LINEAR = NV097_SET_TEXGEN_S_EYE_LINEAR,
    TG_OBJECT_LINEAR = NV097_SET_TEXGEN_S_OBJECT_LINEAR,
    TG_SPHERE_MAP = NV097_SET_TEXGEN_S_SPHERE_MAP,
    TG_NORMAL_MAP = NV097_SET_TEXGEN_S_NORMAL_MAP,
    TG_REFLECTION_MAP = NV097_SET_TEXGEN_S_REFLECTION_MAP,
  };

  enum ColorKeyMode {
    //! Do nothing on a key match.
    CKM_DISABLE = 0,
    //! Zero out just alpha on a key match.
    CKM_KILL_ALPHA = 1,
    //! Zero out ARGB on a key match.
    CKM_KILL_COLOR = 2
  };

 public:
  TextureStage();

  void Reset() {
    lod_min_ = 0;
    lod_max_ = 4095;

    mipmap_levels_ = 1;

    texture_filter_ = 0x1012000;

    border_color_ = 0;
    cubemap_enable_ = false;
    border_source_color_ = true;
    color_key_mode_ = CKM_DISABLE;

    wrap_modes_[0] = WRAP_CLAMP_TO_EDGE;
    wrap_modes_[1] = WRAP_CLAMP_TO_EDGE;
    wrap_modes_[2] = WRAP_CLAMP_TO_EDGE;
    cylinder_wrap_[0] = false;
    cylinder_wrap_[1] = false;
    cylinder_wrap_[2] = false;
    cylinder_wrap_[3] = false;

    bump_env_material[0] = 0.0f;
    bump_env_material[1] = 0.0f;
    bump_env_material[2] = 0.0f;
    bump_env_material[3] = 0.0f;

    bump_env_scale = 0.0f;
    bump_env_offset = 0.0f;

    texture_matrix_enable_ = false;

    texgen_s_ = TG_DISABLE;
    texgen_t_ = TG_DISABLE;
    texgen_r_ = TG_DISABLE;
    texgen_q_ = TG_DISABLE;
  }

  void SetUWrap(WrapMode mode, bool cylinder_wrap = false) {
    wrap_modes_[0] = mode;
    cylinder_wrap_[0] = cylinder_wrap;
  }
  void SetVWrap(WrapMode mode, bool cylinder_wrap = false) {
    wrap_modes_[1] = mode;
    cylinder_wrap_[1] = cylinder_wrap;
  }
  void SetPWrap(WrapMode mode, bool cylinder_wrap = false) {
    wrap_modes_[2] = mode;
    cylinder_wrap_[2] = cylinder_wrap;
  }
  void SetQWrap(bool cylinder_wrap = false) { cylinder_wrap_[3] = cylinder_wrap; }

  bool GetEnabled() const { return enabled_; }
  void SetEnabled(bool enabled = true) { enabled_ = enabled; }
  const TextureFormatInfo &GetFormat() const { return format_; }
  void SetFormat(const TextureFormatInfo &format) { format_ = format; }
  uint32_t GetBorderColor() const { return border_color_; }
  void SetBorderColor(uint32_t color) { border_color_ = color; }

  bool GetCubemapEnable() const { return cubemap_enable_; }
  void SetCubemapEnable(bool val = true) { cubemap_enable_ = val; }

  bool GetAlphaKillEnable() const { return alpha_kill_enable_; }
  void SetAlphaKillEnable(bool val = true) { alpha_kill_enable_ = val; }
  uint32_t GetColorKeyMode() const { return color_key_mode_; }
  void SetColorKeyMode(uint32_t mode) { color_key_mode_ = mode; }
  void SetLODClamp(uint32_t min = 0, uint32_t max = 4095) {
    lod_min_ = min;
    lod_max_ = max;
  }

  // Determines whether texels along the border retrieve their color from border_color_ or the texture.
  void SetBorderFromColor(bool val = true) { border_source_color_ = val; }

  void SetBumpEnv(float x, float y, float z, float w, float scale, float offset = 0.0f) {
    bump_env_material[0] = x;
    bump_env_material[1] = y;
    bump_env_material[2] = z;
    bump_env_material[3] = w;
    bump_env_scale = scale;
    bump_env_offset = offset;
  }

  void SetTextureDimensions(uint32_t u, uint32_t v, uint32_t p = 1) {
    size_u_ = u;
    size_v_ = v;
    size_p_ = p;
    depth_ = size_p_;
  }

  void SetImageDimensions(uint32_t width, uint32_t height, uint32_t depth = 1) {
    width_ = width;
    height_ = height;
    depth_ = depth;
    size_p_ = depth;
  }

  uint32_t GetDimensionality() const;

  void SetFilter(uint32_t lod_bias = 0, ConvolutionKernel kernel = K_QUINCUNX, MinFilter min = MIN_BOX_LOD0,
                 MagFilter mag = MAG_BOX_LOD0, bool signed_alpha = false, bool signed_red = false,
                 bool signed_green = false, bool signed_blue = false);

  bool IsSwizzled() const { return format_.xbox_swizzled; }
  bool IsLinear() const { return format_.xbox_linear; }
  bool RequiresColorspaceConversion() const;

  void SetTextureMatrixEnable(bool enabled) { texture_matrix_enable_ = enabled; }
  bool GetTextureMatrixEnable() const { return texture_matrix_enable_; }
  void SetTextureMatrix(const matrix4_t &matrix) { memcpy(texture_matrix_, matrix, sizeof(texture_matrix_)); }
  const matrix4_t &GetTextureMatrix() const { return texture_matrix_; }
  matrix4_t &GetTextureMatrix() { return texture_matrix_; }

  TexGen GetTexgenS() const { return texgen_s_; }
  TexGen GetTexgenT() const { return texgen_t_; }
  TexGen GetTexgenR() const { return texgen_r_; }
  TexGen GetTexgenQ() const { return texgen_q_; }
  void SetTexgenS(TexGen val) { texgen_s_ = val; }
  void SetTexgenT(TexGen val) { texgen_t_ = val; }
  void SetTexgenR(TexGen val) { texgen_r_ = val; }
  void SetTexgenQ(TexGen val) { texgen_q_ = val; }

  void SetMipMapLevels(uint32_t val) { mipmap_levels_ = val; }
  uint32_t GetMipMapLevels() const { return mipmap_levels_; }

 private:
  friend class TestHost;

  void SetStage(uint32_t stage) { stage_ = stage; }

  uint32_t GetTextureOffset() const { return texture_memory_offset_; }
  void SetTextureOffset(uint32_t offset) { texture_memory_offset_ = offset; }
  uint32_t GetPaletteOffset() const { return palette_memory_offset_; }
  void SetPaletteOffset(uint32_t offset) { palette_memory_offset_ = offset; }

  void Commit(uint32_t memory_dma_offset, uint32_t palette_dma_offset) const;

  int SetTexture(const SDL_Surface *surface, uint8_t *memory_base) const;
  int SetVolumetricTexture(const SDL_Surface **layers, uint32_t depth, uint8_t *memory_base) const;
  int SetRawTexture(const uint8_t *source, uint32_t width, uint32_t height, uint32_t depth, uint32_t pitch,
                    uint32_t bytes_per_pixel, bool swizzle, uint8_t *memory_base) const;

  int SetPalette(const uint32_t *palette, uint32_t length, uint8_t *memory_base);
  int SetPaletteSize(uint32_t length);

 private:
  uint32_t stage_{0};
  bool enabled_{false};
  bool alpha_kill_enable_{false};
  uint32_t color_key_mode_{CKM_DISABLE};
  uint32_t lod_min_{0};
  uint32_t lod_max_{4095};

  TextureFormatInfo format_;
  uint32_t width_{0};
  uint32_t height_{0};
  uint32_t depth_{0};
  uint32_t texture_memory_offset_{0};

  uint32_t size_u_{0};
  uint32_t size_v_{0};
  uint32_t size_p_{0};

  uint32_t mipmap_levels_{1};

  uint32_t palette_length_{10};
  uint32_t palette_memory_offset_{0};

  uint32_t texture_filter_{0x1012000};

  uint32_t border_color_{0};
  bool cubemap_enable_{false};
  bool border_source_color_{true};

  WrapMode wrap_modes_[3]{WRAP_CLAMP_TO_EDGE, WRAP_CLAMP_TO_EDGE, WRAP_CLAMP_TO_EDGE};
  bool cylinder_wrap_[4] = {false};

  float bump_env_material[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float bump_env_scale{0.0f};
  float bump_env_offset{0.0f};

  bool texture_matrix_enable_{false};
  matrix4_t texture_matrix_;

  TexGen texgen_s_{TG_DISABLE};
  TexGen texgen_t_{TG_DISABLE};
  TexGen texgen_r_{TG_DISABLE};
  TexGen texgen_q_{TG_DISABLE};
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_STAGE_H
