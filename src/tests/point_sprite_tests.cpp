#include "point_sprite_tests.h"

#include "test_host.h"
#include "texture_generator.h"
#include "xbox_math_vector.h"

static constexpr char kAlphaTestTest[] = "AlphaTest";

PointSpriteTests::PointSpriteTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Point sprite", config) {
  tests_[kAlphaTestTest] = [this]() { TestAlphaTest(); };
}

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc AlphaTest
 *   Basic test of POINT_SMOOTH_ENABLE true behavior. Renders a textured point, then the same point with the color
 *   combiner set to just diffuse, then the same pair again with point scaling enabled. The second row is the same
 *   rendering with alpha testing enabled.
 */
void PointSpriteTests::Initialize() {
  TestSuite::Initialize();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void PointSpriteTests::TestAlphaTest() {
  static constexpr uint32_t kTextureSize = 64;
  GenerateSwizzledRGBRadialATestPattern(host_.GetTextureMemoryForStage(3), kTextureSize, kTextureSize);

  host_.PrepareDraw(0xFF333333);

  auto &texture_stage = host_.GetTextureStage(3);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
  texture_stage.SetEnabled(true);
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                              TestHost::STAGE_2D_PROJECTIVE);

  host_.SetPointSize(64.f);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, true);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_A, 0.f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_B, 0.f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_C, 2.7125650614578944e-09);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE, 0.9999799728393555);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_1, 0.9999799728393555);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_2, 0.9999799728393555);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_BIAS, -2.0000399672426283e-05);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_MIN_SIZE, 1.9999999494757503e-05);
  Pushbuffer::End();

  static constexpr float kMargin = 180.f;
  static constexpr float kTop = 96.f;
  static constexpr float kSpacing = 96.f;

  host_.SetDiffuse(0xFFFFFFFF);
  host_.SetBlend();

  auto render_row = [this](float left, float top) {
    auto draw_point = [this](float cx, float cy) {
      host_.Begin(TestHost::PRIMITIVE_POINTS);
      vector_t screen_point{cx, cy, 1.f, 1.f};
      vector_t transformed;
      host_.UnprojectPoint(transformed, screen_point);
      VectorCopyVector(screen_point, transformed);
      host_.SetVertex(screen_point);
      host_.End();
    };

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, false);
    Pushbuffer::End();

    // Render a normal textured point sprite.
    {
      host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
      host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);

      draw_point(left, top);
      left += kSpacing;
    }

    // Render the diffuse color with the point sprite texture still enabled but not referenced at all.
    {
      host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
      host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

      draw_point(left, top);
      left += kSpacing;
    }

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, true);
    Pushbuffer::End();
    // Render a normal textured point sprite.
    {
      host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
      host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);

      draw_point(left, top);
      left += kSpacing;
    }

    // Render the diffuse color with the point sprite texture still enabled but not referenced at all.
    {
      host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
      host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

      draw_point(left, top);
      left += kSpacing;
    }
  };

  float top = kTop;
  render_row(kMargin, top);
  top += kSpacing;

  // Enable alpha testing for alpha > 0 and do the same
  host_.SetAlphaFunc(true, NV097_SET_ALPHA_FUNC_V_GREATER);
  host_.SetAlphaReference(0x7F);
  render_row(kMargin, top);

  {
    // Cleanup
    host_.SetAlphaFunc(false);
    host_.SetPointSize(1.f);
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, false);
    Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, false);
    Pushbuffer::End();
  }
  pb_print("%s\n", kAlphaTestTest);
  pb_printat(3, 0, "No test");
  pb_printat(7, 0, "Test 0x7f");
  pb_draw_text_screen();

  FinishDraw(kAlphaTestTest);
}
