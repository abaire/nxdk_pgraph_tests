#include "texture_generator.h"

#include <debug_output.h>

#include "swizzle.h"

void GenerateRGBACheckerboard(void *target, uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height,
                              uint32_t pitch, uint32_t first_color, uint32_t second_color, uint32_t checker_size) {
  auto buffer = reinterpret_cast<uint8_t *>(target);
  auto odd = first_color;
  auto even = second_color;
  buffer += y_offset * pitch;

  for (uint32_t y = 0; y < height; ++y) {
    auto pixel = reinterpret_cast<uint32_t *>(buffer);
    pixel += x_offset;
    buffer += pitch;

    if (!(y % checker_size)) {
      auto temp = odd;
      odd = even;
      even = temp;
    }

    for (uint32_t x = 0; x < width; ++x) {
      *pixel++ = ((x / checker_size) & 0x01) ? odd : even;
    }
  }
}

void GenerateSwizzledRGBACheckerboard(void *target, uint32_t x_offset, uint32_t y_offset, uint32_t width,
                                      uint32_t height, uint32_t pitch, uint32_t first_color, uint32_t second_color,
                                      uint32_t checker_size) {
  const uint32_t size = height * pitch;
  auto temp_buffer = new uint8_t[size];
  memcpy(temp_buffer, target, size);

  GenerateRGBACheckerboard(temp_buffer, x_offset, y_offset, width, height, pitch, first_color, second_color,
                           checker_size);
  swizzle_rect(temp_buffer, width, height, reinterpret_cast<uint8_t *>(target), pitch, 4);
  delete[] temp_buffer;
}

static void GenerateRGBTestPattern(void *target, uint32_t width, uint32_t height, uint8_t alpha, uint32_t pitch) {
  auto row = static_cast<uint32_t *>(target);

  const uint32_t alpha_channel = static_cast<uint32_t>(alpha) << 24;
  for (uint32_t y = 0; y < height; ++y) {
    auto y_normal = static_cast<uint32_t>(static_cast<float>(y) * 255.0f / static_cast<float>(height));

    auto pixels = row;
    for (uint32_t x = 0; x < width; ++x, ++pixels) {
      auto x_normal = static_cast<uint32_t>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      *pixels = y_normal + (x_normal << 8) + ((255 - y_normal) << 16) + alpha_channel;
    }

    row += pitch;
  }
}

void GenerateRGBTestPattern(void *target, uint32_t width, uint32_t height, uint8_t alpha) {
  GenerateRGBTestPattern(target, width, height, alpha, width * 4);
}

void GenerateRGBATestPattern(void *target, uint32_t width, uint32_t height) {
  auto pixels = static_cast<uint32_t *>(target);
  for (uint32_t y = 0; y < height; ++y) {
    auto y_normal = static_cast<uint32_t>(static_cast<float>(y) * 255.0f / static_cast<float>(height));

    for (uint32_t x = 0; x < width; ++x, ++pixels) {
      auto x_normal = static_cast<uint32_t>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      *pixels = y_normal + (x_normal << 8) + ((255 - y_normal) << 16) + ((x_normal + y_normal) << 24);
    }
  }
}

void GenerateRGBRadialATestPattern(void *target, uint32_t width, uint32_t height) {
  ASSERT(!(width & 1) && "Width must be even");
  ASSERT(!(height & 1) && "Height must be even");

  auto pixels = static_cast<uint32_t *>(target);

  auto half_width = width >> 1;
  auto half_height = height >> 1;

  auto ur_pixels = pixels + half_width;

  GenerateRGBTestPattern(pixels, half_width, half_height, 0x00, width);
  GenerateRGBTestPattern(ur_pixels, half_width, half_height, 0x00, width);

  auto ll_pixels = pixels + half_height * width;
  memcpy(ll_pixels, pixels, width * half_height * 4);

  auto cx = static_cast<float>(half_width);
  auto cy = static_cast<float>(half_height);
  auto max_distance = cx * cx + cy * cy;

  for (uint32_t y = 0; y < height; ++y) {
    auto dy = static_cast<float>(y) - cx;

    for (uint32_t x = 0; x < width; ++x, ++pixels) {
      auto dx = static_cast<float>(x) - cx;
      auto distance = dx * dx + dy * dy;
      auto alpha = static_cast<uint32_t>(255.0f * (1.f - distance / max_distance));
      *pixels = (*pixels & 0x00FFFFFF) + (alpha << 24);
    }
  }
}

int GenerateSurface(SDL_Surface **surface, int width, int height) {
  *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  if (!(*surface)) {
    return 1;
  }

  if (SDL_LockSurface(*surface)) {
    SDL_FreeSurface(*surface);
    *surface = nullptr;
    return 2;
  }

  auto pixels = static_cast<uint32_t *>((*surface)->pixels);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x, ++pixels) {
      int x_normal = static_cast<int>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      int y_normal = static_cast<int>(static_cast<float>(y) * 255.0f / static_cast<float>(height));
      *pixels = SDL_MapRGBA((*surface)->format, y_normal, x_normal, 255 - y_normal, x_normal + y_normal);
    }
  }

  SDL_UnlockSurface(*surface);

  return 0;
}

int GenerateCheckboardSurface(SDL_Surface **surface, int width, int height) {
  *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  if (!(*surface)) {
    return 1;
  }

  if (SDL_LockSurface(*surface)) {
    SDL_FreeSurface(*surface);
    *surface = nullptr;
    return 2;
  }

  auto pixels = static_cast<uint32_t *>((*surface)->pixels);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x, ++pixels) {
      int x_normal = static_cast<int>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      int y_normal = static_cast<int>(static_cast<float>(y) * 255.0f / static_cast<float>(height));
      *pixels = SDL_MapRGBA((*surface)->format, y_normal, x_normal, 255 - y_normal, x_normal + y_normal);
    }
  }

  SDL_UnlockSurface(*surface);

  return 0;
}

int GenerateCheckerboardSurface(SDL_Surface **surface, int width, int height, uint32_t first_color,
                                uint32_t second_color, uint32_t checker_size) {
  *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ABGR8888);
  if (!(*surface)) {
    return 1;
  }

  if (SDL_LockSurface(*surface)) {
    SDL_FreeSurface(*surface);
    *surface = nullptr;
    return 2;
  }

  auto pixels = static_cast<uint32_t *>((*surface)->pixels);
  GenerateRGBACheckerboard(pixels, 0, 0, width, height, width * 4, first_color, second_color, checker_size);

  SDL_UnlockSurface(*surface);

  return 0;
}

void GenerateColoredCheckerboard(void *target, uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height,
                                 uint32_t pitch, const uint32_t *colors, uint32_t num_colors, uint32_t checker_size) {
  auto buffer = reinterpret_cast<uint8_t *>(target);
  buffer += y_offset * pitch;

  uint32_t row_color_index = 0;

  for (uint32_t y = 0; y < height; ++y) {
    auto pixel = reinterpret_cast<uint32_t *>(buffer);
    pixel += x_offset;
    buffer += pitch;

    if (y && !(y % checker_size)) {
      row_color_index = ++row_color_index % num_colors;
    }

    auto color_index = row_color_index;
    auto color = colors[color_index];
    for (uint32_t x = 0; x < width; ++x) {
      if (x && !(x % checker_size)) {
        color_index = ++color_index % num_colors;
        color = colors[color_index];
      }
      *pixel++ = color;
    }
  }
}

int GenerateColoredCheckerboardSurface(SDL_Surface **gradient_surface, int width, int height, uint32_t checker_size) {
  *gradient_surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  if (!(*gradient_surface)) {
    return 1;
  }

  if (SDL_LockSurface(*gradient_surface)) {
    SDL_FreeSurface(*gradient_surface);
    *gradient_surface = nullptr;
    return 2;
  }

  static const uint32_t kColors[] = {
      SDL_MapRGBA((*gradient_surface)->format, 0xFF, 0, 0, 0xFF),
      SDL_MapRGBA((*gradient_surface)->format, 0xFF, 0, 0, 0x66),
      SDL_MapRGBA((*gradient_surface)->format, 0, 0xFF, 0, 0xFF),
      SDL_MapRGBA((*gradient_surface)->format, 0, 0xFF, 0, 0x66),
      SDL_MapRGBA((*gradient_surface)->format, 0x44, 0x44, 0xFF, 0xFF),
      SDL_MapRGBA((*gradient_surface)->format, 0x44, 0x44, 0xFF, 0x66),
      SDL_MapRGBA((*gradient_surface)->format, 0xFF, 0xFF, 0xFF, 0xFF),
      SDL_MapRGBA((*gradient_surface)->format, 0xFF, 0xFF, 0xFF, 0x66),
  };
  static const uint32_t kNumColors = sizeof(kColors) / sizeof(kColors[0]);

  auto pixels = static_cast<uint8_t *>((*gradient_surface)->pixels);
  GenerateColoredCheckerboard(pixels, 0, 0, width, height, width * 4, kColors, kNumColors, checker_size);

  SDL_UnlockSurface(*gradient_surface);

  return 0;
}
