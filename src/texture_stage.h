#ifndef NXDK_PGRAPH_TESTS_TEXTURE_STAGE_H
#define NXDK_PGRAPH_TESTS_TEXTURE_STAGE_H

#include <pbkit/pbkit.h>
#include <printf/printf.h>

#include "texture_format.h"

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

 public:
  void SetEnabled(bool enabled = true) { enabled_ = enabled; }
  void SetFormat(const TextureFormatInfo &format) { format_ = format; }
  void SetBorderColor(uint32_t color) { border_color_ = color; }
  void SetBumpEnv(float x, float y, float z, float w, float scale, float offset = 0.0f) {
    bump_env_material[0] = x;
    bump_env_material[1] = y;
    bump_env_material[2] = z;
    bump_env_material[3] = w;
    bump_env_scale = scale;
    bump_env_offset = offset;
  }
  void SetDimensions(uint32_t width, uint32_t height, uint32_t depth = 1) {
    width_ = width;
    height_ = height;
    depth_ = depth;
  }

  uint32_t GetDimensionality() const {
    if (height_ == 1 && depth_ == 1) {
      return 1;
    }
    if (depth_ > 1) {
      return 3;
    }
    return 2;
  }
  bool IsSwizzled() const { return format_.xbox_swizzled; }
  bool RequiresColorspaceConversion() const;

 private:
  friend class TestHost;

  void SetStage(uint32_t stage) { stage_ = stage; }

  void SetTextureOffset(uint32_t offset) { texture_memory_offset_ = offset; }
  void SetPaletteOffset(uint32_t offset) { palette_memory_offset_ = offset; }
  void SetFilter(uint32_t lod_bias = 0, ConvolutionKernel kernel = K_QUINCUNX, MinFilter min = MIN_BOX_LOD0,
                 MagFilter mag = MAG_BOX_LOD0, bool signed_alpha = false, bool signed_red = false,
                 bool signed_green = false, bool signed_blue = false);

  void Commit(uint32_t memory_dma_offset, uint32_t palette_dma_offset) const;

  int SetTexture(const SDL_Surface *surface, uint8_t *memory_base) const;
  int SetVolumetricTexture(const SDL_Surface **layers, uint32_t depth, uint8_t *memory_base) const;
  int SetRawTexture(const uint8_t *source, uint32_t width, uint32_t height, uint32_t depth, uint32_t pitch,
                    uint32_t bytes_per_pixel, bool swizzle, uint8_t *memory_base) const;

  int SetPalette(const uint32_t *palette, uint32_t length, uint8_t *memory_base);

 private:
  uint32_t stage_{0};
  bool enabled_{false};
  TextureFormatInfo format_;
  uint32_t width_{0};
  uint32_t height_{0};
  uint32_t depth_{0};
  uint32_t texture_memory_offset_{0};

  uint32_t mipmap_levels_{1};

  uint32_t palette_length_{10};
  uint32_t palette_memory_offset_{0};

  uint32_t texture_filter_{0x1012000};
  uint32_t border_color_{0};

  float bump_env_material[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float bump_env_scale{0.0f};
  float bump_env_offset{0.0f};
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_STAGE_H
