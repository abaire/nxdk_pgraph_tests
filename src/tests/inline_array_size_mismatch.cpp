#include "inline_array_size_mismatch.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"

static constexpr const char kTestName[] = "ExtraElements";

static void SetVertexAttribute(uint32_t index, uint32_t format, uint32_t size, uint32_t stride);
static void ClearVertexAttribute(uint32_t index);

InlineArraySizeMismatchTests::InlineArraySizeMismatchTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Inline array size mismatch", config) {
  tests_[kTestName] = [this]() { Test(); };
}

void InlineArraySizeMismatchTests::Initialize() {
  TestSuite::Initialize();
  host_.SetCombinerControl(1);
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
}

void InlineArraySizeMismatchTests::Test() {
  host_.PrepareDraw(0xFE312F31);

  SetVertexAttribute(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, 0);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_WEIGHT);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_NORMAL);
  SetVertexAttribute(NV2A_VERTEX_ATTR_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, 0);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_SPECULAR);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_FOG_COORD);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_POINT_SIZE);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_BACK_DIFFUSE);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_BACK_SPECULAR);
  SetVertexAttribute(NV2A_VERTEX_ATTR_TEXTURE0, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 2, 0);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_TEXTURE1);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_TEXTURE2);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_TEXTURE3);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_13);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_14);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_15);

  const float left_middle = host_.GetFramebufferWidthF() * 0.25f;
  const float triangle_width = host_.GetFramebufferHeightF() * 0.20f;
  const float left_left = left_middle - triangle_width;
  const float left_right = left_middle + triangle_width;

  const float right_middle = host_.GetFramebufferWidthF() - left_middle;
  const float right_left = right_middle - triangle_width;
  // const float right_right = right_middle + triangle_width;

  const float z = 0.0f;

  const float top = host_.GetFramebufferHeightF() * 0.25f;
  const float bottom = host_.GetFramebufferHeightF() - top;

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);

  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), left_left, bottom, z);
  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 1.0f, 0.0f, 0.0f, 1.0f);
  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 0.0f, 0.0f);

  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), left_middle, top, z);
  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 1.0f, 1.0f, 0.0f, 1.0f);
  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 0.0f, 0.0f);

  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), left_right, bottom, z);
  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 1.0f, 0.0f, 1.0f, 1.0f);
  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 0.0f, 0.0f);

  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), right_left, bottom, z);
  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 0.0f, 0.0f, 1.0f, 1.0f);
  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 0.0f, 0.0f);

  Pushbuffer::PushF(NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), right_middle, top, z);
  //  Pushbuffer::PushF( NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 0.0f, 1.0f, 1.0f, 1.0f);
  //  Pushbuffer::PushF( NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 0.0f, 0.0f);
  //
  //  Pushbuffer::PushF( NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), right_right, bottom, z);
  //  Pushbuffer::PushF( NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 0.0f, 0.0f, 1.0f, 1.0f);
  //  Pushbuffer::PushF( NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), 0.0f, 0.0f);
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);

  Pushbuffer::End();

  pb_print("%s\n\n", kTestName);
  pb_draw_text_screen();

  FinishDraw(kTestName);
}

static void SetVertexAttribute(uint32_t index, uint32_t format, uint32_t size, uint32_t stride) {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_VERTEX_DATA_ARRAY_FORMAT + index * 4,
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, format) |
                       MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, size) |
                       MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, stride));
  Pushbuffer::End();
}

static void ClearVertexAttribute(uint32_t index) {
  // Note: xemu has asserts on the count for several formats, so any format without that ASSERT must be used.
  SetVertexAttribute(index, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 0, 0);
}
