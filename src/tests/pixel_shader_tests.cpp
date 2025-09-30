#include "pixel_shader_tests.h"

#include "test_host.h"
#include "texture_generator.h"
#include "xbox_math_matrix.h"

static constexpr char kPassthrough[] = "Passthru";
static constexpr char kClipPlane[] = "ClipPlane";
static constexpr char kBumpEnvMap[] = "BumpEnvMap";
static constexpr char kBumpEnvMapLuminance[] = "BumpEnvMapLuminance";
// static constexpr char kBRDF[] = "BRDF";
static constexpr char kDotST[] = "DotST";
static constexpr char kDotZW[] = "DotZW";
static constexpr char kStageDependentAR[] = "StageDependentAlphaRed";
static constexpr char kStageDependentGB[] = "StageDependentGreenBlue";

PixelShaderTests::PixelShaderTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Pixel shader", config) {
  tests_[kPassthrough] = [this]() { TestPassthrough(); };
  tests_[kClipPlane] = [this]() { TestClipPlane(); };
  tests_[kBumpEnvMap] = [this]() { TestBumpEnvMap(); };
  tests_[kBumpEnvMapLuminance] = [this]() { TestBumpEnvMap(true); };

  tests_[kDotST] = [this]() { TestDotST(); };
  tests_[kDotZW] = [this]() { TestDotZW(); };

  tests_[kStageDependentAR] = [this]() { TestDependentColorChannel(); };
  tests_[kStageDependentGB] = [this]() { TestDependentColorChannel(true); };
}

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc Passthru
 *  Demonstrates behavior of PS_TEXTUREMODES_PASSTHRU, which simply utilizes texture coordinates as colors.
 *
 * @tc ClipPlane
 *   Demonstrates behavior of PS_TEXTUREMODES_CLIPPLANE. Various mock values for clipping planes are assigned to texture
 *   coordinates for stages 0 (red), 1 (green), and 2 (blue). The values of the coordinates are summarized in the image,
 *   with "1" used to indicate a positive value and "-" to indicate a negative one. The comparator function is set for
 *   each quad in the image with a similar shorthand indicating whether results < 0 are clipped ("-") or >= 0 ("0").
 *
 * @tc BumpEnvMap
 *   Demonstrates behavior of PS_TEXTUREMODES_BUMPENVMAP with various values for the NV097_SET_TEXTURE_SET_BUMP_ENV_MAT
 *   matrix. Values are {-1.f, 0.f, 1.f, 0.f}, {1.f, 1.f, 1.f, 1.f}, {0.f, 0.2f, 0.f, 0.75f},
 *   {0.5f, -0.5f, 0.5f, -0.5f}, {0.3f, 0.5f, 0.7f, 1.f}, {0.f, 1.f, -1.f, 0.f}, {0.f, 0.f, 1.f, 1.f},
 *   {0.5f, 0.8f, 0.f, 0.f},
 *
 * @tc BumpEnvMapLuminance
 *   Demonstrates behavior of PS_TEXTUREMODES_BUMPENVMAP_LUM with various values for
 *   NV097_SET_TEXTURE_SET_BUMP_ENV_SCALE and NV097_SET_TEXTURE_SET_BUMP_ENV_OFFSET. Outputs are grouped by the env bump
 *   matrix (the value of each element is identical and printed in row 1). Columns are divided up by the scale factor,
 *   whose value is printed in row 2. Rows are defined by the offset value: {0, 0.25, 0.75, 1.0}.
 *
 * @tc DotST
 *   Renders a quad using PS_TEXTUREMODES_DOT_ST. Texture stage 0 is set to a bump map, stage 2 to an image with
 *   diagonal lines. Texture coordinates 1 are set to a normalized vector from the eye coordinate to each vertex
 *   (technically to a virtual position rather than the actual vertex position) and coordinate 2 is set to a normalized
 *   vector from the vertex to a virtual light. See
 *   https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/texm3x2tex---ps
 *
 * @tc DotZW
 *   Renders a quad using PS_TEXTUREMODES_DOT_ZW. Texture stage 0 is set to a bump map. Tex coords for stages 1 and 2
 *   define a 3x2 matrix multiplied against the texture 0 sampler to determine the Z and W components of the fragment's
 *   depth (z = tex1.uvw * tex0.rgb, w = tex2.uvw * tex0.rgb, final depth = z/w).
 *   See https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/texm3x2depth---ps
 *
 * @tc StageDependentAlphaRed
 *   Renders a quad using PS_TEXTUREMODES_DPNDNT_AR. Texture stage 0 is set to a gradient textured. The sampled alpha
 *   and red channels from this texture are used as UV coordinates into the texture at stage 2.
 *
 * @tc StageDependentGreenBlue
 *   Renders a quad using PS_TEXTUREMODES_DPNDNT_GB. Texture stage 0 is set to a gradient textured. The sampled green
 *   and blue channels from this texture are used as UV coordinates into the texture at stage 2.
 *
 */
void PixelShaderTests::Initialize() {
  TestSuite::Initialize();

  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());

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

void PixelShaderTests::TestBumpEnvMap(bool luminance) {
  host_.PrepareDraw(0xFF222222);

  static constexpr auto kTop = 64.f;
  const auto kQuadSize = luminance ? 64.f : 128.f;
  const auto kQuadSpacing = kQuadSize + 16.f;
  static constexpr auto kReferenceLeft = 8.f;
  DrawPlainImage(kReferenceLeft, kTop, bump_map_test_image_, kQuadSize);
  DrawPlainImage(kReferenceLeft, kTop + kQuadSize + 16.f, water_bump_map_, kQuadSize);

  // The BumpEnv stage reads the normal map from a previous stage (for stage 1, only stage 0 may be used, for later
  // stages SetShaderStageInput could be used to pick a previous stage).
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE,
                              luminance ? TestHost::STAGE_BUMPENVMAP_LUMINANCE : TestHost::STAGE_BUMPENVMAP);
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

  auto draw_quad = [this, kQuadSize](float left, float top) {
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

  const auto kLeft = kReferenceLeft + kQuadSize + 16.f;

  float left = kLeft;
  float top = kTop;

  // Note that values must be in the range -1, 1 or hardware will assert.
  if (luminance) {
    static constexpr float kEnvMapMatrices[][4] = {
        {0.f, 0.f, 0.f, 0.f},  // 0.f cancels out the normal map jitter entirely and produces the source image.
        {0.5f, 0.5f, 0.5f, 0.5f},
    };

    static constexpr float kLuminanceTitleSpace = 32.f;
    top += kLuminanceTitleSpace;

    int matrix_col = 17;
    int scale_col = 10;

    for (auto matrix : kEnvMapMatrices) {
      pb_printat(1, matrix_col, "0.%d", static_cast<int>(matrix[0] * 10));
      matrix_col += 27;

      for (auto scale : {0.f, 1.f, 2.f}) {
        for (auto offset : {0.f, 0.25f, 0.75f, 1.f}) {
          auto &texture_stage = host_.GetTextureStage(1);
          texture_stage.SetBumpEnv(matrix[0], matrix[1], matrix[2], matrix[3], scale, offset);
          host_.SetupTextureStages();

          draw_quad(left, top);

          top += kQuadSpacing;
        }

        top = kTop + kLuminanceTitleSpace;
        left += kQuadSpacing;
        pb_printat(2, scale_col, "%d", static_cast<int>(scale));
        scale_col += 8;
      }

      left += kQuadSpacing * 0.5f;
      scale_col += 4;
    }
  } else {
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
  }

  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  pb_printat(0, 0, "%s\n", luminance ? kBumpEnvMapLuminance : kBumpEnvMap);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, luminance ? kBumpEnvMapLuminance : kBumpEnvMap);
}

void PixelShaderTests::TestDotST() {
  host_.PrepareDraw(0xFF222222);

  static constexpr auto kQuadSize = 256.f;

  auto set_look_and_light_vectors = [this](const vector_t &at) {
    const vector_t eye{0.f, 0.f, -7.f, 1.f};
    const vector_t light{-1.5f, 5.f, 0.f, 1.f};

    vector_t look_dir{0.f, 0.f, 0.f, 1.f};
    VectorSubtractVector(at, eye, look_dir);
    VectorNormalize(look_dir);

    vector_t light_dir{0.f, 0.f, 0.f, 1.f};
    VectorSubtractVector(light, at, light_dir);
    VectorNormalize(light_dir);

    host_.SetTexCoord1(look_dir[0], look_dir[1], look_dir[2], look_dir[3]);
    host_.SetTexCoord2(light_dir[0], light_dir[1], light_dir[2], light_dir[3]);
  };

  auto draw_quad = [this, set_look_and_light_vectors](float left, float top) {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    // Tex 0 is the source bump map.
    host_.SetTexCoord0(0.f, 0.f);
    // Tex 1 and 2 specify a 3x2 matrix combined with the tex0 sample to produce the u,v coordinate used to sample tex2.
    set_look_and_light_vectors({-1.f, 1.f, 4.f, 1.f});
    host_.SetScreenVertex(left, top);

    host_.SetTexCoord0(static_cast<float>(water_bump_map_.width), 0.f);
    set_look_and_light_vectors({1.f, 1.f, 7.f, 1.f});
    host_.SetScreenVertex(left + kQuadSize, top);

    host_.SetTexCoord0(static_cast<float>(water_bump_map_.width), static_cast<float>(water_bump_map_.height));
    set_look_and_light_vectors({1.f, -2.f, 6.f, 1.f});
    host_.SetScreenVertex(left + kQuadSize, top + kQuadSize);

    host_.SetTexCoord0(0.f, static_cast<float>(water_bump_map_.height));
    set_look_and_light_vectors({-1.f, -2.f, 4.f, 1.f});
    host_.SetScreenVertex(left, top + kQuadSize);

    host_.End();
  };

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_DOT_PRODUCT, TestHost::STAGE_DOT_ST);
  host_.SetShaderStageInput(0);

  {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
    texture_stage.SetImageDimensions(water_bump_map_.width, water_bump_map_.height);
    water_bump_map_.CopyTo(host_.GetTextureMemoryForStage(0));
  }
  {
    auto &texture_stage = host_.GetTextureStage(2);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
    texture_stage.SetTextureDimensions(bump_map_test_image_.width, bump_map_test_image_.height);
    bump_map_test_image_.SwizzleTo(host_.GetTextureMemoryForStage(2));
  }
  host_.SetupTextureStages();

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX2);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX2, true);

  draw_quad(host_.CenterX(kQuadSize), host_.CenterY(kQuadSize));

  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  pb_printat(0, 0, "%s\n", kDotST);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kDotST);
}

void PixelShaderTests::TestDotZW() {
  host_.PrepareDraw(0xFF222222);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_STENCIL_MASK, false);
  Pushbuffer::End();

  // Override the depth clip to ensure that max_val depths (post-projection) are never clipped.
  host_.SetDepthClip(0.0f, 16777216);

  static constexpr auto kQuadSize = 256.f;
  static constexpr auto kZ = 0;

  auto set_matrix = [this](const vector_t &row0, const vector_t &row1) {
    host_.SetTexCoord1(row0[0], row0[1], row0[2], row0[3]);
    host_.SetTexCoord2(row1[0], row1[1], row1[2], row1[3]);
  };

  auto draw_quad = [this, set_matrix](float left, float top) {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    // Tex 0 is the source bump map.
    host_.SetTexCoord0(0.f, 0.f);
    // Tex 1 and 2 specify a 3x2 matrix combined with the tex0 sample to produce the z,w values used to produce fragment
    // depth = z/w.
    //
    // A typical value from the texture would be 0.5, 0.5, 0.9
    // Final values must fit in the range supported by the current depth buffer.
    set_matrix({1000.f, 1000.f, 1000.f, 1.f}, {0.1f, 0.1f, 0.1f, 1.f});
    host_.SetScreenVertex(left, top, kZ);

    host_.SetTexCoord0(static_cast<float>(water_bump_map_.width), 0.f);
    set_matrix({100.f, 100.f, 40000.f, 1.f}, {1.f, 1.f, 0.f, 1.f});
    host_.SetScreenVertex(left + kQuadSize, top, kZ);

    host_.SetTexCoord0(static_cast<float>(water_bump_map_.width), static_cast<float>(water_bump_map_.height));
    set_matrix({10.f, 100.f, 1000.f, 1.f}, {1.f, 0.f, 0.f, 1.f});
    host_.SetScreenVertex(left + kQuadSize, top + kQuadSize, kZ);

    host_.SetTexCoord0(0.f, static_cast<float>(water_bump_map_.height));
    set_matrix({100.f, 100.f, 100.f, 1.f}, {0.f, 0.f, 1.f, 1.f});
    host_.SetScreenVertex(left, top + kQuadSize, kZ);

    host_.End();
  };

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_DOT_PRODUCT, TestHost::STAGE_DOT_ZW);
  host_.SetShaderStageInput(0);

  {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
    texture_stage.SetImageDimensions(water_bump_map_.width, water_bump_map_.height);
    water_bump_map_.CopyTo(host_.GetTextureMemoryForStage(0));
  }
  host_.SetupTextureStages();

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  draw_quad(host_.CenterX(kQuadSize), host_.CenterY(kQuadSize));

  host_.PBKitBusyWait();

  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);

  {
    // Render the Z buffer over the quad.
    const float left = host_.CenterX(kQuadSize);
    const float top = host_.CenterY(kQuadSize);
    DrawZBuffer(left, top, static_cast<uint32_t>(left), static_cast<uint32_t>(top), kQuadSize);
  }

  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  pb_printat(0, 0, "%s\n", kDotZW);
  pb_printat(1, 0, "Quad at z = %d but reassigned via DOT_ZW\n", kZ);
  pb_printat(2, 0, "Depth buffer as R5G6B5\n");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kDotZW);
}

void PixelShaderTests::TestDependentColorChannel(bool use_blue_green) {
  host_.PrepareDraw(0xFF282828);

  static constexpr auto kQuadSize = 256.f;

  auto draw_quad = [this](float left, float top) {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.f, 0.f);
    host_.SetScreenVertex(left, top);

    host_.SetTexCoord0(1.f, 0.f);
    host_.SetScreenVertex(left + kQuadSize, top);

    host_.SetTexCoord0(1.f, 1.f);
    host_.SetScreenVertex(left + kQuadSize, top + kQuadSize);

    host_.SetTexCoord0(0.f, 1.f);
    host_.SetScreenVertex(left, top + kQuadSize);

    host_.End();
  };

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE,
                              use_blue_green ? TestHost::STAGE_DEPENDENT_GB : TestHost::STAGE_DEPENDENT_AR);
  host_.SetShaderStageInput(0);

  {
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    texture_stage.SetTextureDimensions(kQuadSize, kQuadSize);
    GenerateSwizzledRGBRadialATestPattern(host_.GetTextureMemoryForStage(0), kQuadSize, kQuadSize);
  }
  {
    auto &texture_stage = host_.GetTextureStage(1);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    texture_stage.SetTextureDimensions(kQuadSize, kQuadSize);
    GenerateSwizzledRGBATestPattern(host_.GetTextureMemoryForStage(1), kQuadSize, kQuadSize);
  }
  host_.SetupTextureStages();

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);

  draw_quad(host_.CenterX(kQuadSize), host_.CenterY(kQuadSize));

  host_.PBKitBusyWait();

  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  pb_printat(0, 0, "%s\n", use_blue_green ? kStageDependentGB : kStageDependentAR);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, use_blue_green ? kStageDependentGB : kStageDependentAR);
}

void PixelShaderTests::DrawPlainImage(float x, float y, const ImageResource &image, float quad_size,
                                      bool border) const {
  if (border) {
    static constexpr float kBorderThickness = 2.f;
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFFFFFFFF);
    host_.SetScreenVertex(x - kBorderThickness, y - kBorderThickness);
    host_.SetScreenVertex(x + quad_size + kBorderThickness, y - kBorderThickness);
    host_.SetScreenVertex(x + quad_size + kBorderThickness, y + quad_size + kBorderThickness);
    host_.SetScreenVertex(x - kBorderThickness, y + quad_size + kBorderThickness);
    host_.End();
  }

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

  host_.PBKitBusyWait();
}

void PixelShaderTests::DrawZBuffer(float left, float top, uint32_t src_x, uint32_t src_y, uint32_t quad_size) const {
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetTextureStageEnabled(3, false);

  const auto *z_buffer = reinterpret_cast<const uint8_t *>(
      pb_agp_access(const_cast<void *>(static_cast<const void *>(pb_depth_stencil_buffer()))));
  const auto z_buffer_pitch = pb_depth_stencil_pitch();

  z_buffer += src_x * 2 + src_y * z_buffer_pitch;
  const auto byte_length = static_cast<uint32_t>(quad_size * 2);

  auto texture = host_.GetTextureMemoryForStage(0);
  for (auto y = 0; y < static_cast<uint32_t>(quad_size); ++y) {
    memcpy(texture, z_buffer, byte_length);
    z_buffer += z_buffer_pitch;
    texture += byte_length;
  }

  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R5G6B5));
  texture_stage.SetImageDimensions(quad_size, quad_size);
  host_.SetupTextureStages();

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
  host_.SetBlend(false);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, false);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::End();

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetTexCoord0(0.f, 0.f);
  host_.SetScreenVertex(left, top);

  host_.SetTexCoord0(quad_size, 0.f);
  host_.SetScreenVertex(left + quad_size, top);

  host_.SetTexCoord0(quad_size, quad_size);
  host_.SetScreenVertex(left + quad_size, top + quad_size);

  host_.SetTexCoord0(0.f, quad_size);
  host_.SetScreenVertex(left, top + quad_size);

  host_.End();

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.PBKitBusyWait();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::End();

  host_.SetBlend(true);
}
