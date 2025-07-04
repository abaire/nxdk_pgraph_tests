#include "volume_texture_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "test_host.h"
#include "texture_format.h"
#include "vertex_buffer.h"

static int GenerateSurface(SDL_Surface **gradient_surface, int width, int height, int layer);
static int GeneratePalettizedSurface(uint8_t **ret, uint32_t width, uint32_t height, uint32_t depth,
                                     TestHost::PaletteSize palette_size);
static uint32_t *GeneratePalette(TestHost::PaletteSize size);

static constexpr uint32_t kBackgroundColor = 0xFE202020;
static const uint32_t kTextureWidth = 256;
static const uint32_t kTextureHeight = 256;
static const uint32_t kTextureDepth = 4;

static const uint32_t kNumQuads = 7;

static bool RequiresSpecialTest(const TextureFormatInfo &format) {
  switch (format.xbox_format) {
    case NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8:
    case NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED:
    case NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FLOAT:
    case NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_X8_Y24_FIXED:
    case NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT23_A8R8G8B8:
    case NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT45_A8R8G8B8:
      return true;

    default:
      return false;
  }
}

VolumeTextureTests::VolumeTextureTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Volume texture", config) {
  for (auto i = 0; i < kNumFormats; ++i) {
    auto &format = kTextureFormats[i];
    if (format.xbox_linear) {
      // Linear volumetric formats are not supported by the hardware.
      continue;
    }

    if (!RequiresSpecialTest(format)) {
      tests_[format.name] = [this, format]() { Test(format); };
    }
  }

  auto palettized = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8);
  tests_[palettized.name] = [this]() { TestPalettized(); };
}

void VolumeTextureTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetVertexShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetTextureStageEnabled(0, true);

  host_.SetShaderStageProgram(TestHost::STAGE_3D_PROJECTIVE);

  host_.SetInputColorCombiner(0, TestHost::SRC_TEX0, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                              TestHost::MAP_UNSIGNED_INVERT);
  host_.SetInputAlphaCombiner(0, TestHost::SRC_TEX0, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                              TestHost::MAP_UNSIGNED_INVERT);

  host_.SetOutputColorCombiner(0, TestHost::DST_R0);
  host_.SetOutputAlphaCombiner(0, TestHost::DST_R0);

  host_.SetFinalCombiner0(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false,
                          false, TestHost::SRC_R0);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true);
}

void VolumeTextureTests::CreateGeometry() {
  const float kLeft = -2.75f;
  const float kRight = 2.75f;
  const float kTop = 1.75f;
  const float kBottom = -1.75f;

  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * kNumQuads);
  buffer->SetTexCoord0Count(3);

  const float spacing = 0.1f;
  const float width = (kRight - kLeft) / 3.0f - spacing * 2.0f;
  const float height = (kTop - kBottom) / 3.0f - spacing * 2.0f;

  int index = 0;
  float left = kLeft;
  float top = kTop;
  buffer->DefineBiTri(index++, left, top, left + width, top - height);

  // Midway between layer 0 and 1.
  left += width + spacing;
  buffer->DefineBiTri(index++, left, top, left + width, top - height);

  left += width + spacing;
  buffer->DefineBiTri(index++, left, top, left + width, top - height);

  // Midway between layer 1 and 2.
  left = kLeft + width + spacing;
  top -= height + spacing;
  buffer->DefineBiTri(index++, left, top, left + width, top - height);

  left = kLeft;
  top -= height + spacing;
  buffer->DefineBiTri(index++, left, top, left + width, top - height);

  // Midway between layer 2 and 3.
  left += width + spacing;
  buffer->DefineBiTri(index++, left, top, left + width, top - height);

  left += width + spacing;
  buffer->DefineBiTri(index++, left, top, left + width, top - height);

  // Set texcoords.
  auto vertex = buffer->Lock();

  auto set_bitri_texcoords = [&vertex](float p) {
    vertex++->SetTexCoord0(0.0f, 0.0f, p, 0.0);
    vertex++->SetTexCoord0(1.0f, 0.0f, p, 0.0);
    vertex++->SetTexCoord0(1.0f, 1.0f, p, 0.0);

    vertex++->SetTexCoord0(0.0f, 0.0f, p, 0.0);
    vertex++->SetTexCoord0(1.0f, 1.0f, p, 0.0);
    vertex++->SetTexCoord0(0.0f, 1.0f, p, 0.0);
  };

  float inc = 1.0f / kNumQuads;
  float p = 0.0f;
  for (auto i = 0; i < kNumQuads - 1; ++i) {
    set_bitri_texcoords(p);
    p += inc;
  }
  set_bitri_texcoords(1.0f);

  buffer->Unlock();

  buffer->Linearize(static_cast<float>(kTextureWidth), static_cast<float>(kTextureHeight));
}

void VolumeTextureTests::Test(const TextureFormatInfo &texture_format) {
  const uint32_t width = kTextureWidth;
  const uint32_t height = kTextureHeight;

  host_.SetTextureFormat(texture_format);
  auto **layers = new SDL_Surface *[kTextureDepth];

  for (auto d = 0; d < kTextureDepth; ++d) {
    int update_texture_result = GenerateSurface(&layers[d], (int)width, (int)height, d);
    ASSERT(!update_texture_result && "Failed to generate SDL surface");
  }

  int update_texture_result = host_.SetVolumetricTexture((const SDL_Surface **)layers, kTextureDepth);
  ASSERT(!update_texture_result && "Failed to set texture");

  for (auto d = 0; d < kTextureDepth; ++d) {
    SDL_FreeSurface(layers[d]);
  }
  delete[] layers;

  auto &stage = host_.GetTextureStage(0);
  stage.SetTextureDimensions(width, height, kTextureDepth);
  stage.SetImageDimensions(width, height, kTextureDepth);

  host_.PrepareDraw(kBackgroundColor);
  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE | TestHost::TEXCOORD0);

  pb_print("N: %s\n", texture_format.name);
  pb_print("F: 0x%x\n", texture_format.xbox_format);
  pb_print("SZ: %d\n", texture_format.xbox_swizzled);
  pb_print("C: %d\n", texture_format.require_conversion);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, texture_format.name);
}

void VolumeTextureTests::TestPalettized() {
  TestHost::PaletteSize palette_size = TestHost::PALETTE_256;
  auto &texture_format = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8);
  host_.SetTextureFormat(texture_format);

  const uint32_t width = kTextureWidth;
  const uint32_t height = kTextureHeight;
  const uint32_t depth = kTextureDepth;
  auto &stage = host_.GetTextureStage(0);
  stage.SetTextureDimensions(width, height, depth);
  stage.SetImageDimensions(width, height, depth);

  uint8_t *surface = nullptr;
  int err = GeneratePalettizedSurface(&surface, width, height, depth, palette_size);
  ASSERT(!err && "Failed to generate palettized surface");

  err = host_.SetRawTexture(surface, width, height, depth, width, 1, texture_format.xbox_swizzled);
  delete[] surface;
  ASSERT(!err && "Failed to set texture");

  auto palette = GeneratePalette(palette_size);
  err = host_.SetPalette(palette, palette_size);
  delete[] palette;
  ASSERT(!err && "Failed to set palette");

  host_.PrepareDraw(kBackgroundColor);
  host_.DrawArrays();

  pb_print("N: %s\n", texture_format.name);
  pb_print("F: 0x%x\n", texture_format.xbox_format);
  pb_print("SZ: %d\n", texture_format.xbox_swizzled);
  pb_print("C: %d\n", texture_format.require_conversion);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, texture_format.name);
}

static int GenerateSurface(SDL_Surface **gradient_surface, int width, int height, int layer) {
  *gradient_surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  if (!(*gradient_surface)) {
    return 1;
  }

  if (SDL_LockSurface(*gradient_surface)) {
    SDL_FreeSurface(*gradient_surface);
    *gradient_surface = nullptr;
    return 2;
  }

  uint32_t red_mask = 0xFF;
  uint32_t green_mask = 0xFF;
  uint32_t blue_mask = 0xFF;

  layer %= 4;

  if (layer == 1) {
    red_mask = 0;
    green_mask = 0;
  } else if (layer == 2) {
    green_mask = 0;
    blue_mask = 0;
  } else if (layer == 3) {
    red_mask = 0;
    blue_mask = 0;
  }

  auto pixels = static_cast<uint32_t *>((*gradient_surface)->pixels);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x, ++pixels) {
      int x_normal = static_cast<int>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      int y_normal = static_cast<int>(static_cast<float>(y) * 255.0f / static_cast<float>(height));
      *pixels = SDL_MapRGBA((*gradient_surface)->format, y_normal & red_mask, x_normal & green_mask,
                            (255 - y_normal) & blue_mask, x_normal + y_normal);
    }
  }

  SDL_UnlockSurface(*gradient_surface);

  return 0;
}

static int GeneratePalettizedSurface(uint8_t **ret, uint32_t width, uint32_t height, uint32_t depth,
                                     TestHost::PaletteSize palette_size) {
  *ret = new uint8_t[width * height * depth];
  if (!(*ret)) {
    return 1;
  }

  auto pixel = *ret;

  uint32_t block_size = palette_size / 4;
  uint32_t offset = 0;
  for (auto d = 0; d < depth; ++d, offset += block_size) {
    for (auto y = 0; y < height; ++y) {
      for (auto x = 0; x < width; ++x) {
        *pixel++ = ((y % block_size) + offset) & (palette_size - 1);
      }
    }
  }

  return 0;
}

static uint32_t *GeneratePalette(TestHost::PaletteSize size) {
  auto ret = new uint32_t[size];

  uint32_t block_size = size / 4;
  auto component_inc = (uint32_t)ceilf(255.0f / (float)block_size);
  uint32_t i = 0;
  uint32_t component = 0;
  for (; i < block_size; ++i, component += component_inc) {
    uint32_t color_value = 0xFF - component;
    ret[i + block_size * 0] = 0xFF000000 + color_value;
    ret[i + block_size * 1] = 0xFF000000 + (color_value << 8);
    ret[i + block_size * 2] = 0xFF000000 + (color_value << 16);
    ret[i + block_size * 3] = 0xFF000000 + color_value + (color_value << 8) + (color_value << 16);
  }

  return ret;
}
