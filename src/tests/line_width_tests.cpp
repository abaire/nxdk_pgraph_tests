#include "line_width_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

static constexpr LineWidthTests::fixed_t kDefaultWidth = (1 << 3);

static std::string MakeTestName(bool is_filled, LineWidthTests::fixed_t width) {
  char buf[32];
  if (width == 0xFFFFFFFF) {
    snprintf(buf, sizeof(buf), "%s_FFFFFFFF", is_filled ? "Fill" : "Line");
  } else {
    snprintf(buf, sizeof(buf), "%s_%04u.%u", is_filled ? "Fill" : "Line", width >> 3, width & 0x7);
  }
  return buf;
}

LineWidthTests::LineWidthTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Line width") {
  // SET_LINE_WIDTH only affects lines, so the vast majority are unfilled.

  for (fixed_t line_width = 0; line_width < (2 << 3); ++line_width) {
    const std::string test_name = MakeTestName(false, line_width);
    tests_[test_name] = [this, test_name, line_width]() { Test(test_name, false, line_width); };
  }

  // Test larger values, skipping over fractional ones.
  for (fixed_t line_width = 3 << 3; line_width < (16 << 3); line_width += (1 << 3)) {
    const std::string test_name = MakeTestName(false, line_width);
    tests_[test_name] = [this, test_name, line_width]() { Test(test_name, false, line_width); };
  }

  // Even larger values, skipping by 8's
  for (fixed_t line_width = 16 << 3; line_width < (57 << 3); line_width += (8 << 3)) {
    const std::string test_name = MakeTestName(false, line_width);
    tests_[test_name] = [this, test_name, line_width]() { Test(test_name, false, line_width); };
  }

  // Step up towards 64, the max value that seems to have any effect.
  for (fixed_t line_width = 57 << 3; line_width < (65 << 3); line_width += (1 << 3)) {
    const std::string test_name = MakeTestName(false, line_width);
    tests_[test_name] = [this, test_name, line_width]() { Test(test_name, false, line_width); };
  }

  // Show behavior around the boundary point.
  for (fixed_t line_width = 63 << 3; line_width < (65 << 3); ++line_width) {
    const std::string test_name = MakeTestName(false, line_width);
    tests_[test_name] = [this, test_name, line_width]() { Test(test_name, false, line_width); };
  }

  // Test -1
  {
    const std::string test_name = MakeTestName(false, 0xFFFFFFFF);
    tests_[test_name] = [this, test_name]() { Test(test_name, false, 0xFFFFFFFF); };
  }

  // Add a few filled tests to prove that there's no effect on filled polys.
  for (auto line_width : {0, 1 << 3, 32 << 3}) {
    const std::string test_name = MakeTestName(true, line_width);
    tests_[test_name] = [this, test_name, line_width]() { Test(test_name, true, line_width); };
  }
}

void LineWidthTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  pb_end(p);
  host_.SetBlend(true);
}

static void SetFill(bool enabled) {
  auto p = pb_begin();
  uint32_t fill_mode = enabled ? NV097_SET_FRONT_POLYGON_MODE_V_FILL : NV097_SET_FRONT_POLYGON_MODE_V_LINE;
  p = pb_push1(p, NV097_SET_FRONT_POLYGON_MODE, fill_mode);
  // Note: This shouldn't strictly be necessary, but at the moment xemu disallows different fill modes for front and
  // back.
  p = pb_push1(p, NV097_SET_BACK_POLYGON_MODE, fill_mode);
  pb_end(p);
}

void LineWidthTests::TearDownTest() {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LINE_WIDTH, kDefaultWidth);
  pb_end(p);
  SetFill(true);
}

static constexpr uint32_t kPalette[] = {
    0xFFFF3333, 0xFF33FF33, 0xFF3333FF, 0xFFFFFF33, 0xFF33FFFF, 0xFFFF33FF, 0xFF808080, 0xFFFFFFFF,
};
static constexpr uint32_t kNumPaletteEntries = sizeof(kPalette) / sizeof(kPalette[0]);

static void Draw(TestHost &host, float x, float y) {
  {
    // clang format off
    constexpr float kVertices[][2] = {
        {19.000000f, 15.000000f},  {56.000000f, 29.000000f},  {74.000000f, 77.000000f},  {45.694406f, 101.330152f},
        {36.000000f, 38.369815f},  {14.000000f, 23.000000f},  {85.000000f, 98.000000f},  {31.000000f, 40.000000f},
        {98.000000f, 38.000000f},  {17.000000f, 104.000000f}, {78.000000f, 65.000000f},  {31.644774f, 49.000000f},
        {105.000000f, 11.968842f}, {9.000000f, 40.000000f},   {105.962647f, 76.068491f}, {80.000000f, 87.000000f},
    };
    // clang format on
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
    // clang format off
    constexpr float kVertices[][2] = {
        {196.000000f, 47.000000f}, {194.000000f, 19.000000f},  {182.000000f, 97.000000f}, {106.186250f, 80.000000f},
        {127.000000f, 94.581630f}, {133.597443f, 13.763389f},  {205.000000f, 62.000000f}, {115.543920f, 16.000000f},
        {122.000000f, 3.000000f},  {117.062706f, 87.735558f},  {201.000000f, 28.664894f}, {146.114091f, 10.000000f},
        {205.356591f, 88.000000f}, {125.000000f, 107.000000f}, {190.828953f, 35.510304f}, {163.000000f, 105.956814f},
    };
    // clang format on
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
    // clang format off
    constexpr float kVertices[][2] = {
        {265.185925f, 35.000000f}, {249.778838f, 22.523717f}, {310.000000f, 19.000000f},
        {243.978231f, 47.650185f}, {304.000000f, 37.000000f}, {232.000000f, 102.522615f},
    };
    // clang format on
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
    // clang format off
    constexpr float kVertices[][2] = {
        {0.000000f, 239.000000f},  {0.000000f, 120.000000f},   {52.500000f, 225.400000f},
        {54.750000f, 175.500000f}, {105.000000f, 239.000000f}, {105.000000f, 120.000000f},
    };
    // clang format on
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
    // clang format off
    constexpr float kVertices[][2] = {
        {158.500000f, 205.000000f}, {115.545455f, 239.000000f}, {120.318182f, 120.000000f}, {147.045455f, 157.400000f},
        {158.500000f, 128.500000f}, {166.136364f, 145.500000f}, {204.318182f, 159.100000f}, {209.750000f, 239.000000f},
    };
    // clang format on
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
    // clang format off
    constexpr float kVertices[][2] = {
        {218.000000f, 232.000000f}, {237.772727f, 142.100000f}, {258.772727f, 120.000000f},
        {302.681818f, 169.300000f}, {317.000000f, 230.500000f},
    };
    // clang format on
    host.Begin(TestHost::PRIMITIVE_POLYGON);
    uint32_t i = 0;
    for (auto pt : kVertices) {
      host.SetDiffuse(kPalette[i]);
      host.SetVertex(x + pt[0], y + pt[1], 1.0f);
      i = (i + 1) % kNumPaletteEntries;
    }
    host.End();
  }

  {
    // clang format off
    constexpr float kVertices[][2] = {
        {0.f, 350.f},
        {60.f, 340.f},
        {58.5f, 425.4f},
        {12.75f, 407.5f},
    };
    // clang format on
    host.Begin(TestHost::PRIMITIVE_QUADS);
    uint32_t i = 0;
    for (auto pt : kVertices) {
      host.SetDiffuse(kPalette[i]);
      host.SetVertex(x + pt[0], y + pt[1], 1.0f);
      i = (i + 1) % kNumPaletteEntries;
    }
    host.End();
  }
}

void LineWidthTests::Test(const std::string &name, bool fill, fixed_t width) {
  host_.PrepareDraw(0xFF202224);

  SetFill(fill);
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LINE_WIDTH, width);
    pb_end(p);
  }

  const float x = host_.GetFramebufferWidthF() * 0.25f;
  const float y = host_.GetFramebufferHeightF() * 0.10f;
  Draw(host_, x, y);

  pb_print("%s\n", name.c_str());
  pb_printat(0, 15, (char *)"Pts");
  pb_printat(0, 28, (char *)"LLoop");
  pb_printat(0, 40, (char *)"Tri");

  pb_printat(11, 15, (char *)"QStrip");
  pb_printat(11, 28, (char *)"TFan");
  pb_printat(11, 39, (char *)"Poly");

  pb_printat(13, 15, (char *)"Quad");

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}
