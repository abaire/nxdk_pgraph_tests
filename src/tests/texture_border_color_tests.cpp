#include "texture_border_color_tests.h"

#include <pbkit/pbkit.h>

#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"

static constexpr const char* kTestName = "TextureBorderColor";

static constexpr uint32_t kTextureSize = 8;
static constexpr uint32_t kTestBorderColor = 0xAADDEEBB;
static constexpr auto kQuadSize = 64.f;
static constexpr auto kTextureByteValue = 0x80;

struct TextureFormat {
  uint32_t format;
  const char* name;
  bool is_swizzled;
};

static constexpr TextureFormat kTestFormats[] = {
    {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8, "SZ_A8", true},

    {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R5G6B5, "SZ_RG6B", true},
    {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8B8, "SZ_RB", true},
    {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_G8B8, "SZ_GB", true},
    {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R6G5B5, "SZ_R6GB", true},

    {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8, "SZ_ARGB", true},
    {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8, "SZ_XRGB", true},
    {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8, "SZ_ABGR", true},
    {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_B8G8R8A8, "SZ_BGRA", true},
    {NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8G8B8A8, "SZ_RGBA", true},

    {NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8, "LU_A8", false},

    {NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R5G6B5, "LU_RG6B", false},
    // TODO: Implement in xemu
    //{NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R8B8, "LU_RB", false},
    {NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_G8B8, "LU_GB", false},
    {NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X1R5G5B5, "LU_XRGB5", false},

    {NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8, "LU_ARGB", false},
    {NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8, "LU_XRGB", false},
    {NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8B8G8R8, "LU_ABGR", false},
    {NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_B8G8R8A8, "LU_BGRA", false},
    {NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R8G8B8A8, "LU_RGBA", false},

};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc TextureBorderColor
 *   Demonstrates that texture format has no effect on texture border color.
 */
TextureBorderColorTests::TextureBorderColorTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Texture border color", config) {
  tests_[kTestName] = [this]() { Test(); };
}

void TextureBorderColorTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void TextureBorderColorTests::Test() {
  static constexpr uint32_t kBackgroundColor = 0xFF101010;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawCheckerboardUnproject(0xFF333333, 0xFF444444);

  host_.PBKitBusyWait();
  memset(host_.GetTextureMemoryForStage(0), kTextureByteValue, kTextureSize * kTextureSize * 4);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  auto& stage = host_.GetTextureStage(0);
  stage.SetBorderColor(kTestBorderColor);
  stage.SetTextureDimensions(kTextureSize, kTextureSize);
  stage.SetImageDimensions(kTextureSize, kTextureSize);
  stage.SetBorderFromColor(true);
  stage.SetUWrap(TextureStage::WRAP_BORDER, false);
  stage.SetVWrap(TextureStage::WRAP_BORDER, false);
  host_.SetTextureStageEnabled(0, true);

  auto draw_quad = [this](float left, float top, bool swizzled) {
    const float tex_min = swizzled ? -0.5f : kTextureSize * -0.5f;
    const float tex_max = swizzled ? 1.5f : kTextureSize - tex_min;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(tex_min, tex_min);
    host_.SetScreenVertex(left, top);

    host_.SetTexCoord0(tex_max, tex_min);
    host_.SetScreenVertex(left + kQuadSize, top);

    host_.SetTexCoord0(tex_max, tex_max);
    host_.SetScreenVertex(left + kQuadSize, top + kQuadSize);

    host_.SetTexCoord0(tex_min, tex_max);
    host_.SetScreenVertex(left, top + kQuadSize);
    host_.End();
  };

  const auto kQuadSpacing = floor(kQuadSize + kQuadSize * 0.5f) + 3.f;
  const auto kRight = host_.GetFramebufferWidthF() - kQuadSpacing;

  float left = kQuadSize;
  float top = 96.f;
  int text_x = 4;
  int text_y = 2;

  for (auto& format : kTestFormats) {
    host_.SetTextureFormat(GetTextureFormatInfo(format.format), 0);
    host_.SetupTextureStages();
    draw_quad(left, top, format.is_swizzled);

    pb_printat(text_y, text_x, "%s", format.name);

    left += kQuadSpacing;
    text_x += 10;

    if (left > kRight) {
      left = kQuadSize;
      top += kQuadSpacing;
      text_x = 4;
      text_y += 4;
    }
  }

  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetTextureStageEnabled(0, false);

  pb_printat(0, 0, "%s - Tex 0x%2X+ Border 0x%X", kTestName, kTextureByteValue, kTestBorderColor);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestName);
}
