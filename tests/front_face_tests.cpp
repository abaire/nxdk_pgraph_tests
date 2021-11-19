#include "front_face_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

static std::string WindingName(uint32_t winding);

static constexpr uint32_t kWindings[] = {
    NV097_SET_FRONT_FACE_V_CW, NV097_SET_FRONT_FACE_V_CCW,
    0,   // https://github.com/mborgerson/xemu/issues/321
    99,  // Random value to verify that HW behavior is to map all unknowns to CCW.
};

FrontFaceTests::FrontFaceTests(TestHost& host, std::string output_dir) : TestSuite(host, std::move(output_dir)) {
  for (auto winding : kWindings) {
    std::string name = MakeTestName(winding);

    auto test = [this, winding]() { this->Test(winding); };
    tests_[name] = test;
  }
}

void FrontFaceTests::Initialize() {
  // NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8
  const TextureFormatInfo& texture_format = kTextureFormats[3];
  host_.SetTextureFormat(texture_format);

  auto shader = std::make_shared<PrecalculatedVertexShader>(false);
  host_.SetShaderProgram(shader);

  CreateGeometry();
}

void FrontFaceTests::CreateGeometry() {
  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  float left = floorf(fb_width / 5.0f);
  float right = left + (fb_width - left * 2.0f);
  float top = floorf(fb_height / 12.0f);
  float bottom = top + (fb_height - top * 2.0f);
  float mid_width = left + (right - left) * 0.5f;

  uint32_t num_quads = 2;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);

  Color ul{1.0, 0.0, 0.0, 1.0};
  Color ll{0.0, 1.0, 0.0, 1.0};
  Color lr{0.0, 0.0, 1.0, 1.0};
  Color ur{0.5, 0.5, 0.5, 1.0};

  uint32_t idx = 0;
  buffer->DefineQuad(0, left + 10, top + 4, mid_width - 10, bottom - 10, 10.0f, 10.0f, 10.0f, 10.0f, ul, ll, lr, ur);
  buffer->DefineQuadCW(1, mid_width + 10, top + 4, right - 10, bottom - 10, 10.0f, 10.0f, 10.0f, 10.0f, ul, ll, lr, ur);
}

void FrontFaceTests::Test(uint32_t front_face) {
  host_.SetDepthBufferFormat(NV097_SET_SURFACE_FORMAT_ZETA_Z16);
  host_.SetDepthBufferFloatMode(false);
  host_.PrepareDraw();
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_FRONT_FACE, front_face);
  pb_end(p);
  host_.DrawVertices();

  std::string winding_name = WindingName(front_face);
  pb_print("FF: %s\n", winding_name.c_str());
  pb_draw_text_screen();

  std::string name = MakeTestName(front_face);
  host_.FinishDrawAndSave(output_dir_.c_str(), name.c_str());
}

std::string FrontFaceTests::MakeTestName(uint32_t front_face) {
  std::string ret = "FrontFace_";
  ret += WindingName(front_face);
  return std::move(ret);
}

static std::string WindingName(uint32_t winding) {
  if (winding == NV097_SET_FRONT_FACE_V_CW) {
    return "CW";
  } else if (winding == NV097_SET_FRONT_FACE_V_CCW) {
    return "CCW";
  }

  char buf[16] = {0};
  snprintf(buf, 15, "%X", winding);
  return buf;
}