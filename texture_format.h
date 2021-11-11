#ifndef NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_H
#define NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_H

#include <SDL.h>

typedef struct TextureFormatInfo {
  SDL_PixelFormatEnum SdlFormat;
  uint32_t XboxFormat;
  uint16_t XboxBpp;  // bytes per pixel
  bool XboxSwizzled;
  bool RequireConversion;
  const char *Name;
} TextureFormatInfo;

extern const TextureFormatInfo kTextureFormats[];
extern const int kNumFormats;

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_FORMAT_H
