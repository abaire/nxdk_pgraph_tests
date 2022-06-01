#include "texture_border_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "swizzle.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "vertex_buffer.h"

static void GenerateBordered2DSurface(uint8_t *texture_memory, uint32_t width, uint32_t height, bool swizzle);
static int GeneratePalettized2DSurface(uint8_t **gradient_surface, int width, int height,
                                       TestHost::PaletteSize palette_size);
static uint32_t *GeneratePalette(TestHost::PaletteSize size);

static constexpr uint32_t kTextureWidth = 32;
static constexpr uint32_t kTexturePitch = kTextureWidth * 4;
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

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  // Generate a borderless texture as tex0
  {
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
    SDL_Surface *surface;
    auto update_texture_result = GenerateColoredCheckerboardSurface(&surface, kTextureWidth, kTextureHeight);
    ASSERT(!update_texture_result && "Failed to generate SDL surface");
    update_texture_result = host_.SetTexture(surface);
    SDL_FreeSurface(surface);
    ASSERT(!update_texture_result && "Failed to set texture");
  }

  // Generate a 4 texel bordered texture as tex1
  {
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8), 1);
    GenerateBordered2DSurface(host_.GetTextureMemoryForStage(1), kTextureWidth, kTextureHeight, true);
  }
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

    // The min/max s,t value will be 4 (the size of the border)
    static constexpr float kBorderMin = -4.0f / kTextureWidth;
    static constexpr float kBorderMax = 1.0 + (-kBorderMin);

    auto set_tex = [&vertex](float l, float t, float r, float b) {
#define BORDER_EXPAND(val) ((val) < 0.0f ? kBorderMin : ((val) > 1.0f ? kBorderMax : (val)))
      float border_l = BORDER_EXPAND(l);
      float border_r = BORDER_EXPAND(r);
      float border_t = BORDER_EXPAND(t);
      float border_b = BORDER_EXPAND(b);
#undef BORDER_EXPAND
      vertex->SetTexCoord0(l, t);
      vertex->SetTexCoord1(border_l, border_t);
      ++vertex;
      vertex->SetTexCoord0(r, t);
      vertex->SetTexCoord1(border_r, border_t);
      ++vertex;
      vertex->SetTexCoord0(r, b);
      vertex->SetTexCoord1(border_r, border_b);
      ++vertex;
      vertex->SetTexCoord0(l, t);
      vertex->SetTexCoord1(border_l, border_t);
      ++vertex;
      vertex->SetTexCoord0(r, b);
      vertex->SetTexCoord1(border_r, border_b);
      ++vertex;
      vertex->SetTexCoord0(l, b);
      vertex->SetTexCoord1(border_l, border_b);
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
  host_.PrepareDraw(0xFE202020);

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, false);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  {
    auto &stage = host_.GetTextureStage(0);
    stage.SetEnabled();
    stage.SetBorderColor(0xF0FF00FF);
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_TENT_LOD, TextureStage::MAG_TENT_LOD0);

    stage.SetUWrap(TextureStage::WRAP_REPEAT, false);
    stage.SetVWrap(TextureStage::WRAP_REPEAT, false);
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

    stage.SetBorderFromColor(true);
    stage.SetUWrap(TextureStage::WRAP_CLAMP_TO_EDGE_OGL, false);
    stage.SetVWrap(TextureStage::WRAP_CLAMP_TO_EDGE_OGL, false);
    host_.SetupTextureStages();
    host_.SetVertexBuffer(vertex_buffers_[5]);
    host_.DrawArrays();

    stage.SetEnabled(false);
  }

  // Render the bordered texture.
  {
    host_.SetTextureStageEnabled(0, false);
    host_.SetTextureStageEnabled(1, true);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);

    auto &stage = host_.GetTextureStage(1);
    stage.SetEnabled();
    stage.SetBorderColor(0xF0FF00FF);
    stage.SetBorderFromColor(false);
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_TENT_LOD, TextureStage::MAG_TENT_LOD0);

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);

    stage.SetUWrap(TextureStage::WRAP_BORDER, false);
    stage.SetVWrap(TextureStage::WRAP_BORDER, false);
    host_.SetupTextureStages();

    host_.SetVertexBuffer(vertex_buffers_[4]);
    host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE | TestHost::TEXCOORD1);
    stage.SetEnabled(false);
  }

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

static void GenerateBordered2DSurface(uint8_t *texture_memory, uint32_t width, uint32_t height, bool swizzle) {
  uint32_t border_width = swizzle ? width * 2 : width + 8;
  uint32_t border_height = swizzle ? height * 2 : height + 8;
  uint32_t full_pitch = border_width * 4;

  uint8_t *buffer = texture_memory;
  if (swizzle) {
    const uint32_t size = full_pitch * border_height;
    buffer = new uint8_t[size];
  }

  GenerateRGBACheckerboard(buffer, 0, 0, border_width, border_height, full_pitch, 0xFFCCCCCC, 0xFF444444, 2);

  static const uint32_t kColors[] = {
      0xFF0000FF, 0x660000FF, 0xFF00FF00, 0x6600FF00, 0xFFFF4444, 0x66FF4444, 0xFFFFFFFF, 0x66FFFFFF,
  };
  static const uint32_t kNumColors = sizeof(kColors) / sizeof(kColors[0]);
  GenerateColoredCheckerboard(buffer, 4, 4, width, height, full_pitch, kColors, kNumColors, 4);

  if (swizzle) {
    swizzle_rect(buffer, border_width, border_height, texture_memory, full_pitch, 4);
    delete[] buffer;
  }
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
