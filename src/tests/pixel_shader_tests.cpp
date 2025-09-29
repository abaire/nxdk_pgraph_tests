#include "pixel_shader_tests.h"

#include "test_host.h"

static constexpr char kPassthrough[] = "Passthru";
static constexpr char kClipPlane[] = "ClipPlane";
static constexpr char kBumpEnvMap[] = "BumpEnvMap";
static constexpr char kBumpEnvMapLuminance[] = "BumpEnvMapLuminance";
static constexpr char kBRDF[] = "BRDF";
static constexpr char kDotST[] = "DotST";
static constexpr char kDotZW[] = "DotZW";
static constexpr char kDotReflectDiffuse[] = "DotReflectDiffuse";
static constexpr char kDotReflectSpecular[] = "DotReflectSpecular";
static constexpr char kDotStr3D[] = "DotSTR3D";
static constexpr char kStageDependentAR[] = "StageDependentAlphaRed";
static constexpr char kStageDependentGB[] = "StageDependentGreenBlue";
static constexpr char kDotReflectSpecularConst[] = "DotReflectSpecularConst";

PixelShaderTests::PixelShaderTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Pixel shader", config) {
  tests_[kPassthrough] = [this]() { TestPassthrough(); };
  tests_[kClipPlane] = [this]() { TestClipPlane(); };
  tests_[kBumpEnvMap] = [this]() { TestBumpEnvMap(); };
  //  tests_[kDotReflectDiffuse] = [this]() { TestDotReflectDiffuse(); };
}

void PixelShaderTests::Initialize() {
  TestSuite::Initialize();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  water_bump_map_.LoadPNG("D:\\pixel_shader\\water_bump_map.png");
  bump_map_test_image_.LoadPNG("D:\\pixel_shader\\bump_map_test_image.png");
}

void PixelShaderTests::TestPassthrough() {
  host_.PrepareDraw(0xFF222222);

  static constexpr auto kQuadSize = 256;
  auto draw_quad = [this](float left, float top) {
    host_.SetDiffuse(0xFF7F7F7F);
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.f, 0.f, 1.f, 0.25f);
    host_.SetScreenVertex(left, top);

    host_.SetTexCoord0(0.f, 1.f, 0.f, 0.5f);
    host_.SetScreenVertex(left + kQuadSize, top);

    host_.SetTexCoord0(1.f, 0.f, 0.f, 0.75f);
    host_.SetScreenVertex(left + kQuadSize, top + kQuadSize);

    host_.SetTexCoord0(1.f, 1.f, 0.4f, 0.33f);
    host_.SetScreenVertex(left, top + kQuadSize);
    host_.End();
  };

  // The stage must be enabled. If it is not, SRC_TEX0 will be (0, 0, 0, 1) and the per-vertex coords will be ignored.
  host_.SetTextureStageEnabled(0, true);
  host_.SetupTextureStages();

  host_.SetShaderStageProgram(TestHost::STAGE_PASS_THROUGH);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  draw_quad(host_.CenterX(kQuadSize), host_.CenterY(kQuadSize));

  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  pb_print("%s\n", kPassthrough);
  pb_print("Tex coords are used as colors\n");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kPassthrough);
}

void PixelShaderTests::TestClipPlane() {
  host_.PrepareDraw();

  // No configuration of the stage is necessary, and no texture should be bound.

  static constexpr uint32_t kBackgroundColor = 0xFF333333;
  static constexpr uint32_t kRedFragmentColor = 0xFF7F44DD;
  static constexpr uint32_t kGreenFragmentColor = 0xFF6EDD44;
  static constexpr uint32_t kBlueFragmentColor = 0xFFEE6655;

  static constexpr auto kQuadSize = 64;
  auto draw_quad = [this](float left, float top, uint32_t color) {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(color);
    host_.SetTexCoord0(0.f, 0.1f, -0.02f, -0.25f);
    host_.SetTexCoord1(-0.1f, -0.1f, 0.02f, 0.25f);
    host_.SetTexCoord2(-0.1f, -0.1f, -0.1f, 0.1f);
    host_.SetScreenVertex(left, top);

    host_.SetTexCoord0(1.f, -0.3f, 1.f, 0.f);
    host_.SetTexCoord1(0.1f, 0.3f, -0.2f, -0.01f);
    host_.SetTexCoord2(0.1f, -0.1f, -0.1f, -0.1f);
    host_.SetScreenVertex(left + kQuadSize, top);

    host_.SetTexCoord0(-1.f, -0.1f, -0.1f, 0.5f);
    host_.SetTexCoord1(1.f, 0.1f, 0.1f, -0.25f);
    host_.SetTexCoord2(-0.1f, 0.1f, -0.1f, -0.1f);
    host_.SetScreenVertex(left + kQuadSize, top + kQuadSize);

    host_.SetTexCoord0(-0.1f, 2.f, 3.f, 0.25f);
    host_.SetTexCoord1(0.1f, -0.6f, 1.1f, -0.35f);
    host_.SetTexCoord2(-0.1f, -0.1f, 0.1f, -0.1f);
    host_.SetScreenVertex(left, top + kQuadSize);
    host_.End();
  };

  static constexpr float kSpacing = kQuadSize + 16.f;

  static constexpr float kHorizontalStart = 32.f;
  float left = kHorizontalStart;
  static constexpr float kTextColumnStart = 2;
  int tex_col = kTextColumnStart;

  float top = 208.f;
  int tex_row = 6;

  // The stage must be enabled. If it is not, SRC_TEXx will be (0, 0, 0, 1) and the per-vertex coords will be ignored.
  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetupTextureStages();

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  for (auto s_ge_zero : {false, true}) {
    for (auto t_ge_zero : {false, true}) {
      for (auto r_ge_zero : {false, true}) {
        for (auto q_ge_zero : {false, true}) {
          host_.SetShaderStageProgram(TestHost::STAGE_NONE);
          draw_quad(left, top, kBackgroundColor);

          host_.SetShaderStageProgram(TestHost::STAGE_CLIP_PLANE);
          host_.SetShaderClipPlaneComparator(0, s_ge_zero, t_ge_zero, r_ge_zero, q_ge_zero);
          draw_quad(left, top, kRedFragmentColor);

          host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_CLIP_PLANE);
          host_.SetShaderClipPlaneComparator(1, s_ge_zero, t_ge_zero, r_ge_zero, q_ge_zero);
          draw_quad(left, top, kGreenFragmentColor);

          host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_CLIP_PLANE);
          host_.SetShaderClipPlaneComparator(2, s_ge_zero, t_ge_zero, r_ge_zero, q_ge_zero);
          draw_quad(left, top, kBlueFragmentColor);

          pb_printat(tex_row, tex_col, "%s%s%s%s", s_ge_zero ? "0" : "-", t_ge_zero ? "0" : "-", r_ge_zero ? "0" : "-",
                     q_ge_zero ? "0" : "-");
          left += kSpacing;
          tex_col += 8;

          if (left + kQuadSize >= host_.GetFramebufferWidthF()) {
            left = kHorizontalStart;
            top += kSpacing + 16.f;

            tex_row += 4;
            tex_col = kTextColumnStart;
          }
        }
      }
    }
  }

  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetupTextureStages();

  pb_printat(0, 0, "%s\n", kClipPlane);
  pb_printat(1, 0, "Tex coords are used to cull fragments based on sign\n");
  pb_printat(2, 0, "Red   11-- 1-11 ---1 -111\n");
  pb_printat(3, 0, "Green --11 11-- 111- 1---\n");
  pb_printat(4, 0, "Blue ---1 1--- -1-- --1-\n");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kClipPlane);
}

void PixelShaderTests::TestBumpEnvMap() {
  host_.PrepareDraw(0xFF111111);

  static constexpr auto kTop = 64.f;
  static constexpr auto kReferenceQuadSize = 128.f;
  static constexpr auto kQuadSize = 128.f;
  static constexpr auto kQuadSpacing = kQuadSize + 16.f;
  static constexpr auto kReferenceLeft = 8.f;
  DrawPlainImage(kReferenceLeft, kTop, bump_map_test_image_, kReferenceQuadSize);
  DrawPlainImage(kReferenceLeft, kTop + kReferenceQuadSize + 16.f, water_bump_map_, kReferenceQuadSize);

  // The BumpEnv stage reads the normal map from a previous stage (for stage 1, only stage 0 may be used, for later
  // stages SetShaderStageInput could be used to pick a previous stage).
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_BUMPENVMAP);
  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);

  {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
    texture_stage.SetImageDimensions(water_bump_map_.width, water_bump_map_.height);
    water_bump_map_.CopyTo(host_.GetTextureMemoryForStage(0));
  }
  {
    auto &texture_stage = host_.GetTextureStage(1);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
    texture_stage.SetImageDimensions(bump_map_test_image_.width, bump_map_test_image_.height);
    bump_map_test_image_.CopyTo(host_.GetTextureMemoryForStage(1));
  }
  host_.SetupTextureStages();

  auto draw_quad = [this](float left, float top) {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.f, 0.f);
    host_.SetTexCoord1(0.f, 0.f);
    host_.SetScreenVertex(left, top);

    host_.SetTexCoord0(static_cast<float>(water_bump_map_.width), 0.f);
    host_.SetTexCoord1(static_cast<float>(bump_map_test_image_.width), 0.f);
    host_.SetScreenVertex(left + kQuadSize, top);

    host_.SetTexCoord0(static_cast<float>(water_bump_map_.width), static_cast<float>(water_bump_map_.height));
    host_.SetTexCoord1(static_cast<float>(bump_map_test_image_.width), static_cast<float>(bump_map_test_image_.height));
    host_.SetScreenVertex(left + kQuadSize, top + kQuadSize);

    host_.SetTexCoord0(0.f, static_cast<float>(water_bump_map_.height));
    host_.SetTexCoord1(0.f, static_cast<float>(bump_map_test_image_.height));
    host_.SetScreenVertex(left, top + kQuadSize);
    host_.End();
  };

  static constexpr auto kLeft = kReferenceLeft + kReferenceQuadSize + 16.f;

  float left = kLeft;
  float top = kTop;

  // Note that values must be in the range -1, 1 or hardware will assert.
  static constexpr float kEnvMapMatrices[][4] = {
      // {0.f, 0.f, 0.f, 0.f}, // 0.f cancels out the normal map jitter entirely and produces the source image.
      {-1.f, 0.f, 1.f, 0.f},   {1.f, 1.f, 1.f, 1.f},  {0.f, 0.2f, 0.f, 0.75f}, {0.5f, -0.5f, 0.5f, -0.5f},
      {0.3f, 0.5f, 0.7f, 1.f}, {0.f, 1.f, -1.f, 0.f}, {0.f, 0.f, 1.f, 1.f},    {0.5f, 0.8f, 0.f, 0.f},
  };

  for (auto matrix : kEnvMapMatrices) {
    auto &texture_stage = host_.GetTextureStage(1);
    texture_stage.SetBumpEnv(matrix[0], matrix[1], matrix[2], matrix[3]);
    host_.SetupTextureStages();

    draw_quad(left, top);

    left += kQuadSpacing;
    if (left + kQuadSize > host_.GetFramebufferWidthF()) {
      left = kLeft;
      top += kQuadSpacing;
    }
  }

  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  pb_print("%s\n", kBumpEnvMap);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kBumpEnvMap);
}

void PixelShaderTests::DrawPlainImage(float x, float y, const ImageResource &image, float quad_size) {
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetTextureStageEnabled(3, false);

  {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
    texture_stage.SetImageDimensions(image.width, image.height);
    image.CopyTo(host_.GetTextureMemoryForStage(0));
  }
  host_.SetupTextureStages();

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetTexCoord0(0.f, 0.f);
  host_.SetScreenVertex(x, y);

  host_.SetTexCoord0(static_cast<float>(image.width), 0.f);
  host_.SetScreenVertex(x + quad_size, y);

  host_.SetTexCoord0(static_cast<float>(image.width), static_cast<float>(image.height));
  host_.SetScreenVertex(x + quad_size, y + quad_size);

  host_.SetTexCoord0(0.f, static_cast<float>(image.height));
  host_.SetScreenVertex(x, y + quad_size);
  host_.End();

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
}
