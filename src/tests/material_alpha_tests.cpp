#include "material_alpha_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"

using namespace XboxMath;

static constexpr uint32_t kDiffuseSource[] = {
    NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_MATERIAL,
    NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_DIFFUSE,
    NV097_SET_COLOR_MATERIAL_DIFFUSE_FROM_VERTEX_SPECULAR,
};

static constexpr float kAlphaValues[] = {
    -1.0f, 0.0f, 0.05f, 0.25f, 0.5f, 0.75f, 1.0f, 2.0f,
};

static std::string DiffuseSourceName(uint32_t diffuse_source);

MaterialAlphaTests::MaterialAlphaTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Material alpha", config) {
  for (auto source : kDiffuseSource) {
    for (auto alpha : kAlphaValues) {
      std::string name = MakeTestName(source, alpha);

      auto test = [this, source, alpha]() { Test(source, alpha); };
      tests_[name] = test;
    }
  }
}

void MaterialAlphaTests::Initialize() {
  TestSuite::Initialize();

  CreateGeometry();

  matrix4_t matrix;
  MatrixSetIdentity(matrix);
  host_.SetFixedFunctionModelViewMatrix(matrix);
  host_.SetFixedFunctionProjectionMatrix(matrix);
}

void MaterialAlphaTests::Deinitialize() {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_LIGHTING_ENABLE, 0);
  Pushbuffer::Push(NV097_SET_SPECULAR_ENABLE, 0);
  Pushbuffer::End();
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

  float z = 10.0f;
  buffer->DefineBiTri(0, left + 10, top + 4, mid_width + 10, bottom - 10, z, z, z, z, ul, ll, lr, ur, ul_s, ll_s, lr_s,
                      ur_s);
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
  z = 9.75f;
  buffer->DefineBiTri(1, mid_width - 10, top + 4, right - 10, bottom - 10, z, z, z, z, ul, ll, lr, ur, ul_s, ll_s, lr_s,
                      ur_s);
}

void MaterialAlphaTests::Test(uint32_t diffuse_source, float material_alpha) {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  Pushbuffer::Begin();

  // Set up a directional light.
  Pushbuffer::Push(NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);

  // Ambient color comes from the material's diffuse color.
  Pushbuffer::Push(NV097_SET_LIGHT_AMBIENT_COLOR, 0, 0, 0);
  Pushbuffer::Push(NV097_SET_LIGHT_DIFFUSE_COLOR, 0x3F6EEEEF, 0x3F3BBBBC, 0x3F2AAAAB);
  Pushbuffer::Push(NV097_SET_LIGHT_SPECULAR_COLOR, 0, 0, 0);

  Pushbuffer::Push(NV097_SET_LIGHT_LOCAL_RANGE, 0x7149f2ca);  // 1e+30

  Pushbuffer::Push(NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 0, 0, 0);
  Pushbuffer::Push(NV097_SET_LIGHT_INFINITE_HALF_VECTOR + 0x0C, 0, 0, 0x3F800000);

  Pushbuffer::Push(NV097_SET_CONTROL0, 0x100001);

  Pushbuffer::Push(NV097_SET_VERTEX_DATA4UB + 0x0C, 0xFFFFFFFF);
  Pushbuffer::Push(NV097_SET_VERTEX_DATA4UB + 0x10, 0);
  Pushbuffer::Push(NV097_SET_VERTEX_DATA4UB + 0x1C, 0xFFFFFFFF);
  Pushbuffer::Push(NV097_SET_VERTEX_DATA4UB + 0x20, 0);

  Pushbuffer::Push(NV10_TCL_PRIMITIVE_3D_POINT_PARAMETERS_ENABLE, 0x0);

  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS, 0xBF7730E0);
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 4, 0xC0497B30);
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 8, 0x404BAEF8);
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 12, 0xBF6E9EE4);
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 16, 0xC0463F88);
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 20, 0x404A97CF);

  Pushbuffer::Push(NV097_SET_LIGHT_CONTROL, 0x10001);

  Pushbuffer::Push(NV097_SET_LIGHTING_ENABLE, 0x1);
  Pushbuffer::Push(NV097_SET_SPECULAR_ENABLE, 0x1);

  Pushbuffer::Push(NV097_SET_COLOR_MATERIAL, diffuse_source);
  Pushbuffer::Push(NV097_SET_SCENE_AMBIENT_COLOR, 0x0, 0x3C6DDACA, 0x0);

  Pushbuffer::Push(NV097_SET_MATERIAL_EMISSION, 0x0);
  Pushbuffer::Push(NV097_SET_MATERIAL_EMISSION + 4, 0x0);
  Pushbuffer::Push(NV097_SET_MATERIAL_EMISSION + 8, 0x0);
  Pushbuffer::End();

  // Material's diffuse alpha float
  PGRAPHDiffToken diff_token(true, false);
  Pushbuffer::Begin();
  uint32_t alpha_int = *(uint32_t*)&material_alpha;
  Pushbuffer::Push(NV097_SET_MATERIAL_ALPHA, alpha_int);
  Pushbuffer::End();
  diff_token.DumpDiff();

  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

  std::string source = DiffuseSourceName(diffuse_source);
  pb_print("Src: %s\n", source.c_str());
  pb_print("Alpha: %g\n", material_alpha);
  pb_draw_text_screen();

  std::string name = MakeTestName(diffuse_source, material_alpha);
  FinishDraw(name);
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
