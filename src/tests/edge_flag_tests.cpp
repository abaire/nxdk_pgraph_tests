#include "edge_flag_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "vertex_buffer.h"

static std::string MakeTestName(bool edge_flag) { return edge_flag ? "Enabled" : "Disabled"; }

EdgeFlagTests::EdgeFlagTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Edge flag", config) {
  for (auto edge_flag : {false, true}) {
    const std::string test_name = MakeTestName(edge_flag);
    tests_[test_name] = [this, test_name, edge_flag]() { Test(test_name, edge_flag); };
  }
}

void EdgeFlagTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  // Edge flag has no visible effect if fill is enabled.
  Pushbuffer::Push(NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_LINE);
  // Note: This shouldn't strictly be necessary, but at the moment xemu disallows different fill modes for front and
  // back.
  Pushbuffer::Push(NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_LINE);
  Pushbuffer::End();
  host_.SetBlend(true);
}

void EdgeFlagTests::Deinitialize() {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_EDGE_FLAG, true);
  Pushbuffer::Push(NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
  Pushbuffer::Push(NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
  Pushbuffer::End();
}

static constexpr uint32_t kPalette[] = {
    0xFFFF3333, 0xFF33FF33, 0xFF3333FF, 0xFFFFFF33, 0xFF33FFFF, 0xFFFF33FF, 0xFF808080, 0xFFFFFFFF,
};
static constexpr uint32_t kNumPaletteEntries = sizeof(kPalette) / sizeof(kPalette[0]);

static void Draw(TestHost &host, float x, float y) {
  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {19.000000f, 15.000000f},  {56.000000f, 29.000000f},  {74.000000f, 77.000000f},  {45.694406f, 101.330152f},
        {36.000000f, 38.369815f},  {14.000000f, 23.000000f},  {85.000000f, 98.000000f},  {31.000000f, 40.000000f},
        {98.000000f, 38.000000f},  {17.000000f, 104.000000f}, {78.000000f, 65.000000f},  {31.644774f, 49.000000f},
        {105.000000f, 11.968842f}, {9.000000f, 40.000000f},   {105.962647f, 76.068491f}, {80.000000f, 87.000000f},
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
        {196.000000f, 47.000000f}, {194.000000f, 19.000000f},  {182.000000f, 97.000000f}, {106.186250f, 80.000000f},
        {127.000000f, 94.581630f}, {133.597443f, 13.763389f},  {205.000000f, 62.000000f}, {115.543920f, 16.000000f},
        {122.000000f, 3.000000f},  {117.062706f, 87.735558f},  {201.000000f, 28.664894f}, {146.114091f, 10.000000f},
        {205.356591f, 88.000000f}, {125.000000f, 107.000000f}, {190.828953f, 35.510304f}, {163.000000f, 105.956814f},
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
        {265.185925f, 35.000000f}, {249.778838f, 22.523717f}, {310.000000f, 19.000000f},
        {243.978231f, 47.650185f}, {304.000000f, 37.000000f}, {232.000000f, 102.522615f},
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
        {0.000000f, 239.000000f},  {0.000000f, 120.000000f},   {52.500000f, 225.400000f},
        {54.750000f, 175.500000f}, {105.000000f, 239.000000f}, {105.000000f, 120.000000f},
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
        {158.500000f, 205.000000f}, {115.545455f, 239.000000f}, {120.318182f, 120.000000f}, {147.045455f, 157.400000f},
        {158.500000f, 128.500000f}, {166.136364f, 145.500000f}, {204.318182f, 159.100000f}, {209.750000f, 239.000000f},
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
        {218.000000f, 232.000000f}, {237.772727f, 142.100000f}, {258.772727f, 120.000000f},
        {302.681818f, 169.300000f}, {317.000000f, 230.500000f},
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

  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {0.f, 350.f},
        {60.f, 340.f},
        {58.5f, 425.4f},
        {12.75f, 407.5f},
    };
    // clang-format on
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

void EdgeFlagTests::Test(const std::string &name, bool edge_flag) {
  host_.PrepareDraw(0xFF303234);

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_EDGE_FLAG, edge_flag);
    Pushbuffer::End();
  }

  const float x = host_.GetFramebufferWidthF() * 0.25f;
  const float y = host_.GetFramebufferHeightF() * 0.10f;
  Draw(host_, x, y);

  // Triangles, quads, and polygons can have the flag toggled during the draw action to hide specific lines.
  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {365.185925f, 35.000000f},
        {349.778838f, 22.523717f},
        {410.000000f, 19.000000f},

        {344.f, 48.f},
        {404.f, 37.f},
        {332.f, 103.f},
    };
    // clang-format on
    host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
    uint32_t i = 0;
    for (auto pt : kVertices) {
      Pushbuffer::Begin();
      Pushbuffer::Push(NV097_SET_EDGE_FLAG, edge_flag || (i & 0x01));
      Pushbuffer::End();
      host_.SetDiffuse(kPalette[i]);
      host_.SetVertex(x + pt[0], y + pt[1], 1.0f);
      i = (i + 1) % kNumPaletteEntries;
    }
    host_.End();
  }

  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {320.f, 232.0f}, {340.f, 142.1f}, {361.f, 120.0f}, {405.f, 169.3f}, {419.f, 230.5f},
    };
    // clang-format on
    host_.Begin(TestHost::PRIMITIVE_POLYGON);
    uint32_t i = 0;
    for (auto pt : kVertices) {
      Pushbuffer::Begin();
      Pushbuffer::Push(NV097_SET_EDGE_FLAG, edge_flag || !(i & 0x01));
      Pushbuffer::End();
      host_.SetDiffuse(kPalette[i]);
      host_.SetVertex(x + pt[0], y + pt[1], 1.0f);
      i = (i + 1) % kNumPaletteEntries;
    }
    host_.End();
  }

  {
    // clang-format off
    constexpr float kVertices[][2] = {
        {100.f, 350.f},
        {160.f, 340.f},
        {158.5f, 425.4f},
        {112.75f, 407.5f},
    };
    // clang-format on
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    uint32_t i = 0;
    for (auto pt : kVertices) {
      Pushbuffer::Begin();
      Pushbuffer::Push(NV097_SET_EDGE_FLAG, edge_flag || !(i & 0x01));
      Pushbuffer::End();
      host_.SetDiffuse(kPalette[i]);
      host_.SetVertex(x + pt[0], y + pt[1], 1.0f);
      i = (i + 1) % kNumPaletteEntries;
    }
    host_.End();
  }

  pb_print("%s\n", name.c_str());
  pb_printat(0, 15, (char *)"Pts");
  pb_printat(0, 28, (char *)"LLoop");
  pb_printat(0, 40, (char *)"Tri");
  pb_printat(0, 49, (char *)"Tri-IN");

  pb_printat(11, 15, (char *)"QStrip");
  pb_printat(11, 28, (char *)"TFan");
  pb_printat(11, 39, (char *)"Poly");
  pb_printat(11, 48, (char *)"Poly-IN");

  pb_printat(13, 15, (char *)"Quad");
  pb_printat(13, 24, (char *)"Quad-IN");

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
