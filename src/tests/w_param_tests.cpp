#include "w_param_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "texture_generator.h"
#include "vertex_buffer.h"

static constexpr char kTexturePerspectiveEnabledSuffix[] = "_tex_persp";
static constexpr char kTestWGaps[] = "w_gaps";
static constexpr char kTestWPositiveTriangleStrip[] = "w_pos_strip";
static constexpr char kTestWNegativeTriangleStrip[] = "w_neg_strip";
static constexpr char kTestFixedFunctionZeroW[] = "ff_w_zero_";

static std::string AugmentTestName(const std::string &test_name, bool texture_perspective_enable) {
  return test_name + (texture_perspective_enable ? kTexturePerspectiveEnabledSuffix : "");
}
static std::string MakeFFZeroWTestName(bool draw_quad, bool texture_perspective_enable) {
  return std::string(kTestFixedFunctionZeroW) + (draw_quad ? "_quad" : "_bitri") +
         (texture_perspective_enable ? kTexturePerspectiveEnabledSuffix : "");
}

WParamTests::WParamTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "W param", config) {
  for (auto texture_perspective_enable : {false, true}) {
    auto fullname = [texture_perspective_enable](const std::string &test_name) {
      return AugmentTestName(test_name, texture_perspective_enable);
    };

    tests_[fullname(kTestWGaps)] = [this, texture_perspective_enable]() {
      this->TestWGaps(texture_perspective_enable);
    };
    tests_[fullname(kTestWPositiveTriangleStrip)] = [this, texture_perspective_enable]() {
      this->TestPositiveWTriangleStrip(texture_perspective_enable);
    };
    tests_[fullname(kTestWNegativeTriangleStrip)] = [this, texture_perspective_enable]() {
      this->TestNegativeWTriangleStrip(texture_perspective_enable);
    };

    for (auto quad : {false, true}) {
      tests_[MakeFFZeroWTestName(quad, texture_perspective_enable)] = [this, quad, texture_perspective_enable]() {
        this->TestFixedFunctionZeroW(quad, texture_perspective_enable);
      };
    }
  }
}

void WParamTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
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
  float mid_height = top + (bottom - top) * 0.5f - 4;

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
  triangles_->Translate(-6.0f, 6.0f + (bottom - top) * 0.5f, 0.0f, 0.0f);
}

void WParamTests::TestWGaps(bool texture_perspective_enable) {
  CreateGeometryWGaps();
  host_.PrepareDraw();

  if (!texture_perspective_enable) {
    host_.SetupControl0(true, false, false);
  }

  host_.SetVertexBuffer(triangle_strip_);
  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE, TestHost::PRIMITIVE_TRIANGLE_STRIP);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_LINE);
  // Note: This shouldn't strictly be necessary, but at the moment xemu disallows different fill modes for front and
  // back.
  Pushbuffer::Push(NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_LINE);
  Pushbuffer::Push(NV097_SET_DIFFUSE_COLOR4I, 0xFFFFFFFF);
  Pushbuffer::End();

  host_.DrawArrays(TestHost::POSITION, TestHost::PRIMITIVE_TRIANGLE_STRIP);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
  Pushbuffer::Push(NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
  Pushbuffer::End();

  host_.SetVertexBuffer(triangles_);
  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_LINE);
  // Note: This shouldn't strictly be necessary, but at the moment xemu disallows different fill modes for front and
  // back.
  Pushbuffer::Push(NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_LINE);
  Pushbuffer::Push(NV097_SET_DIFFUSE_COLOR4I, 0xFFFFFFFF);
  Pushbuffer::End();

  host_.DrawArrays(TestHost::POSITION);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
  Pushbuffer::Push(NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
  Pushbuffer::End();

  pb_printat(5, 0, (char *)"Strip");
  pb_printat(12, 0, (char *)"Tris");
  pb_printat(1, 14, (char *)"0,0");
  pb_printat(15, 18, (char *)"0.9,0");
  pb_printat(1, 23, (char *)"10.9,0");
  pb_printat(15, 28, (char *)"-0.9,0");
  pb_printat(1, 33, (char *)"-10.9,10");
  pb_printat(15, 39, (char *)"inf,inf");
  pb_draw_text_screen();

  host_.SetupControl0();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, AugmentTestName(kTestWGaps, texture_perspective_enable));
}

void WParamTests::CreateGeometryPositiveWTriangleStrip() {
  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  const float left = floorf(fb_width / 5.0f);
  const float top = floorf(fb_height / 6.0f);
  const float bottom = top + (fb_height - top * 2.0f);
  const float right = left + (fb_width - left * 2.0f);
  float width = right - left;
  float mid_width = left + width * 0.5f;

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

void WParamTests::TestPositiveWTriangleStrip(bool texture_perspective_enable) {
  CreateGeometryPositiveWTriangleStrip();
  host_.SetVertexBuffer(triangle_strip_);
  host_.PrepareDraw();
  if (!texture_perspective_enable) {
    host_.SetupControl0(true, false, false);
  }

  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE, TestHost::PRIMITIVE_TRIANGLE_STRIP);

  host_.SetupControl0();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_,
                   AugmentTestName(kTestWPositiveTriangleStrip, texture_perspective_enable));
}

void WParamTests::CreateGeometryNegativeWTriangleStrip() {
  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  const float left = floorf(fb_width / 5.0f);
  const float top = floorf(fb_height / 6.0f);
  const float bottom = top + (fb_height - top * 2.0f);
  const float right = left + (fb_width - left * 2.0f);
  float width = right - left;
  float mid_width = left + width * 0.5f;

  triangle_strip_ = host_.AllocateVertexBuffer(6);

  triangle_strip_->SetPositionIncludesW();
  auto vertex = triangle_strip_->Lock();

  auto add_vertex = [&vertex](float x, float y, float w, float r, float g, float b) {
    vertex->SetPosition(x, y, 0.0, w);
    vertex->SetDiffuse(r, g, b);
    ++vertex;
  };

  const float w = 1.0f;
  add_vertex(left, bottom, w, 0.75, 0.75, 0.75);
  add_vertex(left, top, w, 1.0, 1.0, 1.0);
  add_vertex(mid_width, bottom, w, 0.5, 0.5, 0.5);
  add_vertex(mid_width, top, w, 0.0f, 1.0f, 0.0f);

  add_vertex(mid_width, top, -0.1, 1.0, 0.0, 0.0);
  add_vertex(right, bottom, w, 0.0, 0.5, 0.5);

  triangle_strip_->Unlock();
}

void WParamTests::TestNegativeWTriangleStrip(bool texture_perspective_enable) {
  CreateGeometryNegativeWTriangleStrip();
  host_.SetVertexBuffer(triangle_strip_);
  host_.PrepareDraw();

  if (!texture_perspective_enable) {
    host_.SetupControl0(true, false, false);
  }

  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE, TestHost::PRIMITIVE_TRIANGLE_STRIP);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_LINE);
  // Note: This shouldn't strictly be necessary, but at the moment xemu disallows different fill modes for front and
  // back.
  Pushbuffer::Push(NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_LINE);
  Pushbuffer::Push(NV097_SET_DIFFUSE_COLOR4I, 0xFFFFFFFF);
  Pushbuffer::End();

  host_.DrawArrays(TestHost::POSITION, TestHost::PRIMITIVE_TRIANGLE_STRIP);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
  Pushbuffer::Push(NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
  Pushbuffer::End();

  host_.SetupControl0();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_,
                   AugmentTestName(kTestWNegativeTriangleStrip, texture_perspective_enable));
}

void WParamTests::TestFixedFunctionZeroW(bool draw_quad, bool texture_perspective_enable) {
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.PrepareDraw(0xFE251135);

  // Generate a distinct texture.
  constexpr uint32_t kTextureSize = 256;
  {
    memset(host_.GetTextureMemory(), 0, host_.GetTextureMemorySize());
    GenerateRGBACheckerboard(host_.GetTextureMemory(), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
  }

  if (!texture_perspective_enable) {
    host_.SetupControl0(true, false, false);
  }

  {
    const float left = -1.75f;
    const float top = 1.75f;
    const float right = 1.75f;
    const float bottom = -1.75f;
    const auto depth = 0.0f;

    if (draw_quad) {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord0(0.0f, 0.0f);
      host_.SetVertex(left, top, depth, 0.0f);

      host_.SetTexCoord0(kTextureSize, 0.0f);
      host_.SetVertex(right, top, depth, 1.0f);

      host_.SetTexCoord0(kTextureSize, kTextureSize);
      host_.SetVertex(right, bottom, depth, 1.0f);

      host_.SetTexCoord0(0.0f, kTextureSize);
      host_.SetVertex(left, bottom, depth, 1.0f);
      host_.End();
    } else {
      host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
      host_.SetTexCoord0(0.0f, 0.0f);
      host_.SetVertex(left, top, depth, 0.0f);

      host_.SetTexCoord0(kTextureSize, 0.0f);
      host_.SetVertex(right, top, depth, 1.0f);

      host_.SetTexCoord0(0.0f, kTextureSize);
      host_.SetVertex(left, bottom, depth, 1.0f);

      host_.SetTexCoord0(kTextureSize, kTextureSize);
      host_.SetVertex(right, bottom, depth, 1.0f);

      host_.SetTexCoord0(0.0f, kTextureSize);
      host_.SetVertex(left, bottom, depth, 1.0f);

      host_.SetTexCoord0(kTextureSize, 0.0f);
      host_.SetVertex(right, top, depth, 1.0f);

      host_.End();
    }
  }

  host_.SetupControl0();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, MakeFFZeroWTestName(draw_quad, texture_perspective_enable));

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.SetDefaultViewportAndFixedFunctionMatrices();

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
}
