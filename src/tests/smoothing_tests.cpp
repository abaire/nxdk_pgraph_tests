#include "smoothing_tests.h"

#include <pbkit/pbkit.h>

#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

// clang-format off
static constexpr uint32_t kSmoothControlValues[] = {
    0xFFFF0000,
    0xFFFF0001,
};
// clang-format on

static std::string MakeTestName(uint32_t smooth_control) {
  char buf[64] = {0};
  snprintf(buf, sizeof(buf), "0x%08X", smooth_control);
  return buf;
}

SmoothingTests::SmoothingTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Smoothing control") {
  for (auto smooth_control : kSmoothControlValues) {
    const std::string test_name = MakeTestName(smooth_control);
    tests_[test_name] = [this, test_name, smooth_control]() { Test(test_name, smooth_control); };
  }
}

void SmoothingTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  pb_end(p);
}

static constexpr uint32_t kPalette[] = {
    0xFFFF3333, 0xFF33FF33, 0xFF3333FF, 0xFFFFFF33, 0xFF33FFFF, 0xFFFF33FF, 0xFF808080, 0xFFFFFFFF,
};
static constexpr uint32_t kNumPaletteEntries = sizeof(kPalette) / sizeof(kPalette[0]);

static void Draw(TestHost& host, float x, float y) {
  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {19.000000f, 15.000000f},
        {56.000000f, 29.000000f},
        {74.000000f, 77.000000f},
        {45.694406f, 101.330152f},
        {36.000000f, 38.369815f},
        {14.000000f, 23.000000f},
        {85.000000f, 98.000000f},
        {31.000000f, 40.000000f},
        {98.000000f, 38.000000f},
        {17.000000f, 104.000000f},
        {78.000000f, 65.000000f},
        {31.644774f, 49.000000f},
        {105.000000f, 11.968842f},
        {9.000000f, 40.000000f},
        {105.962647f, 76.068491f},
        {80.000000f, 87.000000f},
    };
    // clang-format on
    host.Begin(TestHost::PRIMITIVE_POINTS);
    uint32_t i = 0;
    for (auto pt : kVertices) {
      host.SetDiffuse(kPalette[i]);
      host.SetVertex(x + pt[0], y + pt[1], 1.0f);
      i = (i + 1) % kNumPaletteEntries;
    }
    host.End();
  }

  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {196.000000f, 47.000000f},
        {194.000000f, 19.000000f},
        {182.000000f, 97.000000f},
        {106.186250f, 80.000000f},
        {127.000000f, 94.581630f},
        {133.597443f, 13.763389f},
        {205.000000f, 62.000000f},
        {115.543920f, 16.000000f},
        {122.000000f, 3.000000f},
        {117.062706f, 87.735558f},
        {201.000000f, 28.664894f},
        {146.114091f, 10.000000f},
        {205.356591f, 88.000000f},
        {125.000000f, 107.000000f},
        {190.828953f, 35.510304f},
        {163.000000f, 105.956814f},
    };
    // clang-format on
    host.Begin(TestHost::PRIMITIVE_LINE_LOOP);
    uint32_t i = 0;
    for (auto pt : kVertices) {
      host.SetDiffuse(kPalette[i]);
      host.SetVertex(x + pt[0], y + pt[1], 1.0f);
      i = (i + 1) % kNumPaletteEntries;
    }
    host.End();
  }

  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {265.185925f, 35.000000f},
        {249.778838f, 22.523717f},
        {310.000000f, 19.000000f},
        {243.978231f, 47.650185f},
        {304.000000f, 37.000000f},
        {232.000000f, 102.522615f},
    };
    // clang-format on
    host.Begin(TestHost::PRIMITIVE_TRIANGLES);
    uint32_t i = 0;
    for (auto pt : kVertices) {
      host.SetDiffuse(kPalette[i]);
      host.SetVertex(x + pt[0], y + pt[1], 1.0f);
      i = (i + 1) % kNumPaletteEntries;
    }
    host.End();
  }

  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {0.000000f, 239.000000f},
        {0.000000f, 120.000000f},
        {52.500000f, 225.400000f},
        {54.750000f, 175.500000f},
        {105.000000f, 239.000000f},
        {105.000000f, 120.000000f},
    };
    // clang-format on
    host.Begin(TestHost::PRIMITIVE_QUAD_STRIP);
    uint32_t i = 0;
    for (auto pt : kVertices) {
      host.SetDiffuse(kPalette[i]);
      host.SetVertex(x + pt[0], y + pt[1], 1.0f);
      i = (i + 1) % kNumPaletteEntries;
    }
    host.End();
  }

  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {158.500000f, 205.000000f},
        {115.545455f, 239.000000f},
        {120.318182f, 120.000000f},
        {147.045455f, 157.400000f},
        {158.500000f, 128.500000f},
        {166.136364f, 145.500000f},
        {204.318182f, 159.100000f},
        {209.750000f, 239.000000f},
    };
    // clang-format on
    host.Begin(TestHost::PRIMITIVE_TRIANGLE_FAN);
    uint32_t i = 0;
    for (auto pt : kVertices) {
      host.SetDiffuse(kPalette[i]);
      host.SetVertex(x + pt[0], y + pt[1], 1.0f);
      i = (i + 1) % kNumPaletteEntries;
    }
    host.End();
  }

  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {218.000000f, 232.000000f},
        {237.772727f, 142.100000f},
        {258.772727f, 120.000000f},
        {302.681818f, 169.300000f},
        {317.000000f, 230.500000f},
    };
    // clang-format on
    host.Begin(TestHost::PRIMITIVE_POLYGON);
    uint32_t i = 0;
    for (auto pt : kVertices) {
      host.SetDiffuse(kPalette[i]);
      host.SetVertex(x + pt[0], y + pt[1], 1.0f);
      i = (i + 1) % kNumPaletteEntries;
    }
    host.End();
  }
}

void SmoothingTests::Test(const std::string& name, uint32_t smooth_control) {
  host_.PrepareDraw(0xFF32343A);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SMOOTHING_CONTROL, smooth_control);
    pb_end(p);
  }
  host_.SetBlend(true);

  auto draw = [this](float x, float y, bool line_smooth, bool poly_smooth) {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LINE_SMOOTH_ENABLE, line_smooth);
    p = pb_push1(p, NV097_SET_POLY_SMOOTH_ENABLE, poly_smooth);
    pb_end(p);

    Draw(host_, x, y);
  };

  const float mid_x = host_.GetFramebufferWidthF() * 0.5f;
  const float mid_y = host_.GetFramebufferHeightF() * 0.5f;
  draw(0.0f, 0.0f, false, false);
  draw(mid_x, 0.0f, true, false);
  draw(0.0f, mid_y, false, true);
  draw(mid_x, mid_y, true, true);

  {
    auto p = pb_begin();
    // From Tenchu: Return from Darkness, this disables poly smoothing regardless of whether the individual flags are
    // enabled.
    p = pb_push1(p, NV097_SET_SMOOTHING_CONTROL, 0xFFFF0001);
    p = pb_push1(p, NV097_SET_LINE_SMOOTH_ENABLE, false);
    p = pb_push1(p, NV097_SET_POLY_SMOOTH_ENABLE, false);
    pb_end(p);
  }

  pb_print("%s\n", name.c_str());
  pb_printat(0, 32, (char*)"LS");
  pb_printat(10, 0, (char*)"PS");
  pb_printat(10, 32, (char*)"LSPS");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}
