#include "context_switch_tests.h"

#include "shaders/passthrough_vertex_shader.h"

static constexpr char kGraphicsClassZeroTest[] = "GRZero";

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc GraphicsClassZero
 *  Tests PGRAPH_CTX_SWITCH1 with the graphics class set to 0.
 */
ContextSwitchTests::ContextSwitchTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Context switch", config) {
  tests_[kGraphicsClassZeroTest] = [this]() { Test(); };
}

void ContextSwitchTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void ContextSwitchTests::Test() {
  auto set_crash_register = [](uint32_t reg, uint32_t value) -> uint32_t {
    auto crash_register = reinterpret_cast<uint32_t *>(PGRAPH_REGISTER_BASE + reg);
    auto ret = *crash_register;
    *crash_register = value;
    return ret;
  };

  auto flush = []() {
    Pushbuffer::Begin();
    for (auto i = 0; i < 16; ++i) {
      Pushbuffer::Push(NV097_NO_OPERATION, 0);
    }
    Pushbuffer::End(true);
  };

  host_.PrepareDraw(0xFE1F1F1F);

  host_.DrawCheckerboard(0xFF333333, 0xFF444444);

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  const auto left = 64.f;
  const auto right = host_.GetFramebufferWidthF() - left;
  const auto top = 48.f;
  const auto bottom = host_.GetFramebufferHeightF() - top;
  const auto midline = top + (bottom - top) * 0.5f;

  host_.Begin(TestHost::PRIMITIVE_POLYGON);

  host_.SetDiffuse(1.f, 0.f, 0.f, 0.75f);
  host_.SetVertex(left, midline, 0.1f, 1.0f);

  host_.SetDiffuse(0.f, 1.f, 0.f, 0.5f);
  host_.SetVertex(left, top, 0.1f, 1.0f);

  host_.SetDiffuse(0.f, 0.f, 1.f, 0.75f);
  host_.SetVertex(right, top, 0.1f, 1.0f);

  host_.SetDiffuse(0.3f, 0.4f, 0.7f, 0.66f);
  host_.SetVertex(right, midline, 0.1f, 1.0f);

  flush();

  auto original_value = set_crash_register(NV_PGRAPH_CTX_SWITCH1 & ~PGRAPH_REGISTER_BASE, 0);

  host_.SetDiffuse(0.9f, 0.f, 0.3f, 0.4f);
  host_.SetVertex(right, bottom, 0.1f, 1.0f);

  host_.SetDiffuse(0.5f, 0.9f, 0.3f, 0.75f);
  host_.SetVertex(left, bottom, 0.1f, 1.0f);

  host_.SetDiffuse(1.f, 0.9f, 0.4f, 0.75f);
  host_.SetVertex(left, midline, 0.1f, 1.0f);

  flush();
  set_crash_register(NV_PGRAPH_CTX_SWITCH1 & ~PGRAPH_REGISTER_BASE, original_value);

  host_.End();

  host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
  host_.SetVertex(16.f, top, 0.1f, 12.f);
  host_.SetVertex(32.f, midline, 0.1f, 12.f);
  host_.SetVertex(0.f, midline, 0.1f, 12.f);
  host_.End();

  pb_print("%s\n", kGraphicsClassZeroTest);
  pb_print("Bottom of screen is only drawn to with ctx disabled.\n");
  pb_print("Left triangle uses last color before ctx disable\n");
  pb_draw_text_screen();

  FinishDraw(kGraphicsClassZeroTest);
}
