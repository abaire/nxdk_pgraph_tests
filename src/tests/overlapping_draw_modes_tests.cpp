#include "overlapping_draw_modes_tests.h"

#include <pbkit/pbkit.h>

#include "pbkit_ext.h"
#include "vertex_buffer.h"

static const char kArrElDrawArrArrElTest[] = "ArrElm_DrwArr_ArrElm";
static const char kDrawArrDrawArrTest[] = "DrwArr_DrwArr";
static const char kXemuSquashOptimizationTest[] = "SquashOpt";
static const char kXemuSquashOptimizationSingleDrawArraysTest[] = "SquashOptSingleArray";

static constexpr float kLeft = -2.75f;
static constexpr float kRight = 2.75f;
static constexpr float kTop = 1.75f;
static constexpr float kBottom = -1.75f;

OverlappingDrawModesTests::OverlappingDrawModesTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Overlapping draw modes", config) {
  tests_[kArrElDrawArrArrElTest] = [this]() { TestArrayElementDrawArrayArrayElement(); };
  tests_[kDrawArrDrawArrTest] = [this]() { TestDrawArrayDrawArray(); };
  tests_[kXemuSquashOptimizationTest] = [this]() { TestXemuSquashOptimization(); };
  tests_[kXemuSquashOptimizationSingleDrawArraysTest] = [this]() { TestXemuSquashOptimizationSingleDrawArrays(); };
}

void OverlappingDrawModesTests::Initialize() {
  TestSuite::Initialize();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void OverlappingDrawModesTests::CreateTriangleStrip() {
  constexpr int kNumTriangles = 6;
  auto buffer = host_.AllocateVertexBuffer(3 + (kNumTriangles - 1));

  auto vertex = buffer->Lock();
  index_buffer_.clear();
  auto index = 0;

  auto add_vertex = [&vertex, &index, this](float x, float y, float z, float r, float g, float b) {
    vertex->SetPosition(x, y, z);
    vertex->SetDiffuse(r, g, b);
    index_buffer_.push_back(index++);
    ++vertex;
  };

  const float z = 1.0f;
  add_vertex(kLeft, 0.0f, z, 1.0f, 0.0f, 0.0f);
  add_vertex(kLeft + 1.0f, kTop, z, 0.0f, 1.0f, 0.0f);
  add_vertex(kLeft + 0.5f, kBottom, z, 0.0f, 0.0f, 1.0f);

  add_vertex(kLeft + 1.5f, kTop - 0.5f, z, 0.25f, 0.0f, 0.8f);

  add_vertex(kLeft + 2.0f, kBottom + 0.5f, z, 0.75f, 0.0f, 0.25f);

  add_vertex(kLeft + 2.5f, kTop - 1.0f, z, 0.33f, 0.33f, 0.33f);

  add_vertex(kLeft + 3.0f, kBottom + 1.0f, z, 0.7f, 0.7f, 0.7f);

  add_vertex(kLeft + 3.5f, kTop, z, 0.5f, 0.5f, 0.6f);

  buffer->Unlock();
}

static void SetArrayElements(const uint32_t *next_index, uint32_t count) {
  while (count >= 2) {
    uint32_t index_pair = *next_index++ & 0xFFFF;
    index_pair += *next_index++ << 16;
    Pushbuffer::Push(NV097_ARRAY_ELEMENT16, index_pair);
    count -= 2;
  }
  if (count) {
    Pushbuffer::Push(NV097_ARRAY_ELEMENT32, *next_index);
  }
}

void OverlappingDrawModesTests::TestArrayElementDrawArrayArrayElement() {
  CreateTriangleStrip();

  host_.PrepareDraw(0xFE242424);

  host_.SetVertexBufferAttributes(host_.POSITION | host_.DIFFUSE);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLE_STRIP);

  // Specify the first four indices via ARRAY_ELEMENT invocations.
  {
    const uint32_t indices[] = {0, 1, 2, 3};
    SetArrayElements(indices, sizeof(indices) / sizeof(indices[0]));
  }

  // Then the next 3 via DRAW_ARRAYS
  auto vertex_buffer = host_.GetVertexBuffer();
  Pushbuffer::Push(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_DRAW_ARRAYS),
                   MASK(NV097_DRAW_ARRAYS_COUNT, 2) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 4));

  // And finally the last one via another ARRAY_ELEMENT command.
  {
    const uint32_t indices[] = {7};
    SetArrayElements(indices, sizeof(indices) / sizeof(indices[0]));
  }

  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kArrElDrawArrArrElTest);
}

void OverlappingDrawModesTests::CreateTriangles() {
  constexpr int kNumTriangles = 4;
  auto buffer = host_.AllocateVertexBuffer(kNumTriangles * 3);

  const float z = 1.0f;

  int index = 0;
  Color color_one, color_two, color_three;
  {
    color_one.SetGrey(0.25f);
    color_two.SetGrey(0.55f);
    color_three.SetGrey(0.75f);

    float one[] = {kLeft, kTop, z};
    float two[] = {0.0f, kTop, z};
    float three[] = {kLeft, 0.0f, z};
    buffer->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
  }

  {
    color_one.SetRGB(0.8f, 0.25f, 0.10f);
    color_two.SetRGB(0.1f, 0.85f, 0.10f);
    color_three.SetRGB(0.1f, 0.25f, 0.90f);

    float one[] = {kRight, kTop, z};
    float two[] = {0.10f, kBottom, z};
    float three[] = {0.25f, 0.0f, z};
    buffer->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
  }

  {
    color_one.SetRGB(0.8f, 0.25f, 0.90f);
    color_two.SetRGB(0.1f, 0.85f, 0.90f);
    color_three.SetRGB(0.85f, 0.95f, 0.10f);

    float one[] = {-0.4f, kBottom, z};
    float two[] = {-1.4f, -1.4, z};
    float three[] = {0.0f, 0.0f, z};
    buffer->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
  }

  {
    color_one.SetRGB(0.2f, 0.4f, 0.8f);
    color_two.SetRGB(0.3f, 0.5f, 0.9f);
    color_three.SetRGB(0.4f, 0.3f, 0.7f);

    float one[] = {kLeft, kBottom, z};
    float two[] = {kLeft, 0.0f, z};
    float three[] = {0.0f, 0.0f, z};
    buffer->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
  }

  index_buffer_.clear();
  for (auto i = 0; i < buffer->GetNumVertices(); ++i) {
    index_buffer_.push_back(i);
  }
}

void OverlappingDrawModesTests::TestDrawArrayDrawArray() {
  CreateTriangles();

  host_.PrepareDraw(0xFE242424);

  host_.SetVertexBufferAttributes(host_.POSITION | host_.DIFFUSE);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);

  // Draw the first triangle
  auto vertex_buffer = host_.GetVertexBuffer();
  Pushbuffer::Push(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_DRAW_ARRAYS),
                   MASK(NV097_DRAW_ARRAYS_COUNT, 2) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 0));

  // Draw the third triangle
  Pushbuffer::Push(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_DRAW_ARRAYS),
                   MASK(NV097_DRAW_ARRAYS_COUNT, 2) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 6));

  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kDrawArrDrawArrTest);
}

void OverlappingDrawModesTests::TestXemuSquashOptimization() {
  CreateTriangles();

  host_.PrepareDraw(0xFE242424);

  host_.SetVertexBufferAttributes(host_.POSITION | host_.DIFFUSE);

  Pushbuffer::Begin();
  auto vertex_buffer = host_.GetVertexBuffer();

  // Draw the first triangle as a Begin+DA+End triplet.
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  Pushbuffer::Push(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_DRAW_ARRAYS),
                   MASK(NV097_DRAW_ARRAYS_COUNT, 2) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 0));
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);

  // Draw the second triangle the same way.
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  Pushbuffer::Push(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_DRAW_ARRAYS),
                   MASK(NV097_DRAW_ARRAYS_COUNT, 2) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 3));
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);

  // Draw the third triangle via DRAW_ARRAYS and the fourth via ARRAY_ELEMENT.
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  Pushbuffer::Push(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_DRAW_ARRAYS),
                   MASK(NV097_DRAW_ARRAYS_COUNT, 2) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 6));

  {
    const uint32_t indices[] = {9, 10, 11};
    SetArrayElements(indices, sizeof(indices) / sizeof(indices[0]));
  }

  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);

  Pushbuffer::End();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kXemuSquashOptimizationTest);
}

void OverlappingDrawModesTests::TestXemuSquashOptimizationSingleDrawArrays() {
  // Tests an edge case in xemu where a single DrawArrays followed by ARRAY_ELEMENTS combines the draws but fails
  // to clear the DrawArrays count, leading to an assert.
  CreateTriangles();

  host_.PrepareDraw(0xFE242424);

  host_.SetVertexBufferAttributes(host_.POSITION | host_.DIFFUSE);

  Pushbuffer::Begin();
  auto vertex_buffer = host_.GetVertexBuffer();

  // Draw the third triangle via DRAW_ARRAYS and the fourth via ARRAY_ELEMENT.
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  Pushbuffer::Push(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_DRAW_ARRAYS),
                   MASK(NV097_DRAW_ARRAYS_COUNT, 2) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 6));

  {
    const uint32_t indices[] = {9, 10, 11};
    SetArrayElements(indices, sizeof(indices) / sizeof(indices[0]));
  }

  // Then draw the first triangle as another DrawArrays
  Pushbuffer::Push(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_DRAW_ARRAYS),
                   MASK(NV097_DRAW_ARRAYS_COUNT, 2) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 0));

  // Draw the second triangle the same way.
  Pushbuffer::Push(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_DRAW_ARRAYS),
                   MASK(NV097_DRAW_ARRAYS_COUNT, 2) | MASK(NV097_DRAW_ARRAYS_START_INDEX, 3));

  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);

  Pushbuffer::End();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kXemuSquashOptimizationSingleDrawArraysTest);
}
