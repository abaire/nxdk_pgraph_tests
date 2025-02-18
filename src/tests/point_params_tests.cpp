#include "point_params_tests.h"

#include <debug_output.h>
#include <xbox_math_d3d.h>
#include <xbox_math_matrix.h>

#include <memory>

#include "shaders/precalculated_vertex_shader.h"

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

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc PointParamsOn_SmoothOn_001_FF
 *   Renders points using the fixed function pipeline with point size 1, point smoothing and point params enabled.
 *   Demonstrates that point size is ignored when point params are enabled.
 *
 * @tc PointParamsOn_SmoothOn_128_FF
 *   Renders points using the fixed function pipeline with point size 128, point smoothing and point params enabled.
 *
 * @tc PointParamsOn_SmoothOff_128_FF
 *   Renders points using the fixed function pipeline with point size 128, point smoothing disabled and point params
 *   enabled.
 *
 * @tc PointParamsOn_SmoothOn_128_VS
 *   Renders points using a programmable shader with point size 128, point smoothing and point params enabled.
 *
 * @tc PointParamsOn_SmoothOff_128_VS
 *   Renders points using a programmable shader with point size 128, point smoothing disabled and point params enabled.
 *
 * @tc PointParamsOff_SmoothOff_128_VS
 *   Renders points using a programmable shader with point size 128, point smoothing and point params disabled.
 *
 * @tc PointParamsOff_SmoothOff_128_FF
 *   Renders points using the fixed function pipeline with point size 128, point smoothing and point params disabled.
 *
 * @tc Detailed_FF
 *   Renders points using the fixed function pipeline with more complex point params.
 *
 * @tc Detailed_VS
 *   Renders points using a programmable shader with more complex point params.
 */
PointParamsTests::PointParamsTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Point params", config) {
  for (auto test_config : kBasicTestConfigs) {
    tests_[test_config.name] = [this, test_config]() {
      this->Test(test_config.name, test_config.point_params_enabled, test_config.point_smooth_enabled,
                 test_config.point_size, test_config.use_shader);
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
}

void PointParamsTests::Initialize() {
  TestSuite::Initialize();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, false);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, false);
  pb_end(p);
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

static void RenderLoop(TestHost& host, const PointParams* param_sets, uint32_t num_params, bool use_shader) {
  int param_index;
  auto on_row_start = [&param_index]() { param_index = 0; };

  auto on_render_point = [&host, use_shader, param_sets, num_params, &param_index](float x, float y, float z, float red,
                                                                                   float green, float blue) {
    const auto& params = param_sets[param_index % num_params];
    ++param_index;

    auto p = pb_begin();
    p = pb_push1f(p, NV097_SET_POINT_PARAMS_SCALE_FACTOR_A, params.scaleFactorA);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS_SCALE_FACTOR_B, params.scaleFactorB);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS_SCALE_FACTOR_C, params.scaleFactorC);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS_SIZE_RANGE, params.sizeRange);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS_SIZE_RANGE_1, params.sizeRangeDup1);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS_SIZE_RANGE_2, params.sizeRangeDup2);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS_SCALE_BIAS, params.scaleBias);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS_MIN_SIZE, params.minSize);
    pb_end(p);

    host.Begin(TestHost::PRIMITIVE_POINTS);

    host.SetDiffuse(red, green, blue);

    vector_t screen_point{x, y, z, 1.f};
    if (!use_shader) {
      vector_t transformed;
      host.UnprojectPoint(transformed, screen_point, screen_point[2]);
      VectorCopyVector(screen_point, transformed);
    }
    PrintMsg("Drawing point %f, %f, %f\n", screen_point[0], screen_point[1], screen_point[2]);
    host.SetVertex(screen_point);

    host.End();
  };

  RenderPoints(on_row_start, on_render_point);
}

void PointParamsTests::Test(const std::string& name, bool point_params_enabled, bool point_smooth_enabled,
                            int point_size, bool use_shader) {
  if (use_shader) {
    auto shader = std::make_shared<PrecalculatedVertexShader>();
    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
  }

  host_.PrepareDraw(0xFF222323);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_POINT_PARAMS_ENABLE, point_params_enabled);
  p = pb_push1(p, NV097_SET_POINT_SMOOTH_ENABLE, point_smooth_enabled);
  p = pb_push1(p, NV097_SET_POINT_SIZE, point_size);
  pb_end(p);

  RenderLoop(host_, kBasicPointParams, kNumBasicPointParamsSets, use_shader);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_POINT_SMOOTH_ENABLE, false);
    p = pb_push1(p, NV097_SET_POINT_PARAMS_ENABLE, false);
    p = pb_push1(p, NV097_SET_POINT_SIZE, 8);
    pb_end(p);
  }

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void PointParamsTests::TestDetailed(const std::string& name, bool use_shader) {
  if (use_shader) {
    auto shader = std::make_shared<PrecalculatedVertexShader>();
    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
  }

  host_.PrepareDraw(0xFF222324);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_POINT_PARAMS_ENABLE, 1);
  p = pb_push1(p, NV097_SET_POINT_SMOOTH_ENABLE, 1);
  p = pb_push1(p, NV097_SET_POINT_SIZE, 128);
  pb_end(p);

  RenderLoop(host_, kDetailedPointParams, kNumDetailedPointParamsSets, use_shader);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_POINT_SMOOTH_ENABLE, false);
    p = pb_push1(p, NV097_SET_POINT_PARAMS_ENABLE, false);
    p = pb_push1(p, NV097_SET_POINT_SIZE, 8);
    pb_end(p);
  }

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
