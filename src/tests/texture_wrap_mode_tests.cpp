#include "texture_wrap_mode_tests.h"

#include <texture_generator.h>

#include "test_host.h"

static constexpr char kCylWrapTestName[] = "CylinderWrapping";

static constexpr uint32_t kTextureSize = 256;

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc CylinderWrapping
 *   Tests the cylinder wrap settings of NV097_SET_TEXTURE_ADDRESS by rendering
 *   a simple quad with small and large jumps in UV coordinates across the 1.0
 *   boundary.
 */
TextureWrapModeTests::TextureWrapModeTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "TextureWrapMode", config) {
  tests_[kCylWrapTestName] = [this]() { TestCylinderWrapping(); };
}

void TextureWrapModeTests::Initialize() {
  TestSuite::Initialize();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  GenerateSwizzledRGBRadialGradient(host_.GetTextureMemoryForStage(0), kTextureSize, kTextureSize);
}

void TextureWrapModeTests::TestCylinderWrapping() {
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    texture_stage.SetUWrap(TextureStage::WRAP_REPEAT, false);
    texture_stage.SetVWrap(TextureStage::WRAP_MIRROR, false);
  }
  host_.SetupTextureStages();

  host_.PrepareDraw();

  static constexpr float kQuadSize = 128.f;
  static constexpr float kLeftMargin = 96.f;
  static constexpr float kHSpacing = kQuadSize + 64.f;
  float left = kLeftMargin;
  float top = 96.f;

  static constexpr float kBigDelta = 1.f;
  static constexpr float kSmallDelta = 0.2f;

  auto set_cylinder_wrap = [this](bool enabled) {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetUWrap(TextureStage::WRAP_REPEAT, enabled);
    texture_stage.SetVWrap(TextureStage::WRAP_MIRROR, enabled);
    host_.SetupTextureStages();
  };

  auto draw_quad = [this](float left, float top, float d_u, float d_v, float u0, float v0) {
    host_.DrawTexturedScreenQuadEx(left, top, left + kQuadSize, top + kQuadSize, 1.f, u0, v0, u0 + d_u, v0, u0 + d_u,
                                   v0 + d_v, u0, v0 + d_v);
  };

  {
    static constexpr float kUVOffset = 1.f - (kBigDelta * 0.5f);
    set_cylinder_wrap(false);
    draw_quad(left, top, kBigDelta, kBigDelta, kUVOffset, kUVOffset);
    left += kQuadSize + kHSpacing;

    set_cylinder_wrap(true);
    draw_quad(left, top, kBigDelta, kBigDelta, kUVOffset, kUVOffset);
  }

  left = kLeftMargin;
  top += kQuadSize + 16.f;

  {
    static constexpr float kUVOffset = 1.f - (kSmallDelta * 0.5f);
    set_cylinder_wrap(false);
    draw_quad(left, top, kSmallDelta, kSmallDelta, kUVOffset, kUVOffset);
    left += kQuadSize + kHSpacing;

    set_cylinder_wrap(true);
    draw_quad(left, top, kSmallDelta, kSmallDelta, kUVOffset, kUVOffset);
  }

  pb_printat(0, 0, "%s\n", kCylWrapTestName);
  pb_printat(1, 16, "U: Repeat  V: Mirror");
  pb_printat(2, 12, "NORMAL");
  pb_printat(2, 42, "CYLWRAP");
  pb_printat(5, 0, "dBig");
  pb_printat(10, 0, "dSmall");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kCylWrapTestName);

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetupTextureStages();
}
