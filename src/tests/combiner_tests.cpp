#include "combiner_tests.h"

#include <pbkit/pbkit.h>

#include "pbkit_ext.h"
#include "test_host.h"
#include "vertex_buffer.h"

static constexpr const char* kTestName = "Mux";

CombinerTests::CombinerTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Combiner") {
  auto test = [this]() { Test(); };
  tests_[kTestName] = test;
}

void CombinerTests::Initialize() {
  TestSuite::Initialize();

  host_.SetShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  CreateGeometry();
}

void CombinerTests::Deinitialize() {
  for (auto& buffer : vertex_buffers_) {
    buffer.reset();
  }
  TestSuite::Deinitialize();
}

void CombinerTests::CreateGeometry() {
  static constexpr float kLeft = -2.75f;
  static constexpr float kRight = 2.75f;
  static constexpr float kTop = 1.85f;
  static constexpr float kBottom = -1.75f;
  static constexpr float kZFront = 1.0f;
  static constexpr float kZBack = 5.0f;
  static const float kSpacing = 0.1f;
  static const float kWidth = kRight - kLeft;
  static const float kHeight = kTop - kBottom;
  static const float kUnitWidth = (kWidth - (3.0f * kSpacing)) / 4.0f;
  static const float kUnitHeight = (kHeight - (2.0f * kSpacing)) / 3.0f;

  Color c_one{0.0f, 1.0f, 0.0f};
  Color c_two{0.0f, 0.0f, 1.0f};
  Color c_three{1.0f, 0.0f, 0.0f};
  Color c_four{0.25f, 0.25f, 0.25f};

  float left = kLeft;
  float top = kTop - kSpacing;
  for (auto& buffer : vertex_buffers_) {
    uint32_t num_quads = 1;
    buffer = host_.AllocateVertexBuffer(6 * num_quads);

    const float z = 1.0f;
    buffer->DefineBiTri(0, left, top, left + kUnitWidth, top - kUnitHeight, z, z, z, z, c_one, c_two, c_three, c_four);

    left += kUnitWidth + kSpacing;
    if (left >= (kRight - kUnitWidth)) {
      left = kLeft;
      top -= kUnitHeight + kSpacing * 4.0f;
    }
  }
}

void CombinerTests::Test() {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  uint32_t vertex_elements = host_.POSITION | host_.DIFFUSE | host_.SPECULAR;

  host_.SetCombinerControl(2, false, false, false);

  // TODO: Test behavior when r0 is not explicitly set.
  host_.SetInputColorCombiner(0, TestHost::OneInput(), TestHost::OneInput());
  host_.SetOutputColorCombiner(0, TestHost::DST_R0);

  host_.SetOutputAlphaCombiner(0, TestHost::DST_R0);

  host_.SetCombinerFactorC0(1, 1.0f, 0.0f, 0.0f, 1.0f);
  host_.SetCombinerFactorC1(1, 0.0f, 0.0f, 1.0f, 1.0f);
  host_.SetInputColorCombiner(1, TestHost::ColorInput(TestHost::SRC_C0), TestHost::OneInput(),
                              TestHost::ColorInput(TestHost::SRC_C1), TestHost::OneInput());
  host_.SetOutputColorCombiner(1, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_DIFFUSE, false, false,
                               TestHost::SM_MUX);

  host_.SetInputAlphaCombiner(1, TestHost::OneInput(), TestHost::OneInput());
  host_.SetOutputAlphaCombiner(1, TestHost::DST_DIFFUSE);

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  int row = 5;

  // Set an alpha value with the MSB set and the LSB unset.
  uint32_t c0 = 0x82000000;
  host_.SetCombinerFactorC0(0, c0);
  host_.SetInputAlphaCombiner(0, TestHost::AlphaInput(TestHost::SRC_C0), TestHost::OneInput());
  host_.SetCombinerControl(2, false, false, false);
  pb_printat(row, 11, (char*)"LSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[0]);
  host_.DrawArrays(vertex_elements);

  host_.SetCombinerControl(2, false, false, true);
  pb_printat(row, 21, (char*)"MSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[1]);
  host_.DrawArrays(vertex_elements);

  // Set an alpha value with the MSB and LSB set.
  c0 = 0x81000000;
  host_.SetCombinerFactorC0(0, c0);
  host_.SetCombinerControl(2, false, false, false);
  pb_printat(row, 31, (char*)"LSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[2]);
  host_.DrawArrays(vertex_elements);

  host_.SetCombinerControl(2, false, false, true);
  pb_printat(row, 41, (char*)"MSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[3]);
  host_.DrawArrays(vertex_elements);

  row = 9;

  // Set an alpha value with the MSB and LSB unset.
  c0 = 0x00000000;
  host_.SetCombinerFactorC0(0, c0);
  host_.SetCombinerControl(2, false, false, false);
  pb_printat(row, 11, (char*)"LSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[4]);
  host_.DrawArrays(vertex_elements);

  host_.SetCombinerControl(2, false, false, true);
  pb_printat(row, 21, (char*)"MSB 0x%x", (c0 >> 24));
  host_.SetVertexBuffer(vertex_buffers_[5]);
  host_.DrawArrays(vertex_elements);

  pb_printat(0, 0, (char*)"%s\n", kTestName);
  pb_printat(1, 0, (char*)"Unset = Red");
  pb_printat(2, 0, (char*)"Set = Blue");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kTestName);
}
