#ifndef NXDK_PGRAPH_TESTS_TEXTURE_GENERATOR_H
#define NXDK_PGRAPH_TESTS_TEXTURE_GENERATOR_H

#include <SDL.h>

#include <cstdint>

// Inserts a rectangular checkerboard pattern into the given 32bpp texture buffer.
void GenerateRGBACheckerboard(void *buffer, uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height,
                              uint32_t pitch, uint32_t first_color = 0xFF00FFFF, uint32_t second_color = 0xFF000000,
                              uint32_t checker_size = 8);

void GenerateColoredCheckerboard(void *buffer, uint32_t x_offset, uint32_t y_offset, uint32_t width, uint32_t height,
                                 uint32_t pitch, const uint32_t *colors, uint32_t num_colors,
                                 uint32_t checker_size = 8);

void GenerateSwizzledRGBACheckerboard(void *buffer, uint32_t x_offset, uint32_t y_offset, uint32_t width,
                                      uint32_t height, uint32_t pitch, uint32_t first_color = 0xFF00FFFF,
                                      uint32_t second_color = 0xFF000000, uint32_t checker_size = 8);

void GenerateRGBATestPattern(void *target, uint32_t width, uint32_t height);

int GenerateSurface(SDL_Surface **surface, int width, int height);
int GenerateCheckerboardSurface(SDL_Surface **surface, int width, int height, uint32_t first_color = 0xFF00FFFF,
                                uint32_t second_color = 0xFF000000, uint32_t checker_size = 8);
int GenerateColoredCheckerboardSurface(SDL_Surface **surface, int width, int height, uint32_t checker_size = 4);
#endif  // NXDK_PGRAPH_TESTS_TEXTURE_GENERATOR_H
