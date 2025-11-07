#include "point_size_tests.h"

#include <xbox_math_d3d.h>
#include <xbox_math_matrix.h>

#include <memory>

#include "shaders/passthrough_vertex_shader.h"

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
    {"PointSmoothOn_04_VS", true, 4, true},   {"PointSmoothOff_04_VS", false, 4, true},
    {"PointSmoothOn_08_VS", true, 8, true},   {"PointSmoothOff_08_VS", false, 8, true},
    {"PointSmoothOn_16_VS", true, 16, true},  {"PointSmoothOff_16_VS", false, 16, true},
};

static const char kLargestValueTest[] = "LargestPointSize";

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
 * @tc LargestPointSize_FF
 * Using the fixed function pipeline:
 *   Top row renders a single point with a point size of 1 pixel (8), then a huge value with ~0x1FF set.
 *   Bottom row renders a single point with a point size of 511 (0x1FF), then a huge value with ~0x1FF set.
 *   64 pixel ruler quads are rendered near the huge value points.
 *   Demonstrates that values larger than 0x1FF are completely ignored rather than masked.
 *
 * @tc LargestPointSize_VS
 * Using the programmable pipeline:
 *   Top row renders a single point with a point size of 1 pixel (8), then a huge value with ~0x1FF set.
 *   Bottom row renders a single point with a point size of 511 (0x1FF), then a huge value with ~0x1FF set.
 *   64 pixel ruler quads are rendered near the huge value points.
 *   Demonstrates that values larger than 0x1FF are completely ignored rather than masked.
 *
 */
PointSizeTests::PointSizeTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Point size", config) {
  for (auto testConfig : testConfigs) {
    tests_[testConfig.name] = [this, testConfig]() {
      Test(testConfig.name, testConfig.point_smooth_enabled, testConfig.point_size_increment, testConfig.use_shader);
    };
  }

  for (auto use_shader : {false, true}) {
    std::string name = kLargestValueTest;
    name += use_shader ? "_VS" : "_FF";
    tests_[name] = [this, name, use_shader]() { TestLargestPointSize(name, use_shader); };
  }
}

void PointSizeTests::Initialize() {
  TestSuite::Initialize();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, false);
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_STENCIL_MASK, false);
  Pushbuffer::End();
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
    auto shader = std::make_shared<PassthroughVertexShader>();
    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
  }

  host_.PrepareDraw(0xFF222322);

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, point_smooth_enabled);
    Pushbuffer::End();
  }

  uint32_t point_size = 0;
  auto on_render_point = [this, use_shader, &point_size, point_size_increment](float x, float y, float z, float red,
                                                                               float green, float blue) {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SIZE, point_size);
    point_size += point_size_increment;
    Pushbuffer::End();

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
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, false);
    Pushbuffer::End();
  }

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void PointSizeTests::TestLargestPointSize(const std::string& name, bool use_shader) {
  if (use_shader) {
    auto shader = std::make_shared<PassthroughVertexShader>();
    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
  }

  // Anything over 0x1FF is silently ignored, even values passing a 0x1FF mask are discarded.
  static constexpr uint32_t kLargePointSize = 0x2F0;

  host_.PrepareDraw(0xFF444444);

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SMOOTH_ENABLE, false);
    Pushbuffer::End();
  }

  static constexpr float kRulerOffset = 2.f;
  static constexpr float kRulerSize = 2.f;
  static constexpr float kMaxPointSize = 64.f;

  auto draw = [this, use_shader](float cy) {
    float cx = floorf((host_.GetFramebufferWidthF() - kMaxPointSize) / 3.f);

    auto draw_point = [this, use_shader](float cx, float cy) {
      host_.Begin(TestHost::PRIMITIVE_POINTS);

      vector_t screen_point{cx, cy, 1.f, 1.f};
      if (!use_shader) {
        vector_t transformed;
        host_.UnprojectPoint(transformed, screen_point);
        VectorCopyVector(screen_point, transformed);
      }
      host_.SetVertex(screen_point);

      host_.End();
    };

    host_.SetDiffuse(0xFFBBBBBB);
    draw_point(cx, cy);

    host_.SetDiffuse(0xFF00BB00);
    cx *= 2.f;
    float left = cx - (kMaxPointSize * 0.5f);
    float top = cy - (kMaxPointSize * 0.5f);

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    if (!use_shader) {
      host_.DrawScreenQuad(left, top, left + kMaxPointSize, top + kMaxPointSize, 1.f);
      host_.DrawScreenQuad(left - (kRulerOffset + kRulerSize), top, left - kRulerOffset, top + kMaxPointSize, 1.f);
      host_.DrawScreenQuad(left, top - (kRulerOffset + kRulerSize), left + kMaxPointSize, top - kRulerOffset, 1.f);
    } else {
      auto draw_rect = [this](float left, float top, float right, float bottom) {
        host_.SetVertex(left, top, 1.f);
        host_.SetVertex(right, top, 1.f);
        host_.SetVertex(right, bottom, 1.f);
        host_.SetVertex(left, bottom, 1.f);
      };

      draw_rect(left, top, left + kMaxPointSize, top + kMaxPointSize);
      draw_rect(left - (kRulerOffset + kRulerSize), top, left - kRulerOffset, top + kMaxPointSize);
      draw_rect(left, top - (kRulerOffset + kRulerSize), left + kMaxPointSize, top - kRulerOffset);
    }
    host_.End();

    host_.SetDiffuse(0xFFFFFFFF);

    {
      Pushbuffer::Begin();
      Pushbuffer::Push(NV097_SET_POINT_SIZE, kLargePointSize);
      Pushbuffer::End();
    }
    draw_point(cx, cy);
  };

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SIZE, 0x08);
    Pushbuffer::End();
  }
  float cy = floorf((host_.GetFramebufferHeightF() - kMaxPointSize) / 3.f);
  draw(cy);

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_POINT_SIZE, 0x1FF);
    Pushbuffer::End();
  }
  draw(cy * 2);

  pb_print("%s\n", name.c_str());
  pb_print("Point size 0x%08X\n", kLargePointSize);

  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
