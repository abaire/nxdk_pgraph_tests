#include "test_suite.h"

#include "debug_output.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "texture_format.h"

TestSuite::TestSuite(TestHost& host, std::string output_dir, std::string suite_name)
    : host_(host), output_dir_(std::move(output_dir)), suite_name_(std::move(suite_name)) {
  output_dir_ += "\\";
  output_dir_ += suite_name_;
  std::replace(output_dir_.begin(), output_dir_.end(), ' ', '_');
}

std::vector<std::string> TestSuite::TestNames() const {
  std::vector<std::string> ret;
  ret.reserve(tests_.size());

  for (auto& kv : tests_) {
    ret.push_back(kv.first);
  }
  return std::move(ret);
}

void TestSuite::Run(const std::string& test_name) {
  auto it = tests_.find(test_name);
  if (it == tests_.end()) {
    ASSERT(!"Invalid test name");
  }

  it->second();
}

void TestSuite::RunAll() {
  auto names = TestNames();
  for (const auto& test_name : names) {
    Run(test_name);
  }
}

void TestSuite::SetDefaultTextureFormat() const {
  // NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8
  const TextureFormatInfo& texture_format = kTextureFormats[3];
  host_.SetTextureFormat(texture_format);
}

void TestSuite::Initialize() {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, false);
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_OFF);
  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.0f);

  p = pb_push1(p, NV097_SET_BLEND_ENABLE, 1);
  p = pb_push1(p, NV097_SET_BLEND_EQUATION, NV097_SET_BLEND_EQUATION_V_FUNC_ADD);
  p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
  p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA);
  p = pb_push1(p, 0x17C4, 0);
  p = pb_push1(p, NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
  p = pb_push1(p, NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);

  p = pb_push1(p, NV097_SET_FOG_ENABLE, false);
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW0, 0xe);
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW1, 0x1c80);

  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x10, 0);           // Specular
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x1C, 0xFFFFFFFF);  // Back diffuse
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x20, 0);           // Back specular

  p = pb_push1(p, NV097_SET_POINT_PARAMS_ENABLE, false);
  p = pb_push1(p, NV097_SET_POINT_SMOOTH_ENABLE, false);
  p = pb_push1(p, NV097_SET_POINT_SIZE, 8);

  p = pb_push1(p, NV097_SET_COMBINER_CONTROL, 1);

  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_COLOR_ICW, 8);
  *(p++) = 0x4200000;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;

  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_COLOR_OCW, 8);
  *(p++) = 0xC00;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;

  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_ALPHA_ICW, 8);
  *(p++) = 0x14200000;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;

  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_ALPHA_OCW, 8);
  *(p++) = 0xC00;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  pb_end(p);

  while (pb_busy()) {
    /* Wait for completion... */
  }

  p = pb_begin();
  p = pb_push1(p, NV097_SET_SHADER_STAGE_PROGRAM, 0x0);

  {
    uint32_t address = NV097_SET_TEXTURE_ADDRESS;
    uint32_t control = NV097_SET_TEXTURE_CONTROL0;
    uint32_t filter = NV097_SET_TEXTURE_FILTER;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);

    address += 0x40;
    control += 0x40;
    filter += 0x40;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);

    address += 0x40;
    control += 0x40;
    filter += 0x40;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);

    address += 0x40;
    control += 0x40;
    filter += 0x40;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);
  }

  p = pb_push1(p, NV097_SET_FOG_ENABLE, 0x0);
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW0, 0xc);
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW1, 0x1c80);

  {
    uint32_t matrix_enable = NV097_SET_TEXTURE_MATRIX_ENABLE;
    p = pb_push1(p, matrix_enable, 0x0);
    matrix_enable += 4;
    p = pb_push1(p, matrix_enable, 0x0);
    matrix_enable += 4;
    p = pb_push1(p, matrix_enable, 0x0);
    matrix_enable += 4;
    p = pb_push1(p, matrix_enable, 0x0);
  }

  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, 0);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, 0);
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x20001);
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, 0);

  p = pb_push1(p, NV097_SET_FRONT_FACE, NV097_SET_FRONT_FACE_V_CW);
  p = pb_push1(p, NV097_SET_CULL_FACE, NV097_SET_CULL_FACE_V_BACK);
  p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, true);

  p = pb_push1(p, NV097_SET_DEPTH_MASK, true);
  p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, true);

  pb_end(p);

  host_.SetDefaultViewportAndFixedFunctionMatrices();
  host_.SetDepthBufferFormat(NV097_SET_SURFACE_FORMAT_ZETA_Z16);
  host_.SetDepthBufferFloatMode(false);

  SetDefaultTextureFormat();
  host_.SetTextureStageEnabled(0, false);
}
