#include "front_face_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

static std::string WindingName(uint32_t winding);
static std::string CullFaceName(uint32_t cull_face);

static constexpr uint32_t kWindings[] = {
    NV097_SET_FRONT_FACE_V_CW, NV097_SET_FRONT_FACE_V_CCW,
    0,   // https://github.com/mborgerson/xemu/issues/321
    99,  // Random value to verify that HW behavior is to map all unknowns to CCW.
};

static constexpr uint32_t kCullFaces[] = {
    NV097_SET_CULL_FACE_V_FRONT,
    NV097_SET_CULL_FACE_V_BACK,
    NV097_SET_CULL_FACE_V_FRONT_AND_BACK,
};

FrontFaceTests::FrontFaceTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Front face") {
  for (auto winding : kWindings) {
    for (auto cull_face : kCullFaces) {
      std::string name = MakeTestName(winding, cull_face);

      auto test = [this, winding, cull_face]() { this->Test(winding, cull_face); };
      tests_[name] = test;
    }
  }
}

void FrontFaceTests::Initialize() {
  TestSuite::Initialize();

  {
    auto p = pb_begin();
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_CULL_FACE_ENABLE, true);
    pb_end(p);
  }

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

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
  buffer->SetPositionIncludesW();

  Color ul{1.0, 0.0, 0.0, 1.0};
  Color ll{0.0, 1.0, 0.0, 1.0};
  Color lr{0.0, 0.0, 1.0, 1.0};
  Color ur{0.5, 0.5, 0.5, 1.0};

  float z = 10.0f;
  buffer->DefineBiTriCCW(0, left + 10, top + 4, mid_width - 10, bottom - 10, z, z, z, z, ul, ll, lr, ur);

  buffer->SetPositionIncludesW();
  auto vtx = buffer->Lock();
  vtx[0].pos[3] = INFINITY;
  vtx[1].pos[3] = 0.980578f;
  vtx[2].pos[3] = 0.0f;
  vtx[3].pos[3] = INFINITY;
  vtx[4].pos[3] = INFINITY;
  vtx[5].pos[3] = INFINITY;
  buffer->Unlock();

  Color tmp = ul;
  ul = lr;
  lr = tmp;

  tmp = ur;
  ur = ll;
  ll = tmp;

  buffer->DefineBiTri(1, mid_width + 10, top + 4, right - 10, bottom - 10, z, z, z, z, ul, ll, lr, ur);
}

void FrontFaceTests::Test(uint32_t front_face, uint32_t cull_face) {
  host_.PrepareDraw();

  // To verify that the HW is simply preserving a previously set value, force it to a known valid, but different value
  // before setting the value under test.
  // Note that the setup steps done by host_ will set the front face to CCW, so CW is preferred to differentiate the
  // behavior from simply running tests in sequence.
  if (front_face != NV097_SET_FRONT_FACE_V_CW) {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_FRONT_FACE, NV097_SET_FRONT_FACE_V_CW);
    pb_end(p);
  } else {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_FRONT_FACE, NV097_SET_FRONT_FACE_V_CCW);
    pb_end(p);
  }

  while (pb_busy()) {
  }

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_FRONT_FACE, front_face);
  p = pb_push1(p, NV097_SET_CULL_FACE, cull_face);
  pb_end(p);
  host_.DrawArrays();

  std::string winding_name = WindingName(front_face);
  pb_print("FF: %s\n", winding_name.c_str());
  std::string cull_face_name = CullFaceName(cull_face);
  pb_print("CF: %s\n", cull_face_name.c_str());
  pb_printat(8, 19, (char*)"CCW");
  pb_printat(8, 38, (char*)"CW");
  pb_draw_text_screen();

  std::string name = MakeTestName(front_face, cull_face);
  host_.FinishDraw(allow_saving_, output_dir_, name);
}

std::string FrontFaceTests::MakeTestName(uint32_t front_face, uint32_t cull_face) {
  std::string ret = "FrontFace_";
  ret += WindingName(front_face);
  ret += "_CF_";
  ret += CullFaceName(cull_face);
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

static std::string CullFaceName(uint32_t cull_face) {
  if (cull_face == NV097_SET_CULL_FACE_V_FRONT) {
    return "F";
  } else if (cull_face == NV097_SET_CULL_FACE_V_BACK) {
    return "B";
  } else if (cull_face == NV097_SET_CULL_FACE_V_FRONT_AND_BACK) {
    return "FaB";
  }

  char buf[16] = {0};
  snprintf(buf, 15, "%X", cull_face);
  return buf;
}
