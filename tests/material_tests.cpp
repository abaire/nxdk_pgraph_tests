#include "material_tests.h"

#include <pbkit/pbkit.h>

#include "../pbkit_ext.h"
#include "../test_host.h"
#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

MaterialTests::MaterialTests(TestHost& host, std::string output_dir) : TestSuite(host, std::move(output_dir)) {
  tests_["Material"] = [this]() { this->Test(); };
}

void MaterialTests::Initialize() {
  // NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8
  const TextureFormatInfo& texture_format = kTextureFormats[3];
  host_.SetTextureFormat(texture_format);

  host_.SetShaderProgram(nullptr);

  host_.SetDepthBufferFormat(NV097_SET_SURFACE_FORMAT_ZETA_Z16);
  host_.SetDepthBufferFloatMode(false);

  host_.SetTextureStageEnabled(0, false);

  CreateGeometry();

  auto p = pb_begin();
  MATRIX matrix;
  matrix_unit(matrix);
  p = pb_push_transposed_matrix(p, NV20_TCL_PRIMITIVE_3D_MODELVIEW_MATRIX, matrix);

  matrix[_11] = 1;  // 0x3F800000
  matrix[_12] = 0;  // 0x0
  matrix[_13] = 0;  // 0x0
  matrix[_14] = 0;  // 0x0
  matrix[_21] = 0;  // 0x0
  matrix[_22] = 1;  // 0x3F800000
  matrix[_23] = 0;  // 0x0
  matrix[_24] = 0;  // 0x0
  matrix[_31] = 0;  // 0x0
  matrix[_32] = 0;  // 0x0
  matrix[_33] = 1;  // 0x3F800000
  matrix[_34] = 0;  // 0x0
  p = pb_push_4x3_matrix(p, NV20_TCL_PRIMITIVE_3D_INVERSE_MODELVIEW_MATRIX, matrix);

  matrix_unit(matrix);
  p = pb_push_transposed_matrix(p, NV20_TCL_PRIMITIVE_3D_PROJECTION_MATRIX, matrix);

  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);
  p = pb_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE, 0x4);
  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0x0);
  p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 0x0);
  pb_end(p);
}

void MaterialTests::Deinitialize() {
  host_.SetTextureStageEnabled(0, true);
  TestSuite::Deinitialize();
}

void MaterialTests::CreateGeometry() {
  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  float left = -1.0f * floorf(fb_width / 4.0f);
  float right = floorf(fb_width / 4.0f);
  float top = -1.0f * floorf(fb_height / 3.0f);
  float bottom = floorf(fb_height / 3.0f);
  float mid_width = left + (right - left) * 0.5f;

  uint32_t num_quads = 2;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);

  Color ul{0.4, 0.1, 0.1, 0.25};
  Color ll{0.0, 1.0, 0.0, 1.0};
  Color lr{0.0, 0.0, 1.0, 1.0};
  Color ur{0.5, 0.5, 0.5, 1.0};

  uint32_t idx = 0;
  buffer->DefineQuad(0, left + 10, top + 4, mid_width + 10, bottom - 10, 10.0f, 10.0f, 10.0f, 10.0f, ul, ll, lr, ur);
  // Point normals for half the quad away from the camera.
  Vertex* v = buffer->Lock();
  v[0].normal[2] = -1.0f;
  v[1].normal[2] = -1.0f;
  v[2].normal[2] = -1.0f;
  buffer->Unlock();

  buffer->DefineQuad(1, mid_width - 10, top + 4, right - 10, bottom - 10, 10.0f, 10.0f, 10.0f, 10.0f, ul, ll, lr, ur);
}

void MaterialTests::Test() {
  host_.PrepareDraw(0xFF303030);

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

#define NV097_SET_SPECULAR_PARAMS 0x9E0
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS, 0xBF7730E0);
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 4, 0xC0497B30);
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 8, 0x404BAEF8);
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 12, 0xBF6E9EE4);
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 16, 0xC0463F88);
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 20, 0x404A97CF);

#define NV097_SET_LIGHT_CONTROL NV20_TCL_PRIMITIVE_3D_LIGHT_CONTROL
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x10001);

  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, 0x1);
#define NV097_SET_SPECULAR_ENABLE 0x03b8
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, 0x1);

  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  p = pb_push3(p, NV097_SET_SCENE_AMBIENT_COLOR, 0x0, 0x3C6DDACA, 0x0);

#define NV097_SET_MATERIAL_EMISSION 0x03a8
  p = pb_push1(p, NV097_SET_MATERIAL_EMISSION, 0x0);
  p = pb_push1(p, NV097_SET_MATERIAL_EMISSION + 4, 0x0);
  p = pb_push1(p, NV097_SET_MATERIAL_EMISSION + 8, 0x0);

  // Material's diffuse alpha float
  p = pb_push1(p, NV097_SET_MATERIAL_ALPHA, 0x3E888889);

  pb_end(p);

  host_.DrawVertices(host_.POSITION | host_.NORMAL | host_.DIFFUSE);

  //  std::string name = MakeTestName(front_face, cull_face);
  //  host_.FinishDrawAndSave(output_dir_.c_str(), name.c_str());
  host_.FinishDraw();
}

std::string MaterialTests::MakeTestName() {
  std::string ret = "Material_";

  return std::move(ret);
}
