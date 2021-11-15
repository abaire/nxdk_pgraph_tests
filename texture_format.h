#ifndef NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_H
#define NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_H

#include <SDL.h>

typedef struct TextureFormatInfo {
  SDL_PixelFormatEnum sdl_format;
  uint32_t xbox_format;
  uint16_t xbox_bpp;  // bytes per pixel
  bool xbox_swizzled;
  bool require_conversion;
  const char *name;
} TextureFormatInfo;

extern const TextureFormatInfo kTextureFormats[];
extern const int kNumFormats;

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_H
