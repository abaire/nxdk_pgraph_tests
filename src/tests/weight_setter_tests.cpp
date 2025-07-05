#include "weight_setter_tests.h"

#include <light.h>
#include <models/flat_mesh_grid_model.h>
#include <pbkit/pbkit.h>
#include <shaders/passthrough_vertex_shader.h>

#include "debug_output.h"
#include "test_host.h"

// clang-format off
static constexpr uint32_t kShader[] = {
#include "weight_to_color.vshinc"

};
// clang-format on
static constexpr char kTestName[] = "WeightSetter";

WeightSetterTests::WeightSetterTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Weight setter", config) {
  tests_[kTestName] = [this]() { Test(); };
}

void WeightSetterTests::Initialize() {
  TestSuite::Initialize();

  host_.SetCombinerControl(1);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
}

void WeightSetterTests::Test() {
  auto shader = std::make_shared<PassthroughVertexShader>();
  shader->SetShader(kShader, sizeof(kShader));
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(0xFF202020);

  static constexpr float kQuadWidth = 100.f;
  static constexpr float kQuadHeight = 24.f;

  auto draw_quads = [this](float left, float top) {
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetVertex(left, top, 0.f);
    host_.SetVertex(left + kQuadWidth, top, 0.f);
    host_.SetVertex(left + kQuadWidth, top + kQuadHeight, 0.f);
    host_.SetVertex(left, top + kQuadHeight, 0.f);
    host_.End();

    left *= 2.f;
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetVertex(left, top, 0.f);
    host_.SetVertex(left + kQuadWidth, top, 0.f);
    host_.SetVertex(left + kQuadWidth, top + kQuadHeight, 0.f);
    host_.SetVertex(left, top + kQuadHeight, 0.f);
    host_.End();
  };

  const float left = floorf((host_.GetFramebufferWidthF() - (kQuadWidth * 2.f)) / 3.f);
  float top = 96.f;

  host_.SetWeight(0.25f, 0.5f, 1.f, 0.75f);
  draw_quads(left, top);
  pb_printat(3, 0, ".25,.5,1,.75");

  host_.SetWeight(0.5f, 0.25f, 0.75f);
  top += kQuadHeight + 26.f;
  draw_quads(left, top);
  pb_printat(5, 0, ".5,.25,.75");

  host_.SetWeight(0.75f, 0.5f);
  top += kQuadHeight + 26.f;
  draw_quads(left, top);
  pb_printat(7, 0, ".75,.5");

  host_.SetWeight(0.25f);
  top += kQuadHeight + 26.f;
  draw_quads(left, top);
  pb_printat(9, 0, ".25");

  pb_printat(0, 0, "%s", kTestName);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestName);
}
