#include "texture_border_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "shaders/perspective_vertex_shader.h"
#include "swizzle.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "vertex_buffer.h"
#include "xbox_math_matrix.h"

using namespace XboxMath;

static void GenerateBordered2DSurface(uint8_t *texture_memory, uint32_t width, uint32_t height, bool swizzle);
// static int GeneratePalettized2DSurface(uint8_t **gradient_surface, int width, int height,
//                                        TestHost::PaletteSize palette_size);
// static uint32_t *GeneratePalette(TestHost::PaletteSize size);

static constexpr uint32_t kTextureWidth = 32;
static constexpr uint32_t kTextureHeight = 32;

static constexpr const char *kTest2D = "2D";
// static constexpr const char *kTest2DIndexed = "2D_Indexed";
static constexpr const char *kTest2DBorderedSwizzled = "2D_BorderTex_SZ";
static constexpr const char *kTest3DBorderedSwizzled = "3D_BorderTex_SZ";
static constexpr const char *kTestCubeMapBorderedSwizzled = "Cube_BorderTex_SZ";
static constexpr const char *kXemu1034 = "xemu#1034";

static constexpr uint32_t kDepthSlices = 4;

// clang-format off
static constexpr const uint32_t kBorderTextureSizes[][2] = {
    {1, 1}, {2, 2}, {4, 4}, {8, 8}, {16, 16}, {32, 32}, {16, 1}, {8, 2}, {4, 8},
};
static constexpr const uint32_t kNumBorderTextureSizes = sizeof(kBorderTextureSizes) / sizeof(kBorderTextureSizes[0]);
// clang-format on

// Must be ordered to match kCubeSTPoints for each face.
static constexpr uint32_t kRightSide[] = {3, 7, 6, 2};
static constexpr uint32_t kLeftSide[] = {1, 5, 4, 0};
static constexpr uint32_t kTopSide[] = {4, 5, 6, 7};
static constexpr uint32_t kBottomSide[] = {1, 0, 3, 2};
static constexpr uint32_t kBackSide[] = {2, 6, 5, 1};
static constexpr uint32_t kFrontSide[] = {0, 4, 7, 3};

// clang-format off
static constexpr const uint32_t *kCubeFaces[] = {
  kRightSide,
  kLeftSide,
  kTopSide,
  kBottomSide,
  kBackSide,
  kFrontSide,
};

static constexpr float kCubeSize = 1.0f;

static constexpr float kCubePoints[][3] = {
  {-kCubeSize, -kCubeSize, -kCubeSize},
  {-kCubeSize, -kCubeSize, kCubeSize},
  {kCubeSize, -kCubeSize, kCubeSize},
  {kCubeSize, -kCubeSize, -kCubeSize},
  {-kCubeSize, kCubeSize, -kCubeSize},
  {-kCubeSize, kCubeSize, kCubeSize},
  {kCubeSize, kCubeSize, kCubeSize},
  {kCubeSize, kCubeSize, -kCubeSize},
};

//static constexpr const float kCubeSTPoints[][4][2] = {
//  {{0.000000f, 0.000000f}, {0.000000f, 0.333333f}, {0.333333f, 0.333333f}, {0.333333f, 0.000000f}},
//  {{0.333333f, 0.000000f}, {0.333333f, 0.333333f}, {0.666667f, 0.333333f}, {0.666667f, 0.000000f}},
//  {{0.333333f, 0.333333f}, {0.333333f, 0.666667f}, {0.666667f, 0.666667f}, {0.666667f, 0.333333f}},
//  {{0.000000f, 0.333333f}, {0.000000f, 0.666667f}, {0.333333f, 0.666667f}, {0.333333f, 0.333333f}},
//  {{0.000000f, 0.666667f}, {0.000000f, 1.000000f}, {0.333333f, 1.000000f}, {0.333333f, 0.666667f}},
//  {{0.666667f, 0.000000f}, {0.666667f, 0.333333f}, {1.000000f, 0.333333f}, {1.000000f, 0.000000f}},
//};

// clang-format on

TextureBorderTests::TextureBorderTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture border", config) {
  tests_[kTest2D] = [this]() { Test2D(); };
  tests_[kTest2DBorderedSwizzled] = [this]() { Test2DBorderedSwizzled(); };
  //  tests_[kTest2DIndexed] = [this]() { Test2DPalettized(); };
  tests_[kXemu1034] = [this]() { TestXemu1034(); };

  for (auto size : kBorderTextureSizes) {
    char name[32] = {0};
    snprintf(name, sizeof(name), "%s_%dx%d", kTest3DBorderedSwizzled, size[0], size[1]);
    tests_[name] = [this, name, size]() { Test3DBorderedSwizzled(name, size[0], size[1]); };

    // Cube maps must be square or the hardware will raise an invalid data error.
    if (size[0] == size[1]) {
      snprintf(name, sizeof(name), "%s_%dx%d", kTestCubeMapBorderedSwizzled, size[0], size[1]);
      tests_[name] = [this, name, size]() { TestCubemapBorderedSwizzled(name, size[0], size[1]); };
    }
  }
}

void TextureBorderTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  // Generate a borderless texture as tex0
  {
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
    SDL_Surface *surface;
    auto update_texture_result = GenerateColoredCheckerboardSurface(&surface, kTextureWidth, kTextureHeight, 2);
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
    static constexpr float kBorderMax = 1.0f + (-kBorderMin);

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
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.PrepareDraw(0xFE202020);

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  {
    auto &stage = host_.GetTextureStage(0);
    stage.SetEnabled();
    stage.SetBorderColor(0xF0FF00FF);
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_TENT_LOD, TextureStage::MAG_BOX_LOD0);

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
    stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_TENT_LOD, TextureStage::MAG_BOX_LOD0);

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);

    stage.SetUWrap(TextureStage::WRAP_BORDER, false);
    stage.SetVWrap(TextureStage::WRAP_BORDER, false);
    host_.SetupTextureStages();

    host_.SetVertexBuffer(vertex_buffers_[4]);
    host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE | TestHost::TEXCOORD1);
    stage.SetEnabled(false);
  }

  pb_print("%s\n", kTest2D);
  pb_printat(1, 14, (char *)"Wrap");
  pb_printat(1, 26, (char *)"Mirror");
  pb_printat(1, 39, (char *)"Clamp");
  pb_printat(8, 12, (char *)"Border-C");
  pb_printat(8, 26, (char *)"Border-T");
  pb_printat(8, 39, (char *)"Clamp_OGl");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTest2D);
}

static void SetAddressMode(TextureStage::WrapMode mode) {
  Pushbuffer::Begin();
  uint32_t texture_address = MASK(NV097_SET_TEXTURE_ADDRESS_U, mode) | MASK(NV097_SET_TEXTURE_ADDRESS_V, mode) |
                             MASK(NV097_SET_TEXTURE_ADDRESS_P, mode);
  Pushbuffer::Push(NV20_TCL_PRIMITIVE_3D_TX_WRAP(0), texture_address);
  Pushbuffer::End();
}

void TextureBorderTests::TestXemu1034() {
  // xemu #1034: Textures that are reused with different wrap modes will only use the original wrap mode.
  // This requires some special handling as the normal path (committing changes via TestHost) causes xemu to consider
  // the texture dirty which forces it to apply the new wrapping modes.
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.PrepareDraw(0xFE202020);

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  {
    auto &stage = host_.GetTextureStage(0);
    stage.SetEnabled();
    stage.SetBorderFromColor(true);
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_TENT_LOD, TextureStage::MAG_BOX_LOD0);

    stage.SetUWrap(TextureStage::WRAP_REPEAT, false);
    stage.SetVWrap(TextureStage::WRAP_REPEAT, false);
    host_.SetupTextureStages();
    host_.SetVertexBuffer(vertex_buffers_[0]);
    host_.DrawArrays();

    SetAddressMode(TextureStage::WRAP_MIRROR);
    host_.SetVertexBuffer(vertex_buffers_[1]);
    host_.DrawArrays();

    SetAddressMode(TextureStage::WRAP_CLAMP_TO_EDGE);
    host_.SetVertexBuffer(vertex_buffers_[2]);
    host_.DrawArrays();

    {
      Pushbuffer::Begin();
      Pushbuffer::Push(NV097_SET_TEXTURE_BORDER_COLOR, 0xF0559FFF);
      Pushbuffer::End();
    }
    SetAddressMode(TextureStage::WRAP_BORDER);
    host_.SetVertexBuffer(vertex_buffers_[3]);
    host_.DrawArrays();

    {
      Pushbuffer::Begin();
      Pushbuffer::Push(NV097_SET_TEXTURE_BORDER_COLOR, 0xF0FF5533);
      Pushbuffer::End();
    }
    host_.SetVertexBuffer(vertex_buffers_[4]);
    host_.DrawArrays();

    SetAddressMode(TextureStage::WRAP_CLAMP_TO_EDGE_OGL);
    host_.SetVertexBuffer(vertex_buffers_[5]);
    host_.DrawArrays();

    stage.SetEnabled(false);
  }

  pb_print("%s\n", kXemu1034);
  pb_printat(1, 14, (char *)"Wrap");
  pb_printat(1, 26, (char *)"Mirror");
  pb_printat(1, 39, (char *)"Clamp");
  pb_printat(8, 12, (char *)"Border-C");
  pb_printat(8, 26, (char *)"Border-C2");
  pb_printat(8, 39, (char *)"Clamp_OGl");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kXemu1034);
}

// void TextureBorderTests::Test2DPalettized() {
//   auto &texture_format = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_I8_A8R8G8B8);
//   host_.SetTextureFormat(texture_format);
//
//   uint8_t *gradient_surface = nullptr;
//   int err = GeneratePalettized2DSurface(&gradient_surface, kTextureWidth, kTextureHeight, TestHost::PALETTE_256);
//   ASSERT(!err && "Failed to generate palettized surface");
//
//   err = host_.SetRawTexture(gradient_surface, host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight(), 1,
//                             host_.GetMaxTextureWidth(), 1, texture_format.xbox_swizzled);
//   delete[] gradient_surface;
//   ASSERT(!err && "Failed to set texture");
//
//   auto palette = GeneratePalette(TestHost::PALETTE_256);
//   err = host_.SetPalette(palette, TestHost::PALETTE_256);
//   delete[] palette;
//   ASSERT(!err && "Failed to set palette");
//
//   host_.PrepareDraw(0xFE202020);
//   host_.DrawArrays();
//
////  pb_draw_text_screen();
//
//  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTest2DIndexed);
//}

void TextureBorderTests::Test2DBorderedSwizzled() {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(0xFE181818);

  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, true);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);
  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8), 2);

  auto &stage = host_.GetTextureStage(2);
  stage.SetEnabled();
  stage.SetBorderColor(0xFFFF00FF);
  stage.SetBorderFromColor(false);
  stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_TENT_LOD, TextureStage::MAG_BOX_LOD0);
  stage.SetUWrap(TextureStage::WRAP_BORDER, false);
  stage.SetVWrap(TextureStage::WRAP_BORDER, false);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX2);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX2, true);

  // Render the bordered texture.
  static constexpr float kWidth = 64.0f;
  static constexpr float kHeight = 64.0f;
  static constexpr float kSpacing = 32.0f;

  const float kNumPerRow = floorf((host_.GetFramebufferWidthF() - kSpacing) / (kWidth + kSpacing));
  const float kStartColumn = kSpacing;
  const float kEndColumn = kStartColumn + (kWidth + kSpacing) * kNumPerRow;
  const float kNumRows = ceilf(static_cast<float>(kNumBorderTextureSizes) / kNumPerRow);
  static constexpr float kZ = 1.0f;

  pb_print("%s\n", kTest2DBorderedSwizzled);
  static constexpr int kTextLeft = 3;
  static constexpr int kTextInc = 9;
  static constexpr int kTextRowInc = 4;
  int text_row = 1;
  int text_col = kTextLeft;
  float left = kStartColumn;
  float top = host_.GetFramebufferHeightF() * 0.5f - ((kHeight + kSpacing) * kNumRows - kSpacing);

  // Draw once with st coords from (0,1)
  {
    for (auto size : kBorderTextureSizes) {
      GenerateBordered2DSurface(host_.GetTextureMemoryForStage(2), size[0], size[1], true);
      stage.SetTextureDimensions(size[0], size[1]);
      host_.SetupTextureStages();

      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord2(0.0f, 0.0f);
      host_.SetVertex(left, top, kZ, 1.0f);

      host_.SetTexCoord2(1.0f, 0.0f);
      host_.SetVertex(left + kWidth, top, kZ, 1.0f);

      host_.SetTexCoord2(1.0f, 1.0f);
      host_.SetVertex(left + kWidth, top + kHeight, kZ, 1.0f);

      host_.SetTexCoord2(0.0f, 1.0f);
      host_.SetVertex(left, top + kHeight, kZ, 1.0f);
      host_.End();

      pb_printat(text_row, text_col, (char *)"%dx%d", size[0], size[1]);

      left += kWidth + kSpacing;
      text_col += kTextInc;
      if (left >= kEndColumn) {
        left = kStartColumn;
        top += kHeight + kSpacing;
        text_col = kTextLeft;
        text_row += kTextRowInc;
      }
    }
  }

  top = host_.GetFramebufferHeightF() * 0.5f + kSpacing;
  left = kStartColumn;
  text_col = kTextLeft;
  text_row = 9;

  // Draw with st coords calculated to show the border
  {
    for (auto size : kBorderTextureSizes) {
      GenerateBordered2DSurface(host_.GetTextureMemoryForStage(2), size[0], size[1], true);
      stage.SetTextureDimensions(size[0], size[1]);
      host_.SetupTextureStages();

      const float kBorderMinS = -4.0f / static_cast<float>(size[0]);
      const float kBorderMinT = -4.0f / static_cast<float>(size[1]);
      const float kBorderMaxS = 1.0f + (-kBorderMinS);
      const float kBorderMaxT = 1.0f + (-kBorderMinT);

      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord2(kBorderMinS, kBorderMinT);
      host_.SetVertex(left, top, kZ, 1.0f);

      host_.SetTexCoord2(kBorderMaxS, kBorderMinT);
      host_.SetVertex(left + kWidth, top, kZ, 1.0f);

      host_.SetTexCoord2(kBorderMaxS, kBorderMaxT);
      host_.SetVertex(left + kWidth, top + kHeight, kZ, 1.0f);

      host_.SetTexCoord2(kBorderMinS, kBorderMaxT);
      host_.SetVertex(left, top + kHeight, kZ, 1.0f);
      host_.End();

      pb_printat(text_row, text_col, (char *)"B%dx%d", size[0], size[1]);

      left += kWidth + kSpacing;
      text_col += kTextInc;
      if (left >= kEndColumn) {
        left = kStartColumn;
        top += kHeight + kSpacing;
        text_col = kTextLeft;
        text_row += kTextRowInc;
      }
    }
  }

  host_.SetTextureStageEnabled(2, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTest2DBorderedSwizzled);
}

void TextureBorderTests::Test3DBorderedSwizzled(const std::string &name, uint32_t width, uint32_t height) {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(0xFE181818);

  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, true);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_3D_PROJECTIVE);
  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8), 2);
  GenerateBordered3DSurface(host_.GetTextureMemoryForStage(2), width, height, kDepthSlices, true);

  auto &stage = host_.GetTextureStage(2);
  stage.SetEnabled();
  stage.SetBorderColor(0xFF7777FF);
  stage.SetBorderFromColor(false);
  stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_BOX_LOD0, TextureStage::MAG_BOX_LOD0);
  stage.SetUWrap(TextureStage::WRAP_BORDER, false);
  stage.SetVWrap(TextureStage::WRAP_BORDER, false);
  stage.SetPWrap(TextureStage::WRAP_BORDER, false);
  stage.SetTextureDimensions(width, height, kDepthSlices);
  host_.SetupTextureStages();

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX2);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX2, true);

  // Render the bordered texture.
  static constexpr float kDepths[] = {
      0.0f / kDepthSlices, 0.5f / kDepthSlices, 1.0f / kDepthSlices, 1.5f / kDepthSlices, 2.0f / kDepthSlices,
      2.5f / kDepthSlices, 3.0f / kDepthSlices, 3.5f / kDepthSlices, 4.0f / kDepthSlices, 2.75f / kDepthSlices,
  };
  static constexpr uint32_t kNumDepths = sizeof(kDepths) / sizeof(kDepths[0]);

  static constexpr float kQuadSize = 64.0f;
  static constexpr float kSpacing = 32.0f;

  const float kNumPerRow = floorf((host_.GetFramebufferWidthF() - kSpacing) / (kQuadSize + kSpacing));
  const float kStartColumn = kSpacing;
  const float kEndColumn = kStartColumn + (kQuadSize + kSpacing) * kNumPerRow;
  const float kNumRows = ceilf(static_cast<float>(kNumDepths) / kNumPerRow);
  static constexpr float kZ = 1.0f;

  pb_print("%s\n", name.c_str());
  static constexpr int kTextLeft = 3;
  static constexpr int kTextInc = 9;
  static constexpr int kTextRowInc = 4;
  int text_row = 1;
  int text_col = kTextLeft;
  float left = kStartColumn;
  float top = host_.GetFramebufferHeightF() * 0.5f - ((kQuadSize + kSpacing) * kNumRows - kSpacing);

  // Draw once with st coords from (0,1)
  {
    for (auto depth : kDepths) {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord2(0.0f, 0.0f, depth, 1.0f);
      host_.SetVertex(left, top, kZ, 1.0f);

      host_.SetTexCoord2(1.0f, 0.0f, depth, 1.0f);
      host_.SetVertex(left + kQuadSize, top, kZ, 1.0f);

      host_.SetTexCoord2(1.0f, 1.0f, depth, 1.0f);
      host_.SetVertex(left + kQuadSize, top + kQuadSize, kZ, 1.0f);

      host_.SetTexCoord2(0.0f, 1.0f, depth, 1.0f);
      host_.SetVertex(left, top + kQuadSize, kZ, 1.0f);
      host_.End();

      char label[16] = {0};
      snprintf(label, sizeof(label), "%.02f", depth);
      pb_printat(text_row, text_col, (char *)"%s", label);

      left += kQuadSize + kSpacing;
      text_col += kTextInc;
      if (left >= kEndColumn) {
        left = kStartColumn;
        top += kQuadSize + kSpacing;
        text_col = kTextLeft;
        text_row += kTextRowInc;
      }
    }
  }

  top = host_.GetFramebufferHeightF() * 0.5f + kSpacing;
  left = kStartColumn;
  text_col = kTextLeft;
  text_row = 9;

  // Draw with st coords calculated to show the border
  {
    for (auto depth : kDepths) {
      const float kBorderMinS = -4.0f / static_cast<float>(width);
      const float kBorderMinT = -4.0f / static_cast<float>(height);
      const float kBorderMaxS = 1.0f + (-kBorderMinS);
      const float kBorderMaxT = 1.0f + (-kBorderMinT);

      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord2(kBorderMinS, kBorderMinT, depth, 1.0f);
      host_.SetVertex(left, top, kZ, 1.0f);

      host_.SetTexCoord2(kBorderMaxS, kBorderMinT, depth, 1.0f);
      host_.SetVertex(left + kQuadSize, top, kZ, 1.0f);

      host_.SetTexCoord2(kBorderMaxS, kBorderMaxT, depth, 1.0f);
      host_.SetVertex(left + kQuadSize, top + kQuadSize, kZ, 1.0f);

      host_.SetTexCoord2(kBorderMinS, kBorderMaxT, depth, 1.0f);
      host_.SetVertex(left, top + kQuadSize, kZ, 1.0f);
      host_.End();

      char label[16] = {0};
      snprintf(label, sizeof(label), "B%.02f", depth);
      pb_printat(text_row, text_col, (char *)"%s", label);

      left += kQuadSize + kSpacing;
      text_col += kTextInc;
      if (left >= kEndColumn) {
        left = kStartColumn;
        top += kQuadSize + kSpacing;
        text_col = kTextLeft;
        text_row += kTextRowInc;
      }
    }
  }

  host_.SetTextureStageEnabled(2, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void TextureBorderTests::TestCubemapBorderedSwizzled(const std::string &name, uint32_t width, uint32_t height) {
  const float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                          0.0f, depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  {
    shader->SetLightingEnabled(false);
    shader->SetUseD3DStyleViewport();
    vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
    vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);
  }
  host_.SetVertexShaderProgram(shader);

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8), 3);
  GenerateBorderedCubemapSurface(host_.GetTextureMemoryForStage(3), width, height, true);

  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                              TestHost::STAGE_CUBE_MAP);

  auto &stage = host_.GetTextureStage(3);
  stage.SetEnabled();
  stage.SetCubemapEnable();
  stage.SetBorderColor(0xFF7777FF);
  stage.SetBorderFromColor(false);
  stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_TENT_TENT_LOD, TextureStage::MAG_BOX_LOD0);
  stage.SetUWrap(TextureStage::WRAP_BORDER, false);
  stage.SetVWrap(TextureStage::WRAP_BORDER, false);
  stage.SetPWrap(TextureStage::WRAP_BORDER, false);
  stage.SetTextureDimensions(width, height);
  host_.SetupTextureStages();

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);

  host_.PrepareDraw(0xFE121212);

  auto draw = [this, &shader, width](float x, float y, float z, float r_x, float r_y, float r_z,
                                     bool include_border = false) {
    matrix4_t matrix;
    vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
    vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
    vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
    TestHost::BuildD3DModelViewMatrix(matrix, eye, at, up);

    auto &model_matrix = shader->GetModelMatrix();
    MatrixSetIdentity(model_matrix);
    vector_t rotation = {r_x, r_y, r_z};
    MatrixRotate(model_matrix, rotation);
    vector_t translation = {x, y, z};
    MatrixTranslate(model_matrix, translation);

    matrix4_t mv_matrix;
    MatrixMultMatrix(matrix, shader->GetModelMatrix(), mv_matrix);
    host_.SetFixedFunctionModelViewMatrix(mv_matrix);

    shader->PrepareDraw();

    // The min/max s,t value will be 4 (the size of the border)
    const float kBorderMin = -4.0f / static_cast<float>(width);
    const float kBorderMax = 1.0f + (-kBorderMin);

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    for (auto face : kCubeFaces) {
      for (auto i = 0; i < 4; ++i) {
        uint32_t index = face[i];
        const float *vertex = kCubePoints[index];
        float s = vertex[0];
        float t = vertex[1];
        if (include_border) {
          s = s <= 0.0f ? kBorderMin : kBorderMax;
          t = t <= 0.0f ? kBorderMin : kBorderMax;
        }
        host_.SetTexCoord3(s, t, vertex[2], 1.0f);
        host_.SetVertex(vertex[0], vertex[1], vertex[2], 1.0f);
      }
    }

    host_.End();
  };

  const float z = 2.0f;
  draw(-1.5f, 1.5f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  draw(1.5f, 1.5f, z, M_PI * 1.25f, M_PI * 0.25f, 0.0f);

  draw(-1.5f, -1.5f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f, true);
  draw(1.5f, -1.5f, z, M_PI * 1.25f, M_PI * 0.25f, 0.0f, true);

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

static void GenerateBordered2DSurface(uint8_t *texture_memory, uint32_t width, uint32_t height, bool swizzle) {
  uint32_t bordered_width = swizzle ? (width >= 8 ? width * 2 : 16) : width + 8;
  uint32_t bordered_height = swizzle ? (height >= 8 ? height * 2 : 16) : height + 8;
  uint32_t full_pitch = bordered_width * 4;

  uint8_t *buffer = texture_memory;
  if (swizzle) {
    const uint32_t size = full_pitch * bordered_height;
    buffer = new uint8_t[size];
  }

  GenerateRGBACheckerboard(buffer, 0, 0, bordered_width, bordered_height, full_pitch, 0xFFCCCCCC, 0xFF444444, 2);

  static const uint32_t kColors[] = {
      0xFF0000FF, 0x660000FF, 0xFF00FF00, 0x6600FF00, 0xFFFF4444, 0x66FF4444, 0xFFFFFFFF, 0x66FFFFFF,
  };
  static const uint32_t kNumColors = sizeof(kColors) / sizeof(kColors[0]);
  GenerateColoredCheckerboard(buffer, 4, 4, width, height, full_pitch, kColors, kNumColors, 2);

  if (swizzle) {
    swizzle_rect(buffer, bordered_width, bordered_height, texture_memory, full_pitch, 4);
    delete[] buffer;
  }
}

void TextureBorderTests::GenerateBordered3DSurface(uint8_t *texture_memory, uint32_t width, uint32_t height,
                                                   uint32_t depth, bool swizzle) const {
  const uint32_t bordered_width = swizzle ? (width >= 8 ? width * 2 : 16) : width + 8;
  const uint32_t bordered_height = swizzle ? (height >= 8 ? height * 2 : 16) : height + 8;
  const uint32_t bordered_depth = swizzle ? (depth >= 8 ? depth * 2 : 16) : depth + 8;
  const uint32_t full_pitch = bordered_width * 4;
  const uint32_t full_layer_pitch = full_pitch * bordered_height;
  const uint32_t size = full_layer_pitch * bordered_depth;

  ASSERT(full_layer_pitch * depth < host_.GetMaxSingleTextureSize());

  uint8_t *buffer = texture_memory;
  if (swizzle) {
    buffer = new uint8_t[size];
  }

  static constexpr uint32_t kMasks[] = {
      0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFF00FF, 0xFFFFFF00, 0xFF00FFFF,
  };
  static constexpr uint32_t kNumMasks = sizeof(kMasks);

  auto target = buffer;

  // Leading border layers.
  for (uint32_t i = 0; i < 4; ++i, target += full_layer_pitch) {
    GenerateRGBACheckerboard(target, 0, 0, bordered_width, bordered_height, full_pitch, 0xFFCCCCCC, 0xFF444444, 2);
  }

  for (uint32_t i = 0; i < depth; ++i, target += full_layer_pitch) {
    uint32_t mask = kMasks[i % kNumMasks];
    uint32_t high = 0xFFFFFFFF & mask;
    uint32_t low = 0xFF777777 & mask;

    GenerateRGBACheckerboard(target, 0, 0, bordered_width, bordered_height, full_pitch, 0xFFCCCCCC, 0xFF444444, 2);
    GenerateRGBACheckerboard(target, 4, 4, width, height, full_pitch, high, low, 1);
  }

  // Trailing border layers.
  for (uint32_t i = 0; i < 4; ++i, target += full_layer_pitch) {
    GenerateRGBACheckerboard(target, 0, 0, bordered_width, bordered_height, full_pitch, 0xFFCCCCCC, 0xFF444444, 2);
  }

  if (swizzle) {
    swizzle_box(buffer, bordered_width, bordered_height, bordered_depth, texture_memory, full_pitch, full_layer_pitch,
                4);
    delete[] buffer;
  }
}

void TextureBorderTests::GenerateBorderedCubemapSurface(uint8_t *texture_memory, uint32_t width, uint32_t height,
                                                        bool swizzle) const {
  const uint32_t bordered_width = swizzle ? (width >= 8 ? width * 2 : 16) : width + 8;
  const uint32_t bordered_height = swizzle ? (height >= 8 ? height * 2 : 16) : height + 8;
  const uint32_t full_pitch = bordered_width * 4;
  const uint32_t full_layer_pitch = full_pitch * bordered_height;

  ASSERT(full_layer_pitch * 6 < host_.GetMaxSingleTextureSize());

  uint8_t *buffer = nullptr;
  if (swizzle) {
    buffer = new uint8_t[full_layer_pitch];
  }

  static constexpr uint32_t kMasks[] = {
      0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFF00FF, 0xFFFFFF00, 0xFF00FFFF,
  };
  static constexpr uint32_t kNumMasks = sizeof(kMasks);

  auto target = texture_memory;
  for (uint32_t i = 0; i < 6; ++i, target += full_layer_pitch) {
    const uint32_t high = 0xFFFFFFFF & kMasks[i % kNumMasks];
    const uint32_t low = 0xFF777777 & kMasks[i % kNumMasks];
    auto dest = swizzle ? buffer : target;
    GenerateRGBACheckerboard(dest, 0, 0, bordered_width, bordered_height, full_pitch, 0xFFCCCCCC, 0xFF444444, 2);
    GenerateRGBACheckerboard(dest, 4, 4, width, height, full_pitch, high, low, 1);

    if (swizzle) {
      swizzle_rect(dest, bordered_width, bordered_height, target, full_pitch, 4);
    }
  }

  if (buffer) {
    delete[] buffer;
  }
}

// static int GeneratePalettized2DSurface(uint8_t **gradient_surface, int width, int height,
//                                        TestHost::PaletteSize palette_size) {
//   *gradient_surface = new uint8_t[width * height];
//   if (!(*gradient_surface)) {
//     return 1;
//   }
//
//   auto pixel = *gradient_surface;
//
//   uint32_t total_size = width * height;
//   uint32_t half_size = total_size >> 1;
//
//   for (uint32_t i = 0; i < half_size; ++i, ++pixel) {
//     *pixel = i & (palette_size - 1);
//   }
//
//   for (uint32_t i = half_size; i < total_size; i += 4) {
//     uint8_t value = i & (palette_size - 1);
//     *pixel++ = value;
//     *pixel++ = value;
//     *pixel++ = value;
//     *pixel++ = value;
//   }
//   return 0;
// }

// static uint32_t *GeneratePalette(TestHost::PaletteSize size) {
//   auto ret = new uint32_t[size];
//
//   uint32_t block_size = size / 4;
//   auto component_inc = (uint32_t)ceilf(255.0f / (float)block_size);
//   uint32_t i = 0;
//   uint32_t component = 0;
//   for (; i < block_size; ++i, component += component_inc) {
//     uint32_t color_value = 0xFF - component;
//     ret[i + block_size * 0] = 0xFF000000 + color_value;
//     ret[i + block_size * 1] = 0xFF000000 + (color_value << 8);
//     ret[i + block_size * 2] = 0xFF000000 + (color_value << 16);
//     ret[i + block_size * 3] = 0xFF000000 + color_value + (color_value << 8) + (color_value << 16);
//   }
//
//   return ret;
// }
