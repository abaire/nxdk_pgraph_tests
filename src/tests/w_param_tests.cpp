#include "w_param_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

static constexpr const char kTestWGaps[] = "w_gaps";
static constexpr const char kTestWPositiveTriangleStrip[] = "w_pos_strip";
static constexpr const char kTestWNegativeTriangleStrip[] = "w_neg_strip";

WParamTests::WParamTests(TestHost& host, std::string output_dir) : TestSuite(host, std::move(output_dir), "W param") {
  tests_[kTestWGaps] = [this]() { this->TestWGaps(); };
  tests_[kTestWPositiveTriangleStrip] = [this]() { this->TestPositiveWTriangleStrip(); };
  tests_[kTestWNegativeTriangleStrip] = [this]() { this->TestNegativeWTriangleStrip(); };
}

void WParamTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PrecalculatedVertexShader>(false);
  host_.SetShaderProgram(shader);
}

void WParamTests::Deinitialize() {
  triangle_strip_.reset();
  triangles_.reset();
  TestSuite::Deinitialize();
}

void WParamTests::CreateGeometryWGaps() {
  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  const float left = floorf(fb_width / 5.0f);
  const float top = floorf(fb_height / 6.0f);
  const float bottom = top + (fb_height - top * 2.0f);
  const float right = left + (fb_width - left * 2.0f);
  float width = right - left;
  float mid_height = top + (bottom - top) * 0.5;

  triangle_strip_ = host_.AllocateVertexBuffer(40);

  triangle_strip_->SetPositionIncludesW();
  auto vertex = triangle_strip_->Lock();

  auto add_vertex = [&vertex](float x, float y, float w, float r, float g, float b) {
    vertex->SetPosition(x, y, 0.0, w);
    vertex->SetDiffuse(r, g, b);
    ++vertex;
  };

  float x = left;
  const float inc = width / 14.0f;

  auto add_visible_quad = [add_vertex, &x, mid_height, top, inc](float r, float g, float b) {
    add_vertex(x, mid_height, INFINITY, 0.75, 0.75, 0.75);
    add_vertex(x, top, INFINITY, 1.0, 1.0, 1.0);
    x += inc;
    add_vertex(x, mid_height, INFINITY, 0.5, 0.5, 0.5);
    add_vertex(x, top, INFINITY, r, g, b);
  };

  add_visible_quad(0.0f, 1.0f, 1.0f);

  // Invisible quad with 0 w.
  add_vertex(x, top, 0.0, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, mid_height, 0.0, 0.5, 0.0, 0.0);

  add_visible_quad(0.75f, 0.75f, 0.1f);

  // Invisible quad with 1 > w > 0.
  add_vertex(x, top, 0.9, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, mid_height, 0.0, 0.5, 0.0, 0.0);

  add_visible_quad(0.0f, 1.0f, 0.0f);

  // Invisible quad with w > 1.
  add_vertex(x, top, 10.9, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, mid_height, 0.0, 0.5, 0.0, 0.0);

  add_visible_quad(0.0f, 0.0f, 1.0f);

  // Invisible quad with 0 > w > -1.
  add_vertex(x, top, -0.9, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, mid_height, 0.0, 0.5, 0.0, 0.0);

  add_visible_quad(0.0f, 0.5, 1.0f);

  // Invisible quad with -1 > w.
  add_vertex(x, top, -10.9, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, mid_height, 10.0, 0.5, 0.0, 0.0);

  add_visible_quad(0.5f, 0.5f, 0.5f);

  // Invisible quad with INFINITE w.
  add_vertex(x, top, INFINITY, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, mid_height, INFINITY, 0.5, 0.0, 0.0);

  add_visible_quad(0.3f, 1.0f, 0.3f);

  triangle_strip_->Unlock();

  triangles_ = triangle_strip_->ConvertFromTriangleStripToTriangles();
  triangles_->Translate(0.0f, 2.0f + (bottom - top) * 0.5f, 0.0f, 0.0f);
}

void WParamTests::TestWGaps() {
  CreateGeometryWGaps();
  host_.PrepareDraw();

  host_.SetVertexBuffer(triangle_strip_);
  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE, TestHost::PRIMITIVE_TRIANGLE_STRIP);

  host_.SetVertexBuffer(triangles_);
  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE);

  pb_printat(5, 0, (char*)"Strip");
  pb_printat(12, 0, (char*)"Tris");
  pb_printat(1, 14, (char*)"0,0");
  pb_printat(15, 18, (char*)"0.9,0");
  pb_printat(1, 23, (char*)"10.9,0");
  pb_printat(15, 28, (char*)"-0.9,0");
  pb_printat(1, 33, (char*)"-10.9,10");
  pb_printat(15, 39, (char*)"inf,inf");
  pb_draw_text_screen();

  host_.FinishDrawAndSave(output_dir_, kTestWGaps);
}

void WParamTests::CreateGeometryPositiveWTriangleStrip() {
  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  const float left = floorf(fb_width / 5.0f);
  const float top = floorf(fb_height / 6.0f);
  const float bottom = top + (fb_height - top * 2.0f);
  const float right = left + (fb_width - left * 2.0f);
  float width = right - left;
  float mid_width = left + width * 0.5;

  triangle_strip_ = host_.AllocateVertexBuffer(6);

  triangle_strip_->SetPositionIncludesW();
  auto vertex = triangle_strip_->Lock();

  auto add_vertex = [&vertex](float x, float y, float w, float r, float g, float b) {
    vertex->SetPosition(x, y, 0.0, w);
    vertex->SetDiffuse(r, g, b);
    ++vertex;
  };

  add_vertex(left, bottom, 0.0f, 0.75f, 0.75f, 0.75f);
  add_vertex(left, top, INFINITY, 0.0f, 1.0f, 0.0f);
  add_vertex(mid_width, bottom, 0.1f, 1.0f, 0.0f, 0.0f);
  add_vertex(mid_width, top, 0.5, 0.0f, 0.0f, 1.0f);
  add_vertex(right, bottom, 1.5, 0.0f, 0.5f, 0.75f);
  add_vertex(right, top, 10.5, 0.7f, 0.9f, 0.2f);
}

void WParamTests::TestPositiveWTriangleStrip() {
  CreateGeometryPositiveWTriangleStrip();
  host_.SetVertexBuffer(triangle_strip_);
  host_.PrepareDraw();
  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE, TestHost::PRIMITIVE_TRIANGLE_STRIP);
  host_.FinishDrawAndSave(output_dir_, kTestWPositiveTriangleStrip);
}

void WParamTests::CreateGeometryNegativeWTriangleStrip() {
  CreateGeometryPositiveWTriangleStrip();

  Vertex* vertex = triangle_strip_->Lock();
  vertex[2].pos[3] = -0.01f;
  triangle_strip_->Unlock();
}

void WParamTests::TestNegativeWTriangleStrip() {
  CreateGeometryNegativeWTriangleStrip();
  host_.SetVertexBuffer(triangle_strip_);
  host_.PrepareDraw();
  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE, TestHost::PRIMITIVE_TRIANGLE_STRIP);
  host_.FinishDrawAndSave(output_dir_, kTestWNegativeTriangleStrip);
}
