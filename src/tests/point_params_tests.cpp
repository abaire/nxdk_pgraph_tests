#include "point_params_tests.h"

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

static const TestConfig kTestConfigs[]{
    {"PointParamsOn_SmoothOn_1_FF", true, true, 1, false},
    {"PointParamsOn_SmoothOff_1_FF", true, false, 1, false},
    {"PointParamsOff_SmoothOn_1_FF", false, true, 1, false},
    {"PointParamsOff_SmoothOff_1_FF", false, false, 1, false},
    {"PointParamsOn_SmoothOn_8_FF", true, true, 8, false},
    {"PointParamsOn_SmoothOff_8_FF", true, false, 8, false},
    {"PointParamsOff_SmoothOn_8_FF", false, true, 8, false},
    {"PointParamsOff_SmoothOff_8_FF", false, false, 8, false},
    {"PointParamsOn_SmoothOn_16_FF", true, true, 16, false},
    {"PointParamsOn_SmoothOff_16_FF", true, false, 16, false},
    {"PointParamsOff_SmoothOn_16_FF", false, true, 16, false},
    {"PointParamsOff_SmoothOff_16_FF", false, false, 16, false},
    {"PointParamsOn_SmoothOn_16_VS", true, true, 16, true},
    {"PointParamsOff_SmoothOff_16_VS", false, false, 16, true},
};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc PointParamsOn_SmoothOn_1_FF
 *   Renders points using the fixed function pipeline with point size 1, point smoothing and point params enabled.
 *
 * @tc PointParamsOn_SmoothOff_1_FF
 *   Renders points using the fixed function pipeline with point size 1, point smoothing disabled and point params
 *   enabled.
 *
 * @tc PointParamsOff_SmoothOn_1_FF
 *   Renders points using the fixed function pipeline with point size 1, point smoothing enabled and point params
 *   disabled.
 *
 * @tc PointParamsOff_SmoothOff_1_FF
 *   Renders points using the fixed function pipeline with point size 1, point smoothing and point params disabled.
 *
 * @tc PointParamsOn_SmoothOn_8_FF
 *   Renders points using the fixed function pipeline with point size 8, point smoothing and point params enabled.
 *
 * @tc PointParamsOn_SmoothOff_8_FF
 *   Renders points using the fixed function pipeline with point size 8, point smoothing disabled and point params
 *   enabled.
 *
 * @tc PointParamsOff_SmoothOn_8_FF
 *   Renders points using the fixed function pipeline with point size 8, point smoothing enabled and point params
 *   disabled.
 *
 * @tc PointParamsOff_SmoothOff_8_FF
 *   Renders points using the fixed function pipeline with point size 8, point smoothing and point params disabled.
 *
 * @tc PointParamsOn_SmoothOn_16_FF
 *   Renders points using the fixed function pipeline with point size 16, point smoothing and point params enabled.
 *
 * @tc PointParamsOn_SmoothOff_16_FF
 *   Renders points using the fixed function pipeline with point size 16, point smoothing disabled and point params
 *   enabled.
 *
 * @tc PointParamsOff_SmoothOn_16_FF
 *   Renders points using the fixed function pipeline with point size 16, point smoothing enabled and point params
 *   disabled.
 *
 * @tc PointParamsOff_SmoothOff_16_FF
 *   Renders points using the fixed function pipeline with point size 16, point smoothing and point params disabled.
 *
 * @tc PointParamsOn_SmoothOn_VS
 *   Renders points using a programmable shader with point size 16, point smoothing and point params enabled.
 *   Demonstrates that point params only affect the fixed function pipeline.
 *
 * @tc PointParamsOff_SmoothOff_VS
 *   Renders points using a programmable shader with point size 16, point smoothing and point params disabled.
 *   Demonstrates that point params only affect the fixed function pipeline.
 *
 */
PointParamsTests::PointParamsTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Point params", config) {
  for (auto test_config : kTestConfigs) {
    tests_[test_config.name] = [this, test_config]() {
      this->Test(test_config.name, test_config.point_params_enabled, test_config.point_smooth_enabled,
                 test_config.point_size, test_config.use_shader);
    };
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
  auto z = 0.f;
  float red = 0.1f;
  for (auto y = 100; y < 480; y += 35) {
    float blue = 0.1f;
    on_row_start();
    for (auto x = 20; x < 640; x += 60) {
      on_render_point(static_cast<float>(x), static_cast<float>(y), z, red, 0.65f, blue);
      blue += 0.1f;
    }

    red += 0.1f;
    z += 0.5f;
  }
}

void PointParamsTests::Test(const std::string& name, bool point_params_enabled, bool point_smooth_enabled,
                            int point_size, bool use_shader) {
  if (use_shader) {
    auto shader = std::make_shared<PrecalculatedVertexShader>();
    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
  }

  host_.PrepareDraw(0xFF222322);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_POINT_PARAMS_ENABLE, point_params_enabled);
    p = pb_push1(p, NV097_SET_POINT_SMOOTH_ENABLE, point_smooth_enabled);
    p = pb_push1(p, NV097_SET_POINT_SIZE, point_size);
    pb_end(p);
  }

  // GL_POINT_SIZE_MIN
  // GL_POINT_SIZE_MAX
  // GL_POINT_FADE_THRESHOLD_SIZE
  // GL_POINT_DISTANCE_ATTENUATION
  // GL_POINT_SPRITE_COORD_ORIGIN
  float point_params[8] = {0.f};

  auto on_row_start = [&point_params]() {
    point_params[0] = 0.017778f;
    point_params[1] = 0.f;
    point_params[2] = 0.f;
    point_params[3] = 8.f;
    point_params[4] = 16.f;
    point_params[5] = 32.f;
    point_params[6] = -0.f;
    point_params[7] = 0.f;
  };

  auto on_render_point = [this, use_shader, point_params](float x, float y, float z, float red, float green,
                                                          float blue) {
    auto p = pb_begin();
    p = pb_push1f(p, NV097_SET_POINT_PARAMS + 0, point_params[0]);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS + 4, point_params[1]);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS + 8, point_params[2]);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS + 12, point_params[3]);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS + 16, point_params[4]);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS + 20, point_params[5]);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS + 24, point_params[6]);
    p = pb_push1f(p, NV097_SET_POINT_PARAMS + 28, point_params[7]);

    pb_end(p);
    host_.PBKitBusyWait();

    host_.Begin(TestHost::PRIMITIVE_POINTS);
    host_.SetDiffuse(red, green, blue);
    vector_t screen_point{x, y, z, 1.f};

    if (!use_shader) {
      vector_t transformed;
      host_.UnprojectPoint(transformed, screen_point);
      VectorCopyVector(screen_point, transformed);
    }

    host_.SetVertex(screen_point);
    host_.End();
  };

  RenderPoints(on_row_start, on_render_point);

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
