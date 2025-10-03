#include "blend_surface_tests.h"

#include "texture_generator.h"

struct SurfaceFormatBlendTest {
  const char *name;
  TestHost::SurfaceColorFormat format;
};

static constexpr struct SurfaceFormatBlendTest kSurfaceFormatBlendTests[] = {
    {"X_Z1RGB5", TestHost::SCF_X1R5G5B5_Z1R5G5B5},
    {"X_O1RGB5", TestHost::SCF_X1R5G5B5_O1R5G5B5},
    {"R5G6B5", TestHost::SCF_R5G6B5},
    {"X_ZRGB8", TestHost::SCF_X8R8G8B8_Z8R8G8B8},
    {"X_ORGB8", TestHost::SCF_X8R8G8B8_O8R8G8B8},
    {"XA_Z1A7RGB8", TestHost::SCF_X1A7R8G8B8_Z1A7R8G8B8},
    {"XA_O1A7RGB8", TestHost::SCF_X1A7R8G8B8_O1A7R8G8B8},
    {"ARGB8", TestHost::SCF_A8R8G8B8},
};

static constexpr uint32_t kTextureSize = 128;

/**
 * Initializes the test suite and creates test cases.
 */
BlendSurfaceTests::BlendSurfaceTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Blend surface", config) {
  for (const auto &test : kSurfaceFormatBlendTests) {
    tests_[test.name] = [this, &test] { Test(test.name, test.format); };
  }
}

void BlendSurfaceTests::Initialize() {
  TestSuite::Initialize();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_ALPHA_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::End();
}

void BlendSurfaceTests::Test(const std::string &name, TestHost::SurfaceColorFormat surface_format) {
  host_.PrepareDraw(0xFF050505);

  // The number of partially overlapped quads to blend against each other.
  static constexpr uint32_t kNumBlends = 4;

  static constexpr uint32_t kSwatchSize = kTextureSize / 2;
  GenerateSwizzledRGBRadialATestPattern(host_.GetTextureMemoryForStage(1), kSwatchSize, kSwatchSize);

  auto prepare_texture = [this, surface_format](uint32_t blend_func, uint32_t sfactor, uint32_t dfactor) {
    host_.RenderToSurfaceStart(host_.GetTextureMemoryForStage(0), surface_format, kTextureSize, kTextureSize);

    // Set the initial background alpha.
    {
      static constexpr float kBackgroundAlpha = 0.7f;
      host_.SetBlend(false);
      host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
      host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
      host_.SetDiffuse(0.1f, 0.1f, 0.1f, kBackgroundAlpha);
      host_.DrawScreenQuad(0.0, 0.0, kTextureSize, kTextureSize, 1.f);
    }

    // Blend in the test pattern.
    {
      host_.SetBlend(true, blend_func, sfactor, dfactor);
      host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
      host_.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);
      host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);

      auto &texture_stage = host_.GetTextureStage(1);
      texture_stage.SetEnabled();
      texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
      texture_stage.SetTextureDimensions(kSwatchSize, kSwatchSize);
      host_.SetupTextureStages();

      float increment = floorf(static_cast<float>(kSwatchSize) / (kNumBlends + 1));
      float xy = increment;
      for (auto i = 0; i < kNumBlends; ++i) {
        host_.DrawSwizzledTexturedScreenQuad(xy, xy, xy + kSwatchSize, xy + kSwatchSize, 0.f);
        xy += increment;
      }

      texture_stage.SetEnabled(false);
      host_.SetupTextureStages();
      host_.SetShaderStageProgram(TestHost::STAGE_NONE);

      host_.SetBlend(false);
    }

    host_.RenderToSurfaceEnd();

    host_.PBKitBusyWait();
  };

  //  {"srcA", NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA},
  //  {"1-srcA", NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_SRC_ALPHA},
  //  {"dstA", NV097_SET_BLEND_FUNC_SFACTOR_V_DST_ALPHA},
  //  {"1-dstA", NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_DST_ALPHA},
  //  {"srcAsat", NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA_SATURATE},
  //  {"cA", NV097_SET_BLEND_FUNC_SFACTOR_V_CONSTANT_ALPHA},
  //  {"1-cA", NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_CONSTANT_ALPHA},

  prepare_texture(NV097_SET_BLEND_EQUATION_V_FUNC_ADD, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA,
                  NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetEnabled();
  texture_stage.SetFormat(GetTextureFormatInfo(TextureFormatForSurfaceFormat(surface_format, true)));
  texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  float x = host_.CenterX(kTextureSize);
  float y = host_.CenterY(kTextureSize);
  host_.DrawTexturedScreenQuad(x, y, x + kTextureSize, y + kTextureSize, 0.f, kTextureSize, kTextureSize);

  texture_stage.SetEnabled(false);
  host_.SetupTextureStages();
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  pb_printat(0, 0, (char *)"%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
