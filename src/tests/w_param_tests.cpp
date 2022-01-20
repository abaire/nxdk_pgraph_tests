#include "w_param_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

static constexpr const char kTestName[] = "inf_pos_neg_w";

WParamTests::WParamTests(TestHost& host, std::string output_dir) : TestSuite(host, std::move(output_dir), "W param") {
  auto test = [this]() { this->Test(); };
  tests_[kTestName] = test;
}

void WParamTests::Initialize() {
  TestSuite::Initialize();

  {
    auto p = pb_begin();
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_CULL_FACE_ENABLE, true);
    pb_end(p);
  }

  auto shader = std::make_shared<PrecalculatedVertexShader>(false);
  host_.SetShaderProgram(shader);

  CreateGeometry();
}

void WParamTests::CreateGeometry() {
  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  const float left = floorf(fb_width / 5.0f);
  const float top = floorf(fb_height / 6.0f);
  const float bottom = top + (fb_height - top * 2.0f);
  const float right = left + (fb_width - left * 2.0f);
  float width = right - left;

  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(40);

  buffer->SetPositionIncludesW();
  auto vertex = buffer->Lock();

  auto add_vertex = [&vertex](float x, float y, float w, float r, float g, float b) {
    vertex->SetPosition(x, y, 0.0, w);
    vertex->SetDiffuse(r, g, b);
    ++vertex;
  };

  float x = left;
  const float inc = width / 14.0f;

  auto add_visible_quad = [add_vertex, &x, bottom, top, inc](float r, float g, float b) {
    add_vertex(x, bottom, INFINITY, 0.75, 0.75, 0.75);
    add_vertex(x, top, INFINITY, 1.0, 1.0, 1.0);
    x += inc;
    add_vertex(x, bottom, INFINITY, 0.5, 0.5, 0.5);
    add_vertex(x, top, INFINITY, r, g, b);
  };

  add_visible_quad(0.0f, 1.0f, 1.0f);

  // Invisible quad with 0 w.
  add_vertex(x, top, 0.0, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, bottom, 0.0, 0.5, 0.0, 0.0);

  add_visible_quad(0.75f, 0.75f, 0.1f);

  // Invisible quad with 1 > w > 0.
  add_vertex(x, top, 0.9, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, bottom, 0.0, 0.5, 0.0, 0.0);

  add_visible_quad(0.0f, 1.0f, 0.0f);

  // Invisible quad with w > 1.
  add_vertex(x, top, 10.9, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, bottom, 0.0, 0.5, 0.0, 0.0);

  add_visible_quad(0.0f, 0.0f, 1.0f);

  // Invisible quad with 0 > w > -1.
  add_vertex(x, top, -0.9, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, bottom, 0.0, 0.5, 0.0, 0.0);

  add_visible_quad(0.0f, 0.5, 1.0f);

  // Invisible quad with -1 > w.
  add_vertex(x, top, -10.9, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, bottom, 10.0, 0.5, 0.0, 0.0);

  add_visible_quad(0.5f, 0.5f, 0.5f);

  // Invisible quad with INFINITE w.
  add_vertex(x, top, INFINITY, 1.0, 0.0, 0.0);
  x += inc;
  add_vertex(x, bottom, INFINITY, 0.5, 0.0, 0.0);

  add_visible_quad(0.3f, 1.0f, 0.3f);
}

void WParamTests::Test() {
  host_.PrepareDraw();

  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE, TestHost::PRIMITIVE_TRIANGLE_STRIP);

  pb_printat(1, 14, (char*)"0,0");
  pb_printat(15, 18, (char*)"0.9,0");
  pb_printat(1, 23, (char*)"10.9,0");
  pb_printat(15, 28, (char*)"-0.9,0");
  pb_printat(1, 33, (char*)"-10.9,10");
  pb_printat(15, 39, (char*)"inf,inf");
  pb_draw_text_screen();

  host_.FinishDrawAndSave(output_dir_, kTestName);
}
