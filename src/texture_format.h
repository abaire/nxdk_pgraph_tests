#ifndef NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_H
#define NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_H

#include <SDL.h>

typedef struct TextureFormatInfo {
  SDL_PixelFormatEnum sdl_format{SDL_PIXELFORMAT_ARGB8888};
  uint32_t xbox_format{0};
  uint16_t xbox_bpp{4};  // bytes per pixel
  bool xbox_swizzled{false};
  bool require_conversion{false};
  const char *name{nullptr};
} TextureFormatInfo;

extern const TextureFormatInfo kTextureFormats[];
extern const int kNumFormats;

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_H
