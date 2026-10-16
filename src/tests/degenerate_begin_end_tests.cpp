#include "degenerate_begin_end_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "vertex_buffer.h"

static const char kTestBeginWithoutEnd[] = "BeginWithoutEnd";
static const char kTestEmptyBeginEnd[] = "EmptyBeginEnd";
static const char kTestEndWithoutBegin[] = "EndWithoutBegin";

static constexpr TestHost::DrawPrimitive kEmptyPrimitives[] = {
    TestHost::PRIMITIVE_POINTS,    TestHost::PRIMITIVE_LINES,          TestHost::PRIMITIVE_LINE_STRIP,
    TestHost::PRIMITIVE_TRIANGLES, TestHost::PRIMITIVE_TRIANGLE_STRIP, TestHost::PRIMITIVE_QUADS,
    TestHost::PRIMITIVE_POLYGON,
};

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
 *
 * @tc EndWithoutBegin
 *   Demonstrates that a degenerate END without an associated BEGIN is silently ignored by HW. Each subtest renders a
 *   red triangle, then performs an empty BEGIN(primitive), END followed by the vertices for the primitive, followed by
 *   another END in green. Finally a standard BEGIN(primitive) - vertices - END is performed in blue. The green draw is
 *   never rendered, since hardware was never told to begin the draw.
 */
DegenerateBeginEndTests::DegenerateBeginEndTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Degenerate begin end", config) {
  tests_[kTestBeginWithoutEnd] = [this]() { TestBeginWithoutEnd(); };
  tests_[kTestEmptyBeginEnd] = [this]() { TestEmptyBeginEnd(); };
  tests_[kTestEndWithoutBegin] = [this]() { TestEndWithoutBegin(); };
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

void DegenerateBeginEndTests::TestEndWithoutBegin() {
  host_.PrepareDraw(0xFE242424);

  pb_print("%s\n", kTestEndWithoutBegin);
  pb_print("red tri, green no-BEGIN prim, blue reference\n");

  static constexpr TestHost::DrawPrimitive kPrimitives[] = {
      TestHost::PRIMITIVE_POINTS,       TestHost::PRIMITIVE_LINES,     TestHost::PRIMITIVE_LINE_LOOP,
      TestHost::PRIMITIVE_LINE_STRIP,   TestHost::PRIMITIVE_TRIANGLES, TestHost::PRIMITIVE_TRIANGLE_STRIP,
      TestHost::PRIMITIVE_TRIANGLE_FAN, TestHost::PRIMITIVE_QUADS,     TestHost::PRIMITIVE_QUAD_STRIP,
      TestHost::PRIMITIVE_POLYGON,
  };
  static constexpr int kRowsPerColumn = 5;
  static constexpr int kFirstTextRow = 3;
  static constexpr int kTextRowStride = 3;
  static constexpr int kLabelColumns[] = {0, 32};
  static constexpr float kShapeCenterX[][3] = {{154.f, 218.f, 282.f}, {464.f, 528.f, 592.f}};
  static constexpr float z = 1.f;
  static constexpr float kSize = 52.f;
  static constexpr float kPointSpacing = 3.f;

  // Emits a vertex at normalized (u, v) within a kSize box centered at (cx, cy), with v increasing downward.
  auto vertex = [](float cx, float cy, float u, float v) {
    const float left = floorf(cx - kSize / 2.f);
    const float top = floorf(cy - kSize / 2.f);
    Pushbuffer::PushF(NV097_SET_VERTEX3F, floorf(left + u * kSize), floorf(top + v * kSize), z);
  };

  auto push_vertices = [&vertex](TestHost::DrawPrimitive primitive, float cx, float cy) {
    auto v = [&vertex, cx, cy](float u, float v) { vertex(cx, cy, u, v); };

    switch (primitive) {
      case TestHost::PRIMITIVE_POINTS: {
        // Diamond of 1 pixel points.
        const float step = kPointSpacing / (kSize * 2.f);
        for (float y = 0.f; y <= 1.f; y += step) {
          for (float x = 0.f; x <= 1.f; x += step) {
            if (fabsf(x - 0.5f) + fabsf(y - 0.5f) <= 0.5f) {
              v(x, y);
            }
          }
        }
      } break;

      case TestHost::PRIMITIVE_LINES:
        // Three disjoint segments.
        v(0.f, 0.f);
        v(1.f, 0.f);
        v(0.f, 0.3f);
        v(0.45f, 1.f);
        v(0.65f, 0.3f);
        v(1.f, 1.f);
        break;

      case TestHost::PRIMITIVE_LINE_LOOP:
        // Pentagram; the closing edge is the lower left stroke.
        v(0.f, 1.f);
        v(0.5f, 0.f);
        v(1.f, 1.f);
        v(0.f, 0.35f);
        v(1.f, 0.35f);
        break;

      case TestHost::PRIMITIVE_LINE_STRIP:
        // Open "W".
        v(0.f, 0.f);
        v(0.25f, 1.f);
        v(0.5f, 0.3f);
        v(0.75f, 1.f);
        v(1.f, 0.f);
        break;

      case TestHost::PRIMITIVE_TRIANGLES:
        // Two disjoint triangles in opposite corners.
        v(0.f, 0.6f);
        v(0.f, 0.f);
        v(0.6f, 0.f);
        v(1.f, 0.4f);
        v(1.f, 1.f);
        v(0.4f, 1.f);
        break;

      case TestHost::PRIMITIVE_TRIANGLE_STRIP:
        // "V" shaped band that is thin on the left and thick on the right.
        v(0.f, 0.f);
        v(0.f, 0.2f);
        v(0.5f, 0.45f);
        v(0.5f, 1.f);
        v(1.f, 0.f);
        v(1.f, 0.7f);
        break;

      case TestHost::PRIMITIVE_TRIANGLE_FAN:
        // Quarter disc pivoting around the bottom left corner.
        v(0.f, 1.f);
        v(0.f, 0.f);
        v(0.4f, 0.08f);
        v(0.72f, 0.28f);
        v(0.92f, 0.6f);
        v(1.f, 1.f);
        break;

      case TestHost::PRIMITIVE_QUADS:
        // Two disjoint, irregular quads.
        v(0.f, 0.f);
        v(0.55f, 0.f);
        v(0.4f, 0.4f);
        v(0.05f, 0.55f);
        v(0.55f, 0.6f);
        v(1.f, 0.45f);
        v(0.95f, 1.f);
        v(0.6f, 0.9f);
        break;

      case TestHost::PRIMITIVE_QUAD_STRIP:
        // Band that tapers from full height on the left to a sliver on the right, with a kinked top edge.
        v(0.f, 0.f);
        v(0.f, 1.f);
        v(0.35f, 0.35f);
        v(0.35f, 0.75f);
        v(0.7f, 0.15f);
        v(0.7f, 0.6f);
        v(1.f, 0.45f);
        v(1.f, 0.55f);
        break;

      case TestHost::PRIMITIVE_POLYGON:
        // Irregular convex hexagon.
        v(0.3f, 0.f);
        v(0.85f, 0.1f);
        v(1.f, 0.55f);
        v(0.7f, 1.f);
        v(0.1f, 0.85f);
        v(0.f, 0.35f);
        break;

      default:
        ASSERT(!"Unsupported primitive");
    }
  };

  // Several primitives contain triangles of both windings.
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CULL_FACE_ENABLE, false);
  Pushbuffer::End();

  int index = 0;
  for (auto primitive : kPrimitives) {
    const int column = index / kRowsPerColumn;
    const int text_row = kFirstTextRow + (index % kRowsPerColumn) * kTextRowStride;
    ++index;

    // Center the geometry on the label's pbkit text row (Y = 25 + 25 * row, 16px glyphs).
    const float cy = 33.f + 25.f * static_cast<float>(text_row);
    const float *cx = kShapeCenterX[column];

    host_.SetDiffuse(1.0f, 0.0f, 0.0f);
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_BEGIN_END, TestHost::PRIMITIVE_TRIANGLES);
    vertex(cx[0], cy, 0.f, 1.f);
    vertex(cx[0], cy, 0.f, 0.f);
    vertex(cx[0], cy, 1.f, 0.f);
    Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
    Pushbuffer::End();

    host_.SetDiffuse(0.0f, 1.0f, 0.0f);
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_BEGIN_END, primitive);
    Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
    push_vertices(primitive, cx[1], cy);
    Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
    Pushbuffer::End();

    host_.SetDiffuse(0.0f, 0.0f, 1.0f);
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_BEGIN_END, primitive);
    push_vertices(primitive, cx[2], cy);
    Pushbuffer::Push(NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
    Pushbuffer::End();

    pb_printat(text_row, kLabelColumns[column], "%s", TestHost::GetDrawPrimitiveName(primitive).c_str());
  }

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CULL_FACE_ENABLE, true);
  Pushbuffer::End();

  pb_draw_text_screen();

  FinishDraw(kTestEndWithoutBegin);
}
