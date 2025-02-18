#include "point_size_tests.h"

#include <xbox_math_d3d.h>
#include <xbox_math_matrix.h>

#include <memory>

#include "shaders/precalculated_vertex_shader.h"

struct TestConfig {
  const char* name;
  bool point_smooth_enabled;
  int point_size_increment;
  bool use_shader;
};

static const TestConfig testConfigs[]{
    {"PointSmoothOn_01_FF", true, 1, false},  {"PointSmoothOff_01_FF", false, 1, false},
    {"PointSmoothOn_04_FF", true, 4, false},  {"PointSmoothOff_04_FF", false, 4, false},
    {"PointSmoothOn_08_FF", true, 8, false},  {"PointSmoothOff_08_FF", false, 8, false},
    {"PointSmoothOn_16_FF", true, 16, false}, {"PointSmoothOff_16_FF", false, 16, false},
    {"PointSmoothOn_01_VS", true, 1, true},   {"PointSmoothOff_01_VS", false, 1, true},
    {"PointSmoothOn_04_VS", true, 4, true},   {"PointSmoothOff_01_VS", false, 4, true},
    {"PointSmoothOn_08_VS", true, 8, true},   {"PointSmoothOff_08_VS", false, 8, true},
    {"PointSmoothOn_16_VS", true, 16, true},  {"PointSmoothOff_16_VS", false, 16, true},
};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc PointSmoothOn_01_FF
 *   Renders points using the fixed function pipeline with point smoothing enabled. Point size starts at 0 and increases
 *   by 1 per point starting at the upper left point.
 *
 * @tc PointSmoothOff_01_FF
 *   Renders points using the fixed function pipeline with point smoothing disabled. Point size starts at 0 and
 * increases by 1 per point starting at the upper left point.
 *
 * @tc PointSmoothOn_04_FF
 *   Renders points using the fixed function pipeline with point smoothing enabled. Point size starts at 0 and increases
 *   by 4 per point starting at the upper left point.
 *
 * @tc PointSmoothOff_04_FF
 *   Renders points using the fixed function pipeline with point smoothing disabled. Point size starts at 0 and
 * increases by 4 per point starting at the upper left point.
 *
 * @tc PointSmoothOn_08_FF
 *   Renders points using the fixed function pipeline with point smoothing enabled. Point size starts at 0 and increases
 *   by 8 per point starting at the upper left point.
 *
 * @tc PointSmoothOff_08_FF
 *   Renders points using the fixed function pipeline with point smoothing disabled. Point size starts at 0 and
 * increases by 8 per point starting at the upper left point.
 *
 * @tc PointSmoothOn_16_FF
 *   Renders points using the fixed function pipeline with point smoothing enabled. Point size starts at 0 and increases
 *   by 16 per point starting at the upper left point.
 *
 * @tc PointSmoothOff_16_FF
 *   Renders points using the fixed function pipeline with point smoothing disabled. Point size starts at 0 and
 * increases by 16 per point starting at the upper left point.
 *
 * @tc PointSmoothOn_01_VS
 *   Renders points using a programmable shader with point smoothing enabled. Point size starts at 0 and increases
 *   by 1 per point starting at the upper left point.
 *
 * @tc PointSmoothOff_01_VS
 *   Renders points using a programmable shader with point smoothing disabled. Point size starts at 0 and increases
 *   by 1 per point starting at the upper left point.
 *
 * @tc PointSmoothOn_04_VS
 *   Renders points using a programmable shader with point smoothing enabled. Point size starts at 0 and increases
 *   by 1 per point starting at the upper left point.
 *
 * @tc PointSmoothOff_04_VS
 *   Renders points using a programmable shader with point smoothing disabled. Point size starts at 0 and increases
 *   by 1 per point starting at the upper left point.
 *
 * @tc PointSmoothOn_08_VS
 *   Renders points using a programmable shader with point smoothing enabled. Point size starts at 0 and increases
 *   by 8 per point starting at the upper left point.
 *
 * @tc PointSmoothOff_08_VS
 *   Renders points using a programmable shader with point smoothing disabled. Point size starts at 0 and increases
 *   by 8 per point starting at the upper left point.
 *
 * @tc PointSmoothOn_16_VS
 *   Renders points using a programmable shader with point smoothing enabled. Point size starts at 0 and increases
 *   by 16 per point starting at the upper left point.
 *
 * @tc PointSmoothOff_16_VS
 *   Renders points using a programmable shader with point smoothing disabled. Point size starts at 0 and increases
 *   by 16 per point starting at the upper left point.
 *
 */
PointSizeTests::PointSizeTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Point size", config) {
  for (auto testConfig : testConfigs) {
    tests_[testConfig.name] = [this, testConfig]() {
      this->Test(testConfig.name, testConfig.point_smooth_enabled, testConfig.point_size_increment,
                 testConfig.use_shader);
    };
  }
}

void PointSizeTests::Initialize() {
  TestSuite::Initialize();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, false);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, false);
  pb_end(p);
}

template <typename RenderPointCB>
static void RenderPoints(RenderPointCB&& on_render_point, int x_inc, int y_inc) {
  float red = 0.1f;
  for (auto y = 100; y < 480; y += y_inc) {
    float blue = 0.1f;
    for (auto x = 20; x < 640; x += x_inc) {
      on_render_point(static_cast<float>(x), static_cast<float>(y), 0.5f, red, 0.65f, blue);
      blue += 0.1f;
    }

    red += 0.1f;
  }
}

void PointSizeTests::Test(const std::string& name, bool point_smooth_enabled, int point_size_increment,
                          bool use_shader) {
  if (use_shader) {
    auto shader = std::make_shared<PrecalculatedVertexShader>();
    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
  }

  host_.PrepareDraw(0xFF222322);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_POINT_SMOOTH_ENABLE, point_smooth_enabled);
    pb_end(p);
  }

  uint32_t point_size = 0;
  auto on_render_point = [this, use_shader, &point_size, point_size_increment](float x, float y, float z, float red,
                                                                               float green, float blue) {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_POINT_SIZE, point_size);
    point_size += point_size_increment;
    pb_end(p);

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

  int x_inc = 40;
  int y_inc = 30;
  if (point_size_increment > 1) {
    x_inc = 64;
    y_inc = 64;
  }
  RenderPoints(on_render_point, x_inc, y_inc);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_POINT_SMOOTH_ENABLE, false);
    pb_end(p);
  }

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
