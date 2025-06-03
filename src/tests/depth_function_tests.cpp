#include "depth_function_tests.h"

#include <pbkit/pbkit.h>

#include "../shaders/passthrough_vertex_shader.h"
#include "../test_host.h"
#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "vertex_buffer.h"

static constexpr char kTestName[] = "DepthFunc";

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc DepthFunc
 *   Demonstrates the behavior of the various depth test functions, including
 *   undefined values (which are ignored entirely).
 */
DepthFunctionTests::DepthFunctionTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Depth function", config) {
  tests_[kTestName] = [this]() { Test(); };
}

void DepthFunctionTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
}

void DepthFunctionTests::Test() {
  host_.PrepareDraw(0xFF000000);

  // Override the depth clip to ensure that max_val depths (post-projection) are never clipped.
  host_.SetDepthClip(0.0f, 16777216);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_STENCIL_MASK, false);
  Pushbuffer::End();

  static constexpr uint32_t kBackdropColor = 0xFFAAAAAA;
  static constexpr uint32_t kBehindColor = 0xFF3333FF;
  static constexpr uint32_t kSameColor = 0xFFFF3333;
  static constexpr uint32_t kFrontColor = 0xFF33FF33;

  static constexpr float kBehindZ = 1.f;
  static constexpr float kBackdropZ = 0.f;
  static constexpr float kFrontZ = -1.f;

  static constexpr float kStripWidth = 32.f;
  static constexpr float kStripHeight = 96.f;
  static constexpr float kTop = 64.f;
  static constexpr float kSwatchSize = 24.f;
  static constexpr float kInset = (kStripWidth - kSwatchSize) * 0.5f;

  auto draw_sub_test = [this](float x, float top, uint32_t depth_func, uint32_t pre_depth_func) {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
    Pushbuffer::End();

    const float bottom = top + kStripHeight;
    host_.SetDiffuse(kBackdropColor);
    host_.DrawScreenQuad(x + kInset, top, x + kStripWidth, bottom, kBackdropZ);

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_DEPTH_FUNC, pre_depth_func);
    Pushbuffer::Push(NV097_SET_DEPTH_FUNC, depth_func);
    Pushbuffer::End();

    host_.SetDiffuse(kBehindColor);
    const float kSwatchLeft = x;
    const float kSwatchRight = kSwatchLeft + kSwatchSize;
    host_.DrawScreenQuad(kSwatchLeft, top + kInset, kSwatchRight, top + kSwatchSize, kBehindZ);

    const float kMiddleTop = floor((top + (bottom - top) * 0.5f) - (kSwatchSize * 0.5f));
    host_.SetDiffuse(kSameColor);
    host_.DrawScreenQuad(kSwatchLeft, kMiddleTop, kSwatchRight, kMiddleTop + kSwatchSize, kBackdropZ);

    host_.SetDiffuse(kFrontColor);
    host_.DrawScreenQuad(kSwatchLeft, bottom - (kInset + kSwatchSize), kSwatchRight, bottom - kInset, kFrontZ);
  };

  struct TestCase {
    uint32_t depth_func;
    const char *name;
  };
  static constexpr TestCase kDepthFuncs[] = {
      {NV097_SET_DEPTH_FUNC_V_ALWAYS, "A"},    {NV097_SET_DEPTH_FUNC_V_NEVER, "N"},
      {NV097_SET_DEPTH_FUNC_V_LESS, "<"},      {NV097_SET_DEPTH_FUNC_V_EQUAL, "="},
      {NV097_SET_DEPTH_FUNC_V_LEQUAL, "<="},   {NV097_SET_DEPTH_FUNC_V_GREATER, ">"},
      {NV097_SET_DEPTH_FUNC_V_NOTEQUAL, "!="}, {NV097_SET_DEPTH_FUNC_V_GEQUAL, ">="},
  };

  float left = 32.f;
  int column = 2;
  for (auto &test : kDepthFuncs) {
    draw_sub_test(left, kTop, test.depth_func, test.depth_func);
    pb_printat(6, column, "%s", test.name);
    left += 40.f;
    column += 4;
  }

  // Draw a special test showing that unsupported values are ignored. The standard tests all draw an ALWAYS rect just
  // before setting test values, so any unsupported value will be equivalent to "ALWAYS".
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
    Pushbuffer::End();

    host_.SetDiffuse(kBackdropColor);

    static constexpr float kInvalidRowTop = 240.f;
    left = 32.f;
    column = 2;
    uint32_t unsupported_value = 0x00;
    for (auto &test : kDepthFuncs) {
      draw_sub_test(left, 200.f, unsupported_value, test.depth_func);
      draw_sub_test(left, 320.f, unsupported_value, NV097_SET_DEPTH_FUNC_V_ALWAYS);
      pb_printat(11, column, "%d", unsupported_value);
      left += 40.f;
      column += 4;
      unsupported_value += 10;
    }
  }

  pb_printat(0, 0, "%s\n", kTestName);

  pb_printat(8, 33, "Sets the above function");
  pb_printat(9, 33, "before the below value");

  pb_printat(13, 33, "Sets ALWAYS before the");
  pb_printat(14, 33, "above unsupported value");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestName, true);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  Pushbuffer::End();
}
