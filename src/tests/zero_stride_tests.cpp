#include "zero_stride_tests.h"

#include <pbkit/pbkit.h>

#include "pbkit_ext.h"
#include "test_host.h"
#include "vertex_buffer.h"

// clang-format off
static constexpr ZeroStrideTests::DrawMode kDrawModes[] = {
    ZeroStrideTests::DRAW_ARRAYS,
    ZeroStrideTests::DRAW_INLINE_ELEMENTS,
};
// clang-format on

static constexpr float kLeft = -2.75f;
static constexpr float kRight = 2.75f;
static constexpr float kTop = 1.75f;
static constexpr float kBottom = -1.75f;
static constexpr float kZFront = 1.0f;
static constexpr float kZBack = 5.0f;

ZeroStrideTests::ZeroStrideTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Zero stride", config) {
  for (const auto draw_mode : kDrawModes) {
    const std::string test_name = MakeTestName(draw_mode);
    auto test = [this, draw_mode]() { Test(draw_mode); };
    tests_[test_name] = test;
  }
}

void ZeroStrideTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  CreateGeometry();
}

void ZeroStrideTests::CreateGeometry() {
  constexpr int kNumTriangles = 3;
  auto buffer = host_.AllocateVertexBuffer(kNumTriangles * 3);

  int index = 0;
  Color color_one, color_two, color_three;
  {
    color_one.SetGrey(0.25f);
    color_two.SetGrey(0.55f);
    color_three.SetGrey(0.75f);

    float one[] = {kLeft, kTop, kZFront};
    float two[] = {0.0f, kTop, kZFront};
    float three[] = {kLeft, 0.0f, kZFront};
    buffer->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
  }

  {
    color_one.SetRGB(0.8f, 0.25f, 0.10f);
    color_two.SetRGB(0.1f, 0.85f, 0.10f);
    color_three.SetRGB(0.1f, 0.25f, 0.90f);

    float one[] = {kRight, kTop, kZFront};
    float two[] = {0.10f, kBottom, kZBack};
    float three[] = {0.25f, 0.0f, kZFront};
    buffer->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
  }

  {
    color_one.SetRGB(0.8f, 0.25f, 0.90f);
    color_two.SetRGB(0.1f, 0.85f, 0.90f);
    color_three.SetRGB(0.85f, 0.95f, 0.10f);

    float one[] = {-0.4f, kBottom, kZBack};
    float two[] = {-1.4f, -1.4, kZBack};
    float three[] = {0.0f, 0.0f, kZBack};
    buffer->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
  }

  index_buffer_.clear();
  for (auto i = 0; i < buffer->GetNumVertices(); ++i) {
    index_buffer_.push_back(i);
  }
}

void ZeroStrideTests::Test(DrawMode draw_mode) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::End();

  host_.OverrideVertexAttributeStride(TestHost::DIFFUSE, 0);

  uint32_t vertex_elements = host_.POSITION | host_.DIFFUSE;
  switch (draw_mode) {
    case DRAW_ARRAYS:
      host_.DrawArrays(vertex_elements);
      break;

    case DRAW_INLINE_ELEMENTS:
      host_.DrawInlineElements16(index_buffer_, vertex_elements);
      break;

    case DRAW_INLINE_ARRAYS:
      host_.DrawInlineArray(vertex_elements);
      break;
  }

  std::string name = MakeTestName(draw_mode);
  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  FinishDraw(name);
}

std::string ZeroStrideTests::MakeTestName(ZeroStrideTests::DrawMode draw_mode) {
  switch (draw_mode) {
    case DRAW_ARRAYS:
      return "DrawArrays";
    case DRAW_INLINE_ARRAYS:
      return "InlineArrays";
    case DRAW_INLINE_ELEMENTS:
      return "InlineElements";
  }
}
