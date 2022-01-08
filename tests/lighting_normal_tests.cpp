#include "lighting_normal_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

struct TestParams {
  bool set_normal;
  float normal[3];
};

static constexpr TestParams kTests[] = {
    {false, {0.0f, 0.0f, 0.0f}},
    {true, {0.0f, 0.0f, 1.0f}},
    {true, {0.0f, 0.0f, -1.0f}},
    {true, {1.0f, 0.0f, 0.0f}},
    {true, {0.7071067811865475f, 0.0f, 0.7071067811865475f}},
    {true, {0.9486832980505138f, 0.0f, 0.31622776601683794f}},
    {true, {0.24253562503633297f, 0.0f, 0.9701425001453319f}},
};

LightingNormalTests::LightingNormalTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Lighting normals") {
  for (auto use_inline : {false, true}) {
    for (auto params : kTests) {
      std::string name = MakeTestName(params.set_normal, params.normal, use_inline);
      tests_[name] = [this, params, use_inline]() { this->Test(params.set_normal, params.normal, use_inline); };
    }
  }
}

void LightingNormalTests::Initialize() {
  TestSuite::Initialize();

  host_.SetShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void LightingNormalTests::Deinitialize() {
  normal_bleed_buffer_.reset();
  lit_buffer_.reset();
  TestSuite::Deinitialize();
}

void LightingNormalTests::CreateGeometry() {
  float left = -2.75f;
  float right = 2.75f;
  float top = 1.75f;
  float bottom = -1.75f;
  float mid_width = 0.0f;

  {
    normal_bleed_buffer_ = host_.AllocateVertexBuffer(3);
    Color one{0.0f, 1.0f, 0.0f, 0.5f};
    Color two{1.0f, 0.0f, 0.0f, 0.25f};
    Color three{0.4f, 0.4f, 1.0f, 1.0f};

    float z = 3.0f;
    float one_pos[3] = {left, top, z};
    float two_pos[3] = {left + (mid_width - left) * 0.5f, bottom, z};
    float three_pos[3] = {mid_width, top, z};
    normal_bleed_buffer_->DefineTriangle(0, one_pos, two_pos, three_pos, one, two, three);
  }

  {
    lit_buffer_ = host_.AllocateVertexBuffer(3);
    Color one{0.4f, 0.1f, 0.1f, 1.0f};
    Color two{1.0f, 1.0f, 0.4f, 1.0f};
    Color three{1.0f, 0.6f, 0.3f, 1.0f};

    float z = 1.0f;
    float one_pos[3] = {mid_width, top, z};
    float two_pos[3] = {mid_width + (right - mid_width) * 0.5f, bottom, z};
    float three_pos[3] = {right, top, z};
    lit_buffer_->DefineTriangle(0, one_pos, two_pos, three_pos, one, two, three);
  }
}

static void SetLightAndMaterial() {
  auto p = pb_begin();

  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS, 0xbf34dce5);
  p = pb_push1(p, 0x09e4, 0xc020743f);
  p = pb_push1(p, 0x09e8, 0x40333d06);
  p = pb_push1(p, 0x09ec, 0xbf003612);
  p = pb_push1(p, 0x09f0, 0xbff852a5);
  p = pb_push1(p, 0x09f4, 0x401c1bce);

  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  pb_push_to(SUBCH_3D, p++, NV097_SET_SCENE_AMBIENT_COLOR, 3);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;

  p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
  p = pb_push1(p, NV097_SET_MATERIAL_ALPHA, 0x3f800000);  // 1.0

  pb_push_to(SUBCH_3D, p++, NV097_SET_LIGHT_AMBIENT_COLOR, 3);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;

  pb_push_to(SUBCH_3D, p++, NV097_SET_LIGHT_DIFFUSE_COLOR, 3);
  *(p++) = 0x0;
  *(p++) = 0x3F800000;
  *(p++) = 0x3F333333;

  pb_push_to(SUBCH_3D, p++, NV097_SET_LIGHT_SPECULAR_COLOR, 3);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;

  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, 0x1);

  p = pb_push1(p, NV097_SET_LIGHT_LOCAL_RANGE, 0x7149f2ca);  // 1e30

  pb_push_to(SUBCH_3D, p++, NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 3);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  pb_push_to(SUBCH_3D, p++, NV097_SET_LIGHT_INFINITE_DIRECTION, 3);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x3F800000;  // 1.0f

  pb_end(p);
}

void LightingNormalTests::Test(bool set_normal, const float* normal, bool use_inline_buffer) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  uint32_t* p;

  SetLightAndMaterial();

  if (set_normal) {
    p = pb_begin();

    p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x10, 0);           // Specular
    p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x1C, 0xFFFFFFFF);  // Back diffuse
    p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x20, 0);           // Back specular

    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, 1);
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, 1);
    p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x20001);

    pb_end(p);

    Vertex* buf = normal_bleed_buffer_->Lock();
    memcpy(buf[2].normal, normal, sizeof(buf[2].normal));
    normal_bleed_buffer_->Unlock();

    host_.SetVertexBuffer(normal_bleed_buffer_);
    host_.DrawVertices(host_.POSITION | host_.NORMAL | host_.DIFFUSE);
  }

  // Render the test subject with no normals but lighting enabled.
  p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, 0x1);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, 0x1);
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);

  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x10, 0);           // Specular
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x1C, 0xFFFFFFFF);  // Back diffuse
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x20, 0);           // Back specular

  pb_end(p);

  host_.SetVertexBuffer(lit_buffer_);
  if (!use_inline_buffer) {
    host_.DrawVertices(host_.POSITION | host_.DIFFUSE);
  } else {
    host_.DrawVerticesAsInlineBuffer(host_.POSITION | host_.DIFFUSE);
  }

  if (!set_normal) {
    pb_print("No normal\n");
  } else {
    pb_print("Nx: %g\nNy: %g\nNz: %g\n", normal[0], normal[1], normal[2]);
  }
  if (use_inline_buffer) {
    pb_print("Inline buffer");
  }
  pb_draw_text_screen();

  std::string name = MakeTestName(set_normal, normal, use_inline_buffer);
  host_.FinishDrawAndSave(output_dir_, name);
}

std::string LightingNormalTests::MakeTestName(bool set_normal, const float* normal, bool inline_buffer) {
  if (!set_normal) {
    if (inline_buffer) {
      return "NoNormal-inlinebuf";
    }
    return "NoNormal";
  }

  char buf[128] = {0};
  snprintf(buf, 127, "Nz_%d%s", (int)(normal[2] * 100), inline_buffer ? "-inlinebuf" : "");
  return buf;
}
