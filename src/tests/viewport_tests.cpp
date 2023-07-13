#include "viewport_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/perspective_vertex_shader.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"

static constexpr ViewportTests::Viewport kTestCases[] = {
    {{0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0.531250f, 0.531250f, 0, 0}, {0, 0, 0, 0}},
    {{0.5625f, 0.5625f, 0, 0}, {0, 0, 0, 0}},
    {{-0.531250f, -0.531250f, 0, 0}, {0, 0, 0, 0}},
    {{-0.4375f, -0.4375f, 0, 0}, {0, 0, 0, 0}},
    {{1.0, 1.0, 0, 0}, {0, 0, 0, 0}},
    {{-1.0, -1.0, 0, 0}, {0, 0, 0, 0}},

    {{0, 0, 0, 0}, {2.0f, 0, 0, 0}},
    {{0.531250f, 0.531250f, 0, 0}, {2.0f, 0, 0, 0}},
    {{-0.531250f, -0.531250f, 0, 0}, {2.0f, 0, 0, 0}},
    {{1.0, 1.0, 0, 0}, {2.0f, 0, 0, 0}},
    {{-1.0, -1.0, 0, 0}, {2.0f, 0, 0, 0}},
};

static std::string MakeTestName(const ViewportTests::Viewport &vp) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.03f_%.03f-%.03f_%.03f", vp.offset[0], vp.offset[1], vp.scale[0], vp.scale[1]);
  return buffer;
}

ViewportTests::ViewportTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Viewport") {
  for (auto &vp : kTestCases) {
    tests_[MakeTestName(vp)] = [this, &vp]() { Test(vp); };
  }
}

void ViewportTests::Test(const Viewport &vp) {
  std::string name = MakeTestName(vp);

  host_.PrepareDraw(0xFE111213);

  float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                          0.0f, depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  {
    shader->SetLightingEnabled(false);
    shader->SetUse4ComponentTexcoords();
    shader->SetUseD3DStyleViewport();
    vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
    vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);
  }
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetViewportOffset(vp.offset[0], vp.offset[1], vp.offset[2], vp.offset[3]);
  host_.SetViewportScale(vp.scale[0], vp.scale[1], vp.scale[2], vp.scale[3]);

  host_.SetVertexShaderProgram(shader);

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.PrepareDraw(0xFE404040);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  static constexpr float kQuadSize = 100.0f;

  const float kMidX = floorf(host_.GetFramebufferWidthF() * 0.5f);
  const float kMidY = floor(host_.GetFramebufferHeightF() * 0.5f);
  const float kRowStart = kMidX - (kQuadSize * 2);
  float left = kRowStart;
  float top = kMidY - kQuadSize;
  const float kZBottom = 0.0f;
  const float kZTop = 10.0f;
  const float kBackgroundZBottom = kZBottom + 0.0001f;
  const float kBackgroundZTop = kZTop + 0.0001f;

  auto set_vertex = [this](float x, float y, float z) {
    vector_t world;
    vector_t screen_point = {x, y, z, 1.0f};
    host_.UnprojectPoint(world, screen_point, z);
    host_.SetVertex(world[0], world[1], z, 1.0f);
  };

  // Draw a background.
  uint32_t color = 0xFFE0E0E0;
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  set_vertex(left, top, kBackgroundZTop);
  set_vertex(left + kQuadSize * 4.0f, top, kBackgroundZTop);
  set_vertex(left + kQuadSize * 4.0f, top + kQuadSize * 2.0f, kBackgroundZBottom);
  set_vertex(left, top + kQuadSize * 2.0f, kBackgroundZBottom);
  host_.End();

  float z_inc = kZTop - kZBottom * 0.5f;
  float bottom_z = kZBottom;

  struct Quad {
    vector_t ul;
    vector_t ur;
    vector_t lr;
    vector_t ll;
  };

  Quad quads[8];
  for (uint32_t y = 0, i = 0; y < 2; ++y) {
    float bottom = top + kQuadSize;
    float top_z = bottom_z + z_inc;

    for (uint32_t x = 0; x < 4; ++x, ++i) {
      float right = left + kQuadSize;

      quads[i].ul[0] = left;
      quads[i].ul[1] = top;
      quads[i].ul[2] = top_z;

      quads[i].ur[0] = right;
      quads[i].ur[1] = top;
      quads[i].ur[2] = top_z;

      quads[i].lr[0] = right;
      quads[i].lr[1] = bottom;
      quads[i].lr[2] = bottom_z;

      quads[i].ll[0] = left;
      quads[i].ll[1] = bottom;
      quads[i].ll[2] = bottom_z;

      left += kQuadSize;
    }
    left = kRowStart;
    bottom_z = kZBottom;
    top = bottom;
  }

  // Draw squares using the programmable pipeline.
  color = 0xFF001199;
  for (uint32_t i = 0; i < 4; i += 2) {
    auto &q = quads[i];
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(color);
    set_vertex(q.ul[0], q.ul[1], q.ul[2]);
    set_vertex(q.ur[0], q.ur[1], q.ur[2]);
    set_vertex(q.lr[0], q.lr[1], q.lr[2]);
    set_vertex(q.ll[0], q.ll[1], q.ll[2]);
    host_.End();
  }
  for (uint32_t i = 5; i < 8; i += 2) {
    auto &q = quads[i];
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(color);
    set_vertex(q.ul[0], q.ul[1], q.ul[2]);
    set_vertex(q.ur[0], q.ur[1], q.ur[2]);
    set_vertex(q.lr[0], q.lr[1], q.lr[2]);
    set_vertex(q.ll[0], q.ll[1], q.ll[2]);
    host_.End();
  }

  // Draw squares using the fixed pipeline.
  color = 0xFF0033BB;
  host_.SetVertexShaderProgram(nullptr);
  for (uint32_t i = 1; i < 4; i += 2) {
    auto &q = quads[i];
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(color);
    set_vertex(q.ul[0], q.ul[1], q.ul[2]);
    set_vertex(q.ur[0], q.ur[1], q.ur[2]);
    set_vertex(q.lr[0], q.lr[1], q.lr[2]);
    set_vertex(q.ll[0], q.ll[1], q.ll[2]);
    host_.End();
  }
  for (uint32_t i = 4; i < 8; i += 2) {
    auto &q = quads[i];
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(color);
    set_vertex(q.ul[0], q.ul[1], q.ul[2]);
    set_vertex(q.ur[0], q.ur[1], q.ur[2]);
    set_vertex(q.lr[0], q.lr[1], q.lr[2]);
    set_vertex(q.ll[0], q.ll[1], q.ll[2]);
    host_.End();
  }

  pb_printat(0, 25, (char *)"%s", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}
