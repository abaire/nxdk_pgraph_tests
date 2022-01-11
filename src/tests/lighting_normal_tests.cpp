#include "lighting_normal_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

struct MaterialParams {
  bool set_normal;
  float normal[3];
};

static constexpr MaterialParams kTests[] = {
    {false, {0.0f, 0.0f, 0.0f}},
    {true, {0.0f, 0.0f, 1.0f}},
    {true, {0.0f, 0.0f, -1.0f}},
    {true, {1.0f, 0.0f, 0.0f}},
    {true, {0.7071067811865475f, 0.0f, 0.7071067811865475f}},
    {true, {0.9486832980505138f, 0.0f, 0.31622776601683794f}},
    {true, {0.24253562503633297f, 0.0f, 0.9701425001453319f}},
};

static constexpr LightingNormalTests::DrawMode kDrawMode[] = {
    LightingNormalTests::DRAW_ARRAYS,
    LightingNormalTests::DRAW_INLINE_BUFFERS,
    LightingNormalTests::DRAW_INLINE_ARRAYS,
    LightingNormalTests::DRAW_INLINE_ELEMENTS,
};

LightingNormalTests::LightingNormalTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Lighting normals") {
  for (auto draw_mode : kDrawMode) {
    for (auto params : kTests) {
      std::string name = MakeTestName(params.set_normal, params.normal, draw_mode);
      tests_[name] = [this, params, draw_mode]() { this->Test(params.set_normal, params.normal, draw_mode); };
    }
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
  p = pb_push3(p, NV097_SET_SCENE_AMBIENT_COLOR, 0x0, 0x0, 0x0);
  p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.0f);

  p = pb_push3(p, NV097_SET_LIGHT_AMBIENT_COLOR, 0, 0, 0);
  p = pb_push3f(p, NV097_SET_LIGHT_DIFFUSE_COLOR, 0.0f, 1.0f, 0.7f);
  p = pb_push3(p, NV097_SET_LIGHT_SPECULAR_COLOR, 0, 0, 0);
  p = pb_push1(p, NV097_SET_LIGHT_LOCAL_RANGE, 0x7149f2ca);  // 1e30
  p = pb_push3(p, NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 0, 0, 0);
  p = pb_push3f(p, NV097_SET_LIGHT_INFINITE_DIRECTION, 0.0f, 0.0f, 1.0f);

  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);

  pb_end(p);
}

void LightingNormalTests::Initialize() {
  TestSuite::Initialize();

  host_.SetShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);

  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_SPECULAR), 0);
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_DIFFUSE), 0xFFFFFFFF);
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_SPECULAR), 0);
  pb_end(p);

  SetLightAndMaterial();
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

    lit_index_buffer_.clear();
    lit_index_buffer_.push_back(2);  // Intentionally specify the vertices out of order.
    lit_index_buffer_.push_back(0);
    lit_index_buffer_.push_back(1);
  }
}

void LightingNormalTests::Test(bool set_normal, const float* normal, DrawMode draw_mode) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  uint32_t* p;

  if (set_normal) {
    p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x20001);
    pb_end(p);

    Vertex* buf = normal_bleed_buffer_->Lock();
    memcpy(buf[2].normal, normal, sizeof(buf[2].normal));
    normal_bleed_buffer_->Unlock();

    host_.SetVertexBuffer(normal_bleed_buffer_);
    host_.DrawArrays(host_.POSITION | host_.NORMAL);
  }

  // Render the test subject with no normals but lighting enabled.
  p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);
  pb_end(p);

  uint32_t vertex_elements = host_.POSITION | host_.DIFFUSE;
  host_.SetVertexBuffer(lit_buffer_);
  switch (draw_mode) {
    case DRAW_ARRAYS:
      host_.DrawArrays(vertex_elements);
      break;

    case DRAW_INLINE_BUFFERS:
      host_.DrawInlineBuffer(vertex_elements);
      break;

    case DRAW_INLINE_ELEMENTS:
      host_.DrawInlineElements16(lit_index_buffer_, vertex_elements);
      break;

    case DRAW_INLINE_ARRAYS:
      host_.DrawInlineArray(vertex_elements);
      break;
  }

  if (!set_normal) {
    pb_print("No normal\n");
  } else {
    pb_print("Nx: %g\nNy: %g\nNz: %g\n", normal[0], normal[1], normal[2]);
  }

  switch (draw_mode) {
    case DRAW_ARRAYS:
      break;

    case DRAW_INLINE_BUFFERS:
      pb_print("Inline buffer\n");
      break;

    case DRAW_INLINE_ELEMENTS:
      pb_print("Inline elements\n");
      break;

    case DRAW_INLINE_ARRAYS:
      pb_print("Inline arrays\n");
      break;
  }
  pb_draw_text_screen();

  std::string name = MakeTestName(set_normal, normal, draw_mode);
  host_.FinishDrawAndSave(output_dir_, name);
}

std::string LightingNormalTests::MakeTestName(bool set_normal, const float* normal, DrawMode draw_mode) {
  char buf[128] = {0};
  static constexpr const char* kModeSuffix[] = {
      "",
      "-inlinebuf",
      "-inlinearrays",
      "-inlineelements",
  };

  if (!set_normal) {
    snprintf(buf, 127, "NoNormal%s", kModeSuffix[draw_mode]);
  } else {
    snprintf(buf, 127, "Nz_%d%s", (int)(normal[2] * 100), kModeSuffix[draw_mode]);
  }
  return buf;
}
