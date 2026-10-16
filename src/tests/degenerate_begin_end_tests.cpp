#include "degenerate_begin_end_tests.h"

#include <pbkit/pbkit.h>

#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "vertex_buffer.h"

static const char kTestBeginWithoutEnd[] = "BeginWithoutEnd";
static const char kTestEmptyBeginEnd[] = "EmptyBeginEnd";

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc BeginWithoutEnd
 *   Renders a series of 3 triangles and one quad. The middle two triangles send NV097_SET_BEGIN_END with a triangle
 *   primitive, but do not send NV097_SET_BEGIN_END with NV097_SET_BEGIN_END_OP_END when completed. The final quad is
 *   treated as a triangle by the nv2a as the previous triangle primitive was never ended.
 *
 * @tc EmptyBeginEnd
 *   Renders 7 rows, each labeled with a primitive type. Each row draws a red triangle, a green quad, and a blue
 *   triangle, each in its own properly terminated NV097_SET_BEGIN_END block. Between each pair of shapes, an empty
 *   NV097_SET_BEGIN_END block using the row's primitive type is sent (BEGIN immediately followed by END with no
 *   vertices). Each row is expected to contain exactly the three solid shapes, with no missing, extra, or misshapen
 *   geometry.
 */
DegenerateBeginEndTests::DegenerateBeginEndTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Degenerate begin end", config) {
  tests_[kTestBeginWithoutEnd] = [this]() { TestBeginWithoutEnd(); };
  tests_[kTestEmptyBeginEnd] = [this]() { TestEmptyBeginEnd(); };
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

  FinishDraw(kTestBeginWithoutEnd);
}

void DegenerateBeginEndTests::TestEmptyBeginEnd() {
  host_.PrepareDraw(0xFE242424);

  pb_print("%s: no-vertex BEGIN(prim)/END between shapes\n", kTestEmptyBeginEnd);
  pb_print("Expect each row: red Tri, green Quad, blue Tri\n");

  static constexpr TestHost::DrawPrimitive kEmptyPrimitives[] = {
      TestHost::PRIMITIVE_POINTS,    TestHost::PRIMITIVE_LINES,          TestHost::PRIMITIVE_LINE_STRIP,
      TestHost::PRIMITIVE_TRIANGLES, TestHost::PRIMITIVE_TRIANGLE_STRIP, TestHost::PRIMITIVE_QUADS,
      TestHost::PRIMITIVE_POLYGON,
  };
  static constexpr int kFirstTextRow = 3;
  static constexpr float z = 1.f;
  static constexpr float kWidth = 64.f;
  static constexpr float kHeight = 40.f;

  const float fb_width = host_.GetFramebufferWidthF();
  const float tri_left_cx = floorf(fb_width * 2.f / 5.f);
  const float quad_cx = floorf(fb_width * 3.f / 5.f);
  const float tri_right_cx = floorf(fb_width * 4.f / 5.f);

  auto bounds = [](float cx, float cy, float &left, float &right, float &top, float &bottom) {
    left = floorf(cx - kWidth / 2.f);
    right = floorf(left + kWidth);
    top = floorf(cy - kHeight / 2.f);
    bottom = floorf(cy + kHeight / 2.f);
  };

  auto draw_triangle = [&bounds](float cx, float cy) {
    float left, right, top, bottom;
    bounds(cx, cy, left, right, top, bottom);

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, left, bottom, z);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, left, top, z);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, right, top, z);
    Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
    Pushbuffer::End();
  };

  auto draw_quad = [&bounds](float cx, float cy) {
    float left, right, top, bottom;
    bounds(cx, cy, left, right, top, bottom);

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_QUADS);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, right, bottom, z);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, left, bottom, z);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, left, top, z);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, right, top, z);
    Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
    Pushbuffer::End();
  };

  auto draw_empty = [](TestHost::DrawPrimitive primitive) {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_BEGIN_END, primitive);
    Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
    Pushbuffer::End();
  };

  int text_row = kFirstTextRow;
  for (auto primitive : kEmptyPrimitives) {
    // Center the geometry on the label's pbkit text row (Y = 25 + 25 * row, 16px glyphs).
    const float cy = 33.f + 25.f * static_cast<float>(text_row);

    host_.SetDiffuse(1.0f, 0.0f, 0.0f);
    draw_triangle(tri_left_cx, cy);

    draw_empty(primitive);

    host_.SetDiffuse(0.0f, 1.0f, 0.0f);
    draw_quad(quad_cx, cy);

    draw_empty(primitive);

    host_.SetDiffuse(0.0f, 0.0f, 1.0f);
    draw_triangle(tri_right_cx, cy);

    pb_printat(text_row, 0, "%s", TestHost::GetDrawPrimitiveName(primitive).c_str());
    text_row += 2;
  }

  pb_draw_text_screen();

  FinishDraw(kTestEmptyBeginEnd);
}
