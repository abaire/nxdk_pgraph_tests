#include "texture_anisotropy_tests.h"

#include "test_host.h"
#include "texture_generator.h"

static constexpr char kAnisotropyTest[] = "Anisotropy";

static std::string MakeTestName(uint32_t anisotropy_shift) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%s-%d", kAnisotropyTest, (1 << anisotropy_shift));
  return buf;
}

/**
 * Constructs the test suite and creates test cases.
 *
 * @tc Anisotropy-1
 *   Renders a flat, checkerboard-textured quad with perspective and anisotropic filtering set to off.
 *
 * @tc Anisotropy-2
 *   Renders a flat, checkerboard-textured quad with perspective and anisotropic filtering set to 2x.
 *
 * @tc Anisotropy-4
 *   Renders a flat, checkerboard-textured quad with perspective and anisotropic filtering set to 4x.
 *
 * @tc Anisotropy-8
 *   Renders a flat, checkerboard-textured quad with perspective and anisotropic filtering set to 8x.
 *
 */
TextureAnisotropyTests::TextureAnisotropyTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture anisotropy", config) {
  for (uint32_t i = 0; i < 4; ++i) {
    tests_[MakeTestName(i)] = [this, i]() { Test(i); };
  }
}

void TextureAnisotropyTests::Initialize() {
  TestSuite::Initialize();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void TextureAnisotropyTests::Test(uint32_t anisotropy_shift) {
  static constexpr uint32_t kTextureSize = 128;
  static constexpr float kCheckerboardBoxSize = 4.f;

  static constexpr float kQuadYPosition = -1.5f;
  static constexpr float kQuadNearZ = 200.f;
  static constexpr float kQuadFarZ = 0.f;

  // The texture is repeated a number of times to guarantee that anisotropy will have a visible effect.
  static constexpr float kTextureVRepeat = 32.0f;

  static constexpr float kQuadWidth = 8.0f;

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  PBKitPlusPlus::GenerateSwizzledRGBACheckerboard(host_.GetTextureMemoryForStage(0), 0, 0, kTextureSize, kTextureSize,
                                                  kTextureSize * 4, 0xFFFFFF00, 0xFF663333, kCheckerboardBoxSize);

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  texture_stage.SetUWrap(TextureStage::WRAP_REPEAT);
  texture_stage.SetVWrap(TextureStage::WRAP_REPEAT);
  texture_stage.SetAnisotropy(1 << anisotropy_shift);
  host_.SetupTextureStages();

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  host_.PrepareDraw(0xFE202020);

  const float left = -kQuadWidth * 0.5f;
  const float right = -left;

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetTexCoord0(0.0f, 0.0f);
  host_.SetVertex(left, kQuadYPosition, kQuadNearZ);

  host_.SetTexCoord0(1.0f, 0.0f);
  host_.SetVertex(right, kQuadYPosition, kQuadNearZ);

  host_.SetTexCoord0(1.0f, kTextureVRepeat);
  host_.SetVertex(right, kQuadYPosition, kQuadFarZ);

  host_.SetTexCoord0(0.0f, kTextureVRepeat);
  host_.SetVertex(left, kQuadYPosition, kQuadFarZ);

  host_.End();

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  auto test_name = MakeTestName(anisotropy_shift);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, test_name);
}
