#include "texture_format.h"

#include <SDL_pixels.h>
#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "nxdk_ext.h"

static TextureFormatInfo kInvalidTextureFormatInfo{};

constexpr TextureFormatInfo kTextureFormats[] = {

    // swizzled
    {SDL_PIXELFORMAT_ABGR8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8, 32, true, false, false, "A8B8G8R8"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8G8B8A8, 32, true, false, false, "R8G8B8A8"},
    {SDL_PIXELFORMAT_ARGB8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8, 32, true, false, false, "A8R8G8B8"},
    {SDL_PIXELFORMAT_ARGB8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8, 32, true, false, false, "X8R8G8B8"},
    {SDL_PIXELFORMAT_BGRA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_B8G8R8A8, 32, true, false, false, "B8G8R8A8"},
    {SDL_PIXELFORMAT_RGB565, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R5G6B5, 16, true, false, false, "R5G6B5"},
    {SDL_PIXELFORMAT_ARGB1555, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A1R5G5B5, 16, true, false, false, "A1R5G5B5"},
    {SDL_PIXELFORMAT_ARGB1555, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X1R5G5B5, 16, true, false, false, "X1R5G5B5"},
    {SDL_PIXELFORMAT_ARGB4444, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A4R4G4B4, 16, true, false, false, "A4R4G4B4"},

    // linear unsigned
    {SDL_PIXELFORMAT_ABGR8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8B8G8R8, 32, false, true, false, "A8B8G8R8"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R8G8B8A8, 32, false, true, false, "R8G8B8A8"},
    {SDL_PIXELFORMAT_ARGB8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8, 32, false, true, false, "A8R8G8B8"},
    {SDL_PIXELFORMAT_ARGB8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8, 32, false, true, false, "X8R8G8B8"},
    {SDL_PIXELFORMAT_BGRA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_B8G8R8A8, 32, false, true, false, "B8G8R8A8"},
    {SDL_PIXELFORMAT_RGB565, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R5G6B5, 16, false, true, false, "R5G6B5"},
    {SDL_PIXELFORMAT_ARGB1555, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A1R5G5B5, 16, false, true, false, "A1R5G5B5"},
    {SDL_PIXELFORMAT_ARGB1555, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X1R5G5B5, 16, false, true, false, "X1R5G5B5"},
    {SDL_PIXELFORMAT_ARGB4444, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A4R4G4B4, 16, false, true, false, "A4R4G4B4"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_G8B8, 16, false, true, true, "G8B8"},
    // TODO: Implement in xemu
    //{SDL_PIXELFORMAT_UNKNOWN, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R8B8, 16, false, true, true, "R8B8"},

    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_X8_Y24_FIXED, 32, false, true, false,
     "D_X8Y24_FIXED"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED, 16, false, true, true,
     "D_Y16_FIXED"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FLOAT, 16, false, true, true,
     "D_Y16_FLOAT"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_Y16, 16, false, true, true, "Y16"},

    // yuv color space
    // Each 4 bytes represent the color for 2 neighboring pixels:
    // [ U0 | Y0 | V0 | Y1 ]
    // Y0 is the brightness of pixel 0, Y1 the brightness of pixel 1.
    // U0 and V0 is the color of both pixels. (second pixel is the one sampled?
    // or averaged? doesn't really matter here)
    // https://docs.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering#converting-8-bit-yuv-to-rgb888
    {SDL_PIXELFORMAT_BGRA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_CR8YB8CB8YA8, 16, false, true, true, "YUY2"},
    {SDL_PIXELFORMAT_BGRA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_YB8CR8YA8CB8, 16, false, true, true, "UYVY"},
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_Y16, false, true, "Y16" },
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_Y8, 8, false, true, true, "Y8"},
    {SDL_PIXELFORMAT_UNKNOWN, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8, 8, false, true, false, "A8"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_AY8, 8, false, true, true, "AY8"},

    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_Y8, 8, true, false, true, "Y8"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_AY8, 8, true, false, true, "AY8"},
    {SDL_PIXELFORMAT_UNKNOWN, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8, 8, true, false, false, "A8"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8Y8, 16, true, false, true, "A8Y8"},

    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_G8B8, 16, true, false, true, "G8B8"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8B8, 16, true, false, true, "R8B8"},

    // Compressed formats
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT1_A1R5G5B5, 4, false, false, true, "DXT1"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT23_A8R8G8B8, 4, false, false, true, "DXT3"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT45_A8R8G8B8, 4, false, false, true, "DXT5"},

    // misc formats
    // TODO: implement in xemu
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_D16, false, true, "D16" },
    // TODO: implement in xemu
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LIN_F16, false, true, "LIN_F16" },
    {SDL_PIXELFORMAT_RGB565, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R6G5B5, 16, true, false, false, "R6G5B5"},

    {SDL_PIXELFORMAT_INDEX8, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8, 8, true, false, false, "SZ_Index8"},
};

constexpr int kNumFormats = sizeof(kTextureFormats) / sizeof(kTextureFormats[0]);

const TextureFormatInfo &GetTextureFormatInfo(uint32_t nv_texture_format) {
  for (const auto &info : kTextureFormats) {
    if (info.xbox_format == nv_texture_format) {
      return info;
    }
  }

  ASSERT(!"Unknown texture format.");
  return kInvalidTextureFormatInfo;
}
