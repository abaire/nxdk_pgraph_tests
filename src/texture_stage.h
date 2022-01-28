#ifndef NXDK_PGRAPH_TESTS_TEXTURE_STAGE_H
#define NXDK_PGRAPH_TESTS_TEXTURE_STAGE_H

#include <pbkit/pbkit.h>
#include <printf/printf.h>

#include "texture_format.h"

class TextureStage {
 public:
  void SetEnabled(bool enabled = true) { enabled_ = enabled; }
  void SetFormat(const TextureFormatInfo &format) { format_ = format; }
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
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_STAGE_H
