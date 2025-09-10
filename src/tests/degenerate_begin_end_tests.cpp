#include "degenerate_begin_end_tests.h"

#include <pbkit/pbkit.h>

#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "vertex_buffer.h"

static const char kTestBeginWithoutEnd[] = "BeginWithoutEnd";

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc BeginWithoutEnd
 *   Renders a series of 3 triangles and one quad. The middle two triangles send NV097_SET_BEGIN_END with a triangle
 *   primitive, but do not send NV097_SET_BEGIN_END with NV097_SET_BEGIN_END_OP_END when completed. The final quad is
 *   treated as a triangle by the nv2a as the previous triangle primitive was never ended.
 */
DegenerateBeginEndTests::DegenerateBeginEndTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Degenerate begin end", config) {
  tests_[kTestBeginWithoutEnd] = [this]() { TestBeginWithoutEnd(); };
}

void DegenerateBeginEndTests::Initialize() {
  TestSuite::Initialize();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void DegenerateBeginEndTests::TestBeginWithoutEnd() {
  host_.PrepareDraw(0xFE242424);

  static constexpr float z = 1.f;
  static constexpr float kHeader = 80.f;
  static constexpr float width = 64.f;
  const float x_offset = floorf(host_.GetFramebufferWidthF() / 5.f);

  float cx = x_offset;
  const float height = floorf((host_.GetFramebufferHeightF() - kHeader) / 10.f);
  float cy = kHeader + height;
  const float y_offset = height * 2.f;

  auto draw_triangle = [&cx, &cy, x_offset, height]() {
    const float left = floorf(cx - width / 2.f);
    const float right = floorf(left + width);
    const float top = floorf(cy - (height / 2.f));
    const float bottom = floorf(cy + (height / 2.f));

    Pushbuffer::PushF(NV097_SET_VERTEX3F, left, bottom, z);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, left, top, z);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, right, top, z);

    cx += x_offset;
  };

  auto draw_quad = [&cx, &cy, x_offset, height]() {
    const float left = floorf(cx - width / 2.f);
    const float right = floorf(left + width);
    const float top = floorf(cy - (height / 2.f));
    const float bottom = floorf(cy + (height / 2.f));

    Pushbuffer::PushF(NV097_SET_VERTEX3F, right, bottom, z);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, left, bottom, z);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, left, top, z);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, right, top, z);

    cx += x_offset;
  };

  host_.SetDiffuse(1.0f, 0.0f, 0.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  draw_triangle();
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  // Draw the second triangle with a begin but no end.
  host_.SetDiffuse(0.0f, 1.0f, 0.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  draw_triangle();
  Pushbuffer::End();

  host_.SetDiffuse(0.0f, 0.0f, 1.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  draw_triangle();
  Pushbuffer::End();

  host_.SetDiffuse(0.75f, 0.75f, 0.75f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_QUADS);
  draw_quad();
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  cx = x_offset;
  cy += y_offset;
  host_.SetDiffuse(1.0f, 0.0f, 0.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_QUADS);
  draw_quad();
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  host_.SetDiffuse(0.0f, 1.0f, 0.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_QUADS);
  draw_quad();
  Pushbuffer::End();

  host_.SetDiffuse(0.0f, 0.0f, 1.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  draw_triangle();
  Pushbuffer::End();

  host_.SetDiffuse(0.75f, 0.75f, 0.75f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  draw_triangle();
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  cx = x_offset;
  cy += y_offset;
  host_.SetDiffuse(1.0f, 0.0f, 0.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_QUADS);
  draw_quad();
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  host_.SetDiffuse(0.0f, 1.0f, 0.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_QUADS);
  draw_quad();
  Pushbuffer::End();

  host_.SetDiffuse(0.0f, 0.0f, 1.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  draw_triangle();
  Pushbuffer::End();

  host_.SetDiffuse(0.75f, 0.75f, 0.75f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_QUADS);
  draw_quad();
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  cx = x_offset;
  cy += y_offset;
  host_.SetDiffuse(1.0f, 0.0f, 0.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_POLYGON);
  draw_quad();
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  host_.SetDiffuse(0.0f, 1.0f, 0.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_POLYGON);
  draw_quad();
  Pushbuffer::End();

  host_.SetDiffuse(0.0f, 0.0f, 1.0f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
  draw_triangle();
  Pushbuffer::End();

  host_.SetDiffuse(0.75f, 0.75f, 0.75f);
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_QUADS);
  draw_quad();
  Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  Pushbuffer::End();

  pb_print("%s\n", kTestBeginWithoutEnd);
  pb_print("Center primitives omit NV097_SET_BEGIN_END_OP_END\n");
  pb_print("Tri: LB, LT, RT              Quad: RB, LB, LT, RT\n");
  pb_printat(5, 9, "Tri");
  pb_printat(5, 22, "Tri");
  pb_printat(5, 35, "Tri");
  pb_printat(5, 48, "Quad");

  pb_printat(8, 9, "Quad");
  pb_printat(8, 22, "Quad");
  pb_printat(8, 35, "Tri");
  pb_printat(8, 48, "Tri");

  pb_printat(11, 9, "Quad");
  pb_printat(11, 22, "Quad");
  pb_printat(11, 35, "Tri");
  pb_printat(11, 48, "Quad");

  pb_printat(15, 9, "Poly 4");
  pb_printat(15, 22, "Poly 4");
  pb_printat(15, 35, "Tri");
  pb_printat(15, 48, "Quad");

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestBeginWithoutEnd);
}
