#include "material_alpha_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

static constexpr uint32_t kDiffuseSource[] = {
    NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_MATERIAL,
    NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_DIFFUSE,
    NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_SPECULAR,
};

static constexpr float kAlphaValues[] = {
    -1.0f, 0.0f, 0.05f, 0.25f, 0.5f, 0.75f, 1.0f, 2.0f,
};

static std::string DiffuseSourceName(uint32_t diffuse_source);

MaterialAlphaTests::MaterialAlphaTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Material Alpha") {
  for (auto source : kDiffuseSource) {
    for (auto alpha : kAlphaValues) {
      std::string name = MakeTestName(source, alpha);

      auto test = [this, source, alpha]() { this->Test(source, alpha); };
      tests_[name] = test;
    }
  }
}

void MaterialAlphaTests::Initialize() {
  TestSuite::Initialize();
  // NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8
  const TextureFormatInfo& texture_format = kTextureFormats[3];
  host_.SetTextureFormat(texture_format);

  host_.SetShaderProgram(nullptr);

  host_.SetDepthBufferFormat(NV097_SET_SURFACE_FORMAT_ZETA_Z16);
  host_.SetDepthBufferFloatMode(false);

  host_.SetTextureStageEnabled(0, false);

  CreateGeometry();

  MATRIX matrix;
  matrix_unit(matrix);
  host_.SetFixedFunctionModelViewMatrix(matrix);
  host_.SetFixedFunctionProjectionMatrix(matrix);
}

void MaterialAlphaTests::Deinitialize() {
  host_.SetTextureStageEnabled(0, true);
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, 0);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, 0);
  pb_end(p);
  TestSuite::Deinitialize();
}

void MaterialAlphaTests::CreateGeometry() {
  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  float left = -1.0f * floorf(fb_width / 4.0f);
  float right = floorf(fb_width / 4.0f);
  float top = -1.0f * floorf(fb_height / 3.0f);
  float bottom = floorf(fb_height / 3.0f);
  float mid_width = left + (right - left) * 0.5f;

  uint32_t num_quads = 2;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);

  Color ul{0.4f, 0.1f, 0.1f, 0.25f};
  Color ll{0.0f, 1.0f, 0.0f, 1.0f};
  Color lr{0.0f, 0.0f, 1.0f, 1.0f};
  Color ur{0.5f, 0.5f, 0.5f, 1.0f};
  Color ul_s{0.0f, 1.0f, 0.0f, 0.5f};
  Color ll_s{1.0f, 0.0f, 0.0f, 0.1f};
  Color lr_s{1.0f, 1.0f, 0.0f, 0.5f};
  Color ur_s{0.0f, 1.0f, 1.0f, 0.75f};

  buffer->DefineQuad(0, left + 10, top + 4, mid_width + 10, bottom - 10, 10.0f, 10.0f, 10.0f, 10.0f, ul, ll, lr, ur,
                     ul_s, ll_s, lr_s, ur_s);
  // Point normals for half the quad away from the camera.
  Vertex* v = buffer->Lock();
  v[0].normal[2] = -1.0f;
  v[1].normal[2] = -1.0f;
  v[2].normal[2] = -1.0f;
  buffer->Unlock();

  ul.SetRGBA(1.0f, 1.0f, 0.0f, 1.0f);
  ul_s.SetRGBA(1.0f, 0.0f, 0.0f, 0.25f);

  ll.SetGreyA(0.5, 1.0f);
  ll_s.SetRGBA(0.3f, 0.3f, 1.0f, 1.0f);

  ur.SetRGBA(0.0f, 0.3f, 0.8f, 0.15f);
  ur_s.SetRGBA(0.9f, 0.9f, 0.4f, 0.33);

  lr.SetRGBA(1.0f, 0.0f, 0.0f, 0.75f);
  lr_s.SetRGBA(0.95f, 0.5f, 0.8f, 0.05);
  buffer->DefineQuad(1, mid_width - 10, top + 4, right - 10, bottom - 10, 10.0f, 10.0f, 10.0f, 10.0f, ul, ll, lr, ur,
                     ul_s, ll_s, lr_s, ur_s);
}

void MaterialAlphaTests::Test(uint32_t diffuse_source, float material_alpha) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  auto p = pb_begin();

  // Set up a directional light.
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);

  // Ambient color comes from the material's diffuse color.
  pb_push_to(SUBCH_3D, p++, NV097_SET_LIGHT_AMBIENT_COLOR, 9);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x3F6EEEEF;  // 0.933333 0xEE
  *(p++) = 0x3F3BBBBC;  // 0.733333 0xBB
  *(p++) = 0x3F2AAAAB;  // 0.666667 0xAA
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  p = pb_push1(p, NV097_SET_LIGHT_LOCAL_RANGE, 0x7149f2ca);  // 1e+30
  pb_push_to(SUBCH_3D, p++, NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 6);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x3F800000;

  p = pb_push1(p, NV097_SET_CONTROL0, 0x100001);

  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x0C, 0xFFFFFFFF);
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x10, 0);
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x1C, 0xFFFFFFFF);
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x20, 0);

  p = pb_push1(p, NV10_TCL_PRIMITIVE_3D_POINT_PARAMETERS_ENABLE, 0x0);

  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS, 0xBF7730E0);
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 4, 0xC0497B30);
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 8, 0x404BAEF8);
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 12, 0xBF6E9EE4);
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 16, 0xC0463F88);
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 20, 0x404A97CF);

  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);

  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, 0x1);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, 0x1);

  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, diffuse_source);
  p = pb_push3(p, NV097_SET_SCENE_AMBIENT_COLOR, 0x0, 0x3C6DDACA, 0x0);

  p = pb_push1(p, NV097_SET_MATERIAL_EMISSION, 0x0);
  p = pb_push1(p, NV097_SET_MATERIAL_EMISSION + 4, 0x0);
  p = pb_push1(p, NV097_SET_MATERIAL_EMISSION + 8, 0x0);

  // Material's diffuse alpha float
  uint32_t alpha_int = *(uint32_t*)&material_alpha;
  p = pb_push1(p, NV097_SET_MATERIAL_ALPHA, alpha_int);

  pb_end(p);

  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

  std::string source = DiffuseSourceName(diffuse_source);
  pb_print("Src: %s\n", source.c_str());
  pb_print("Alpha: %g\n", material_alpha);
  pb_draw_text_screen();

  std::string name = MakeTestName(diffuse_source, material_alpha);
  host_.FinishDrawAndSave(output_dir_, name);
}

std::string MaterialAlphaTests::MakeTestName(uint32_t diffuse_source, float material_alpha) {
  std::string ret = "MatA_S";
  ret += DiffuseSourceName(diffuse_source);
  ret += "_A";
  char buf[16] = {0};
  snprintf(buf, 15, "%X", *(uint32_t*)&material_alpha);
  ret += buf;

  return std::move(ret);
}

static std::string DiffuseSourceName(uint32_t diffuse_source) {
  switch (diffuse_source & 0x30) {
    case NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_MATERIAL:
      return "Mat";
    case NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_DIFFUSE:
      return "VDiffuse";
    case NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_SPECULAR:
      return "VSpec";
  }

  char buf[16] = {0};
  snprintf(buf, 15, "%X", diffuse_source);
  return buf;
}
