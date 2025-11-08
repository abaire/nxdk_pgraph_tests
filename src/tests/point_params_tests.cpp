#include "point_params_tests.h"

#include <debug_output.h>
#include <xbox_math_d3d.h>
#include <xbox_math_matrix.h>

#include <memory>

#include "shaders/passthrough_vertex_shader.h"
#include "texture_generator.h"

// clang-format on
struct TestConfig {
  const char* name;
  bool point_params_enabled;
  bool point_smooth_enabled;
  int point_size;
  bool use_shader;
};

struct PointParams {
  float scaleFactorA;   //!< Constant component.
  float scaleFactorB;   //!< Linear component.
  float scaleFactorC;   //!< Quadratic component.
  float sizeRange;      //<! Max point size - min point size.
  float sizeRangeDup1;  //!< Seems to be ignored, always set to sizeRange.
  float sizeRangeDup2;  //!< Seems to be ignored, always set to sizeRange.
  float scaleBias;      //!< Modifies the attenuated distance.
  float minSize;        //!< Minimum point size
};

static constexpr float kZIncrementPerRow = 25.f;
static constexpr uint32_t kTextureSize = 32;

static constexpr PointParams kBasicPointParams[] = {
    {0.f, 0.f, 0.f, 0.f, 100.f, 200.f, -0.f, 0.f},      {0.f, 0.f, 0.f, 0.f, 100.f, 200.f, -0.f, 16.f},
    {0.000001f, 0.f, 0.f, 16.f, 16.f, 32.f, -0.f, 0.f}, {0.000001f, 0.f, 0.f, 16.f, 16.f, 32.f, -0.f, 8.f},
    {4.f, 0.f, 0.f, 14.f, 0.f, 0.f, -0.f, 16.f},        {4.f, 0.f, 0.f, 36.f, 0.f, 0.f, -0.f, 4.f},
    {256.f, 0.f, 0.f, 36.f, 0.f, 0.f, -0.f, 4.f},       {0.f, 0.25f, 0.f, 39.f, 0.f, 0.f, -0.f, 1.f},
    {0.f, 0.f, 0.05f, 39.f, 0.f, 0.f, -0.f, 1.f},       {4.f, 0.f, 0.f, 14.f, 0.f, 0.f, -0.5f, 16.f},
    {4.f, 0.f, 0.f, 14.f, 0.f, 0.f, 0.5f, 16.f},        {0.f, 0.f, 0.f, 15.f, 0.f, 0.f, -100.f, 1.f},
    {0.f, 0.f, 0.f, 15.f, 0.f, 0.f, 100.f, 1.f},
};

static constexpr int kNumBasicPointParamsSets = sizeof(kBasicPointParams) / sizeof(kBasicPointParams[0]);

static constexpr PointParams kDetailedPointParams[] = {
    {0.f, 0.25f, 0.f, 39.f, 0.f, 0.f, -1.0f, 1.f},  {0.f, 0.25f, 0.f, 39.f, 0.f, 0.f, -0.5f, 1.f},
    {0.f, 0.25f, 0.f, 39.f, 0.f, 0.f, -0.25f, 1.f}, {0.f, 0.25f, 0.f, 39.f, 0.f, 0.f, -0.01f, 1.f},

    {0.f, 0.25f, 0.f, 39.f, 0.f, 0.f, -0.f, 1.f},

    {0.f, 0.25f, 0.f, 39.f, 0.f, 0.f, 0.01f, 1.f},  {0.f, 0.25f, 0.f, 39.f, 0.f, 0.f, 0.25f, 1.f},
    {0.f, 0.25f, 0.f, 39.f, 0.f, 0.f, 0.5f, 1.f},   {0.f, 0.25f, 0.f, 39.f, 0.f, 0.f, 1.0f, 1.f},

    {4.f, 0.f, 0.f, 36.f, 0.f, 0.f, -0.4f, 4.f},    {4.f, 0.f, 0.f, 36.f, 0.f, 0.f, -0.25f, 4.f},
    {4.f, 0.f, 0.f, 36.f, 0.f, 0.f, 0.25f, 4.f},    {4.f, 0.f, 0.f, 36.f, 0.f, 0.f, 0.4f, 4.f},
};

static constexpr int kNumDetailedPointParamsSets = sizeof(kDetailedPointParams) / sizeof(kDetailedPointParams[0]);

static const TestConfig kBasicTestConfigs[]{
    {"PointParamsOff_SmoothOff_128_FF", false, false, 128, false},
    {"PointParamsOn_SmoothOn_001_FF", true, true, 1, false},
    {"PointParamsOn_SmoothOn_128_FF", true, true, 128, false},
    {"PointParamsOn_SmoothOff_128_FF", true, false, 128, false},

    {"PointParamsOn_SmoothOn_128_VS", true, true, 128, true},
    {"PointParamsOn_SmoothOff_128_VS", true, false, 128, true},
    {"PointParamsOff_SmoothOff_128_VS", false, false, 128, true},
};

static constexpr const char kDetailedTestName[] = "Detailed_";

static constexpr const char kScaleParamsTestName[] = "Scale_";

static std::string MakeScaleParamTestName(bool scale_a, bool scale_b, bool scale_c, bool programmable) {
  std::string ret = kScaleParamsTestName;
  ret += (scale_a ? "4_" : "0_");
  ret += (scale_b ? "1_" : "0_");
  ret += (scale_c ? "1_" : "0_");
  ret += (programmable ? "VS" : "FF");
  return ret;
}

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc PointParamsOn_SmoothOn_001_FF
 *   Renders points using the fixed function pipeline with point size 1, point smoothing and point params enabled.
 *   Demonstrates that point size is ignored when point params are enabled.
 *
 * @tc PointParamsOn_SmoothOn_128_FF
 *   Renders points using the fixed function pipeline with point size 128 (16px), point smoothing and point params
 *   enabled.
 *
 * @tc PointParamsOn_SmoothOff_128_FF
 *   Renders points using the fixed function pipeline with point size 128 (16px), point smoothing disabled and point
 *   params enabled.
 *
 * @tc PointParamsOn_SmoothOn_128_VS
 *   Renders points using a programmable shader with point size 128 (16px), point smoothing and point params enabled.
 *
 * @tc PointParamsOn_SmoothOff_128_VS
 *   Renders points using a programmable shader with point size 128 (16px), point smoothing disabled and point params
 *   enabled.
 *
 * @tc PointParamsOff_SmoothOff_128_VS
 *   Renders points using a programmable shader with point size 128 (16px), point smoothing and point params disabled.
 *
 * @tc PointParamsOff_SmoothOff_128_FF
 *   Renders points using the fixed function pipeline with point size 128 (16px), point smoothing and point params
 *   disabled.
 *
 * @tc Detailed_FF
 *   Renders points using the fixed function pipeline with more complex point params.
 *
 * @tc Detailed_VS
 *   Renders points using a programmable shader with more complex point params.
 *
 * @tc Scale_0_0_0_FF
 *   Renders points using the fixed function pipeline at varying distances from the eye point with point size
 *   130 (16.25px) and point scale params 0.0, 0.0, 0.0. Minimum size is set to 0.5.
 *
 * @tc Scale_4_0_0_FF
 *   Renders points using the fixed function pipeline at varying distances from the eye point with point size
 *   130 (16.25px) and point scale params 4.0, 0.0, 0.0. Minimum size is set to 0.5.
 *
 * @tc Scale_0_1_0_FF
 *   Renders points using the fixed function pipeline at varying distances from the eye point with point size
 *   130 (16.25px) and point scale params 0.0, 1.0, 0.0. Minimum size is set to 0.5.
 *
 * @tc Scale_0_0_1_FF
 *   Renders points using the fixed function pipeline at varying distances from the eye point with point size
 *   130 (16.25px) and point scale params 0.0, 0.0, 1.0. Minimum size is set to 0.5.
 *
 * @tc Scale_0_0_0_VS
 *   Renders points using the programmable pipeline at varying distances from the eye point with point size
 *   130 (16.25px) and point scale params 0.0, 0.0, 0.0. Minimum size is set to 0.5.
 *
 * @tc Scale_4_0_0_VS
 *   Renders points using the programmable pipeline at varying distances from the eye point with point size
 *   130 (16.25px) and point scale params 4.0, 0.0, 0.0. Minimum size is set to 0.5.
 *
 * @tc Scale_0_1_0_VS
 *   Renders points using the programmable pipeline at varying distances from the eye point with point size
 *   130 (16.25px) and point scale params 0.0, 1.0, 0.0. Minimum size is set to 0.5.
 *
 * @tc Scale_0_0_1_VS
 *   Renders points using the programmable pipeline at varying distances from the eye point with point size
 *   130 (16.25px) and point scale params 0.0, 0.0, 1.0. Minimum size is set to 0.5.
 */
PointParamsTests::PointParamsTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Point params", config) {
  for (auto test_config : kBasicTestConfigs) {
    tests_[test_config.name] = [this, test_config]() {
      Test(test_config.name, test_config.point_params_enabled, test_config.point_smooth_enabled, test_config.point_size,
           test_config.use_shader);
    };
  }

  {
    std::string test_name = kDetailedTestName;
    test_name += "FF";
    tests_[test_name] = [this, test_name]() { TestDetailed(test_name, false); };
  }
  {
    std::string test_name = kDetailedTestName;
    test_name += "VS";
    tests_[test_name] = [this, test_name]() { TestDetailed(test_name, true); };
  }

  for (bool programmable : {false, true}) {
    for (bool scale_c : {false, true}) {
      for (bool scale_b : {false, true}) {
        for (bool scale_a : {false, true}) {
          auto test_name = MakeScaleParamTestName(scale_a, scale_b, scale_c, programmable);
          tests_[test_name] = [this, scale_a, scale_b, scale_c, programmable]() {
            TestScaleParams(scale_a, scale_b, scale_c, programmable);
          };
        }
      }
    }
  }
}

void PointParamsTests::Initialize() {
  TestSuite::Initialize();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, false);
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_STENCIL_MASK, false);
  Pushbuffer::End();

  // The texture is a white and grey checkerboard that will be multiplied by the diffuse color of the point.
  PBKitPlusPlus::GenerateSwizzledRGBACheckerboard(host_.GetTextureMemoryForStage(3), 0, 0, kTextureSize, kTextureSize,
                                                  kTextureSize * 4, 0xFFFFFFFF, 0xFF333333, 2);
}

template <typename RowStartCB, typename RenderPointCB>
static void RenderPoints(RowStartCB&& on_row_start, RenderPointCB&& on_render_point) {
  auto z = -5.0f;
  float red = 0.1f;
  for (auto y = 100; y < 480; y += 48) {
    float blue = 0.1f;
    on_row_start();
    for (auto x = 20; x < 640; x += 48) {
      on_render_point(static_cast<float>(x), static_cast<float>(y), z, red, 0.65f, blue);
      blue += 0.1f;
    }

    red += 0.1f;
    z += kZIncrementPerRow;
  }
}

static void RenderLoop(TestHost& host, int point_size, const PointParams* param_sets, uint32_t num_params,
                       bool use_shader) {
  // IMPORTANT: It appears that only stage 3 may be used for point sprites.
  // All other texture stages will not get the automatically expanded texture coordinates, leading to a single texel
  // being stretched across the expanded point.
  host.SetTextureStageEnabled(3, true);
  host.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                             TestHost::STAGE_2D_PROJECTIVE);
  auto& texture_stage = host.GetTextureStage(3);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  texture_stage.SetUWrap(TextureStage::WRAP_REPEAT);
  texture_stage.SetVWrap(TextureStage::WRAP_REPEAT);
  host.SetupTextureStages();
  host.SetFinalCombiner0(TestHost::SRC_TEX3, false, false, TestHost::SRC_DIFFUSE);
  host.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  int param_index;
  auto on_row_start = [&param_index]() { param_index = 0; };

  auto on_render_point = [&host, point_size, use_shader, param_sets, num_params, &param_index](
                             float x, float y, float z, float red, float green, float blue) {
    const auto& params = param_sets[param_index % num_params];
    ++param_index;

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SIZE, 8);
    Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_A, params.scaleFactorA);
    Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_B, params.scaleFactorB);
    Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_C, params.scaleFactorC);
    Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE, params.sizeRange);
    Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_1, params.sizeRangeDup1);
    Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_2, params.sizeRangeDup2);
    Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_BIAS, params.scaleBias);
    Pushbuffer::PushF(NV097_SET_POINT_PARAMS_MIN_SIZE, params.minSize);
    Pushbuffer::End();

    if (use_shader) {
      // Vertex shader iPts is not populated by NV097_SET_POINT_SIZE, so for the test to be hermetic, immediate mode
      // cannot be used.
      auto buffer = host.AllocateVertexBuffer(1);
      auto vertex = buffer->Lock();
      vertex->SetDiffuse(red, green, blue);
      vertex->SetPointSize(static_cast<float>(point_size) / 8.f);
      vertex->SetPosition(x, y, z, 1.f);
      vertex->SetTexCoord3(0.f, 0.f, 0.f, 1.f);
      buffer->Unlock();

      host.SetVertexBuffer(buffer);
      host.DrawArrays(TestHost::POSITION | TestHost::POINT_SIZE | TestHost::DIFFUSE | TestHost::TEXCOORD3,
                      TestHost::PRIMITIVE_POINTS);
    } else {
      Pushbuffer::Begin();
      Pushbuffer::Push(NV097_SET_POINT_SIZE, point_size);
      Pushbuffer::End();
      host.SetDiffuse(red, green, blue);
      host.Begin(TestHost::PRIMITIVE_POINTS);
      vector_t screen_point{x, y, z, 1.f};
      vector_t transformed;
      host.UnprojectPoint(transformed, screen_point, screen_point[2]);
      VectorCopyVector(screen_point, transformed);
      host.SetTexCoord3(0.f, 0.f, 0.f, 1.f);
      host.SetVertex(screen_point);
      host.End();
    }
  };

  RenderPoints(on_row_start, on_render_point);

  host.SetTextureStageEnabled(3, false);
  host.SetShaderStageProgram(TestHost::STAGE_NONE);
  host.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
}

void PointParamsTests::Test(const std::string& name, bool point_params_enabled, bool point_smooth_enabled,
                            int point_size, bool use_shader) {
  if (use_shader) {
    auto shader = std::make_shared<PassthroughVertexShader>();
    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
  }

  host_.PrepareDraw(0xFF222323);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, point_params_enabled);
  Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, point_smooth_enabled);
  Pushbuffer::End();

  RenderLoop(host_, point_size, kBasicPointParams, kNumBasicPointParamsSets, use_shader);

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, false);
    Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, false);
    Pushbuffer::Push(NV097_SET_POINT_SIZE, 8);
    Pushbuffer::End();
  }

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void PointParamsTests::TestDetailed(const std::string& name, bool use_shader) {
  if (use_shader) {
    auto shader = std::make_shared<PassthroughVertexShader>();
    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
  }

  host_.PrepareDraw(0xFF222324);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, true);
  Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, true);
  Pushbuffer::End();

  RenderLoop(host_, 128, kDetailedPointParams, kNumDetailedPointParamsSets, use_shader);

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, false);
    Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, false);
    Pushbuffer::Push(NV097_SET_POINT_SIZE, 8);
    Pushbuffer::End();
  }

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void PointParamsTests::TestScaleParams(bool scale_a, bool scale_b, bool scale_c, bool use_shader) {
  if (use_shader) {
    auto shader = std::make_shared<PassthroughVertexShader>();
    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
  }

  static constexpr float kPointSize = 16.25;
  static constexpr float kSizeRange = 64.f;
  static constexpr float kScaleBias = 0.f;
  static constexpr float kMinSize = 0.5f;

  host_.PrepareDraw(0xFF242323);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, true);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_A, scale_a ? 4.f : 0.f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_B, scale_b ? 1.f : 0.f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_FACTOR_C, scale_c ? 1.f : 0.f);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE, kSizeRange);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_1, kSizeRange);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SIZE_RANGE_DUP_2, kSizeRange);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_SCALE_BIAS, kScaleBias);
  Pushbuffer::PushF(NV097_SET_POINT_PARAMS_MIN_SIZE, kMinSize);
  Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, false);
  Pushbuffer::End();

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);

  static constexpr float kTop = 116.f;
  static constexpr float kInset = 32.f;
  static constexpr float kSpacing = 32.f;

  float x = kInset;
  float y = kTop;
  for (uint32_t z = 0; z < 192; ++z) {
    if (use_shader) {
      // Vertex shader iPts is not populated by NV097_SET_POINT_SIZE, so for the test to be hermetic, immediate mode
      // cannot be used.
      auto buffer = host_.AllocateVertexBuffer(1);
      auto vertex = buffer->Lock();
      vertex->SetDiffuse(0.93f, 0.33f, 0.33f);
      vertex->SetPointSize(kPointSize);
      vertex->SetPosition(x, y, static_cast<float>(z), 1.f);
      buffer->Unlock();

      host_.SetVertexBuffer(buffer);
      host_.DrawArrays(TestHost::POSITION | TestHost::POINT_SIZE | TestHost::DIFFUSE, TestHost::PRIMITIVE_POINTS);
    } else {
      host_.SetPointSize(kPointSize);
      host_.SetDiffuse(0xFF5555EE);
      host_.Begin(TestHost::PRIMITIVE_POINTS);
      vector_t screen_point{x, y, static_cast<float>(z), 1.f};
      vector_t transformed;
      host_.UnprojectPoint(transformed, screen_point, screen_point[2]);
      VectorCopyVector(screen_point, transformed);
      host_.SetVertex(screen_point);
      host_.End();
    }

    x += kSpacing;
    if (x >= host_.GetFramebufferWidthF() - kInset) {
      x = kInset;
      y += kSpacing;
    }
  }

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, false);
    Pushbuffer::Push(NV097_SET_POINT_PARAMS_ENABLE, false);
    Pushbuffer::Push(NV097_SET_POINT_SIZE, 8);
    Pushbuffer::End();
  }

  auto name = MakeScaleParamTestName(scale_a, scale_b, scale_c, use_shader);
  pb_print("%s\n", name.c_str());
  pb_print("sqrt(1/(%d + %d*D + %d*D^2))\n", scale_a ? 4 : 0, scale_b ? 1 : 0, scale_c ? 1 : 0);
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
