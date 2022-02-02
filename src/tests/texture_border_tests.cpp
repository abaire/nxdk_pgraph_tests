#include "texture_border_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "test_host.h"
#include "texture_format.h"
#include "vertex_buffer.h"

static int Generate2DSurface(SDL_Surface **gradient_surface, int width, int height);
static int GeneratePalettized2DSurface(uint8_t **gradient_surface, int width, int height,
                                       TestHost::PaletteSize palette_size);
static uint32_t *GeneratePalette(TestHost::PaletteSize size);

static constexpr uint32_t kTextureWidth = 32;
static constexpr uint32_t kTextureHeight = 32;

static constexpr const char *kTest2D = "2D";
static constexpr const char *kTest2DIndexed = "2D_Indexed";

TextureBorderTests::TextureBorderTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture border") {
  tests_[kTest2D] = [this]() { Test2D(); };
  //  tests_[kTest2DIndexed] = [this]() { Test2DPalettized(); };
}

void TextureBorderTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

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

void TextureBorderTests::CreateGeometry() {
  static constexpr float kLeft = -2.75f;
  static constexpr float kRight = 2.75f;
  static constexpr float kTop = 1.75f;
  static constexpr float kBottom = -1.75f;
  static const float kSpacing = 0.1f;
  static const float kWidth = kRight - kLeft;
  static const float kHeight = kTop - kBottom;
  static const float kUnitWidth = (kWidth - (2.0f * kSpacing)) / 3.0f;
  static const float kUnitHeight = (kHeight - kSpacing) / 2.0f;
  static const float kVSpacing = kUnitHeight * 0.25f;

  static const float kSubQuadWidth = kUnitWidth / 3.0f;
  static const float kSubQuadHeight = kUnitHeight / 3.0f;

  float left = kLeft;
  float top = kTop + (kVSpacing + kSpacing);
  for (auto &buffer : vertex_buffers_) {
    uint32_t num_quads = 9;
    buffer = host_.AllocateVertexBuffer(6 * num_quads);

    const float z = 1.0f;
    uint32_t index = 0;
    float x = left;
    float y = top;

    for (uint32_t i = 0; i < 3; ++i) {
      buffer->DefineBiTri(index++, x, y, x + kSubQuadWidth, y - kSubQuadHeight, z);
      x += kSubQuadWidth;
      buffer->DefineBiTri(index++, x, y, x + kSubQuadWidth, y - kSubQuadHeight, z);
      x += kSubQuadWidth;
      buffer->DefineBiTri(index++, x, y, x + kSubQuadWidth, y - kSubQuadHeight, z);

      x = left;
      y -= kSubQuadHeight;
    }

    auto *vertex = buffer->Lock();

    auto set_tex = [&vertex](float l, float t, float r, float b) {
      vertex->SetTexCoord0(l, t);
      ++vertex;
      vertex->SetTexCoord0(r, t);
      ++vertex;
      vertex->SetTexCoord0(r, b);
      ++vertex;
      vertex->SetTexCoord0(l, t);
      ++vertex;
      vertex->SetTexCoord0(r, b);
      ++vertex;
      vertex->SetTexCoord0(l, b);
      ++vertex;
    };

    x = -1.0f;
    y = -1.0f;
    for (uint32_t i = 0; i < 3; ++i) {
      set_tex(x, y, x + 1.0f, y + 1.0f);
      x += 1.0f;
      set_tex(x, y, x + 1.0f, y + 1.0f);
      x += 1.0f;
      set_tex(x, y, x + 1.0f, y + 1.0f);

      x = -1.0f;
      y += 1.0f;
    }

    buffer->Unlock();

    left += kUnitWidth + kSpacing;
    if (left > (kRight - kUnitWidth * 0.5f)) {
      left = kLeft;
      top = -kSpacing * 2.0f;
    }
  }
}

void TextureBorderTests::Test2D() {
  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));

  SDL_Surface *surface;
  int update_texture_result = Generate2DSurface(&surface, kTextureWidth, kTextureHeight);
  ASSERT(!update_texture_result && "Failed to generate SDL surface");

  update_texture_result = host_.SetTexture(surface);
  SDL_FreeSurface(surface);
  ASSERT(!update_texture_result && "Failed to set texture");

  host_.PrepareDraw(0xFE202020);

  auto &stage = host_.GetTextureStage(0);
  stage.SetEnabled();
  stage.SetBorderColor(0xF00000CC);
  stage.SetDimensions(kTextureWidth, kTextureHeight);

  stage.SetUWrap(TextureStage::WRAP_WRAP, false);
  stage.SetVWrap(TextureStage::WRAP_WRAP, false);
  host_.SetupTextureStages();
  host_.SetVertexBuffer(vertex_buffers_[0]);
  host_.DrawArrays();

  stage.SetUWrap(TextureStage::WRAP_MIRROR, false);
  stage.SetVWrap(TextureStage::WRAP_MIRROR, false);
  host_.SetupTextureStages();
  host_.SetVertexBuffer(vertex_buffers_[1]);
  host_.DrawArrays();

  stage.SetUWrap(TextureStage::WRAP_CLAMP_TO_EDGE, false);
  stage.SetVWrap(TextureStage::WRAP_CLAMP_TO_EDGE, false);
  host_.SetupTextureStages();
  host_.SetVertexBuffer(vertex_buffers_[2]);
  host_.DrawArrays();

  stage.SetBorderFromColor(true);
  stage.SetUWrap(TextureStage::WRAP_BORDER, false);
  stage.SetVWrap(TextureStage::WRAP_BORDER, false);
  host_.SetupTextureStages();
  host_.SetVertexBuffer(vertex_buffers_[3]);
  host_.DrawArrays();

  stage.SetBorderFromColor(false);
  stage.SetUWrap(TextureStage::WRAP_BORDER, false);
  stage.SetVWrap(TextureStage::WRAP_BORDER, false);
  host_.SetupTextureStages();
  host_.SetVertexBuffer(vertex_buffers_[4]);
  host_.DrawArrays();

  stage.SetUWrap(TextureStage::WRAP_CLAMP_TO_EDGE_OGL, false);
  stage.SetVWrap(TextureStage::WRAP_CLAMP_TO_EDGE_OGL, false);
  host_.SetupTextureStages();
  host_.SetVertexBuffer(vertex_buffers_[5]);
  host_.DrawArrays();

  pb_printat(1, 14, (char *)"Wrap");
  pb_printat(1, 26, (char *)"Mirror");
  pb_printat(1, 39, (char *)"Clamp");
  pb_printat(8, 12, (char *)"Border-C");
  pb_printat(8, 26, (char *)"Border-T");
  pb_printat(8, 39, (char *)"Clamp_OGl");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kTest2D);
}

void TextureBorderTests::Test2DPalettized() {
#if 0
  auto &texture_format = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8);
  host_.SetTextureFormat(texture_format);

  uint8_t *gradient_surface = nullptr;
  int err = GeneratePalettized2DSurface(&gradient_surface, kTextureWidth, kTextureHeight, TestHost::PALETTE_256);
  ASSERT(!err && "Failed to generate palettized surface");

  err = host_.SetRawTexture(gradient_surface, host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight(), 1,
                            host_.GetMaxTextureWidth(), 1, texture_format.xbox_swizzled);
  delete[] gradient_surface;
  ASSERT(!err && "Failed to set texture");

  auto palette = GeneratePalette(TestHost::PALETTE_256);
  err = host_.SetPalette(palette, TestHost::PALETTE_256);
  delete[] palette;
  ASSERT(!err && "Failed to set palette");

  host_.PrepareDraw(0xFE202020);
  host_.DrawArrays();

//  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kTest2DIndexed);
#endif
}

static int Generate2DSurface(SDL_Surface **gradient_surface, int width, int height) {
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

  auto pixels = static_cast<uint32_t *>((*gradient_surface)->pixels);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x += 4) {
      int index = (x / 4) + (y / 4);

      uint32_t pixel = kColors[index % kNumColors];
      *pixels++ = pixel;
      *pixels++ = pixel;
      *pixels++ = pixel;
      *pixels++ = pixel;
    }
  }

  SDL_UnlockSurface(*gradient_surface);

  return 0;
}

static int GeneratePalettized2DSurface(uint8_t **gradient_surface, int width, int height,
                                       TestHost::PaletteSize palette_size) {
#if 0
  *gradient_surface = new uint8_t[width * height];
  if (!(*gradient_surface)) {
    return 1;
  }

  auto pixel = *gradient_surface;

  uint32_t total_size = width * height;
  uint32_t half_size = total_size >> 1;

  for (uint32_t i = 0; i < half_size; ++i, ++pixel) {
    *pixel = i & (palette_size - 1);
  }

  for (uint32_t i = half_size; i < total_size; i += 4) {
    uint8_t value = i & (palette_size - 1);
    *pixel++ = value;
    *pixel++ = value;
    *pixel++ = value;
    *pixel++ = value;
  }

#endif
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
