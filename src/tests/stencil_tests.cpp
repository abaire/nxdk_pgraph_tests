#include "stencil_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

constexpr StencilTests::StencilParams kStencilParams[] = {
    // Stencil enable, depth disable
    {NV097_SET_STENCIL_OP_V_ZERO, "ZERO", true, false, 0xFF, 0},
    {NV097_SET_STENCIL_OP_V_REPLACE, "REPLACE", true, false, 0x00, 1},
    // Stencil disable, depth enable
    {NV097_SET_STENCIL_OP_V_ZERO, "ZERO", false, true, 0xFF, 0},
    {NV097_SET_STENCIL_OP_V_REPLACE, "REPLACE", false, true, 0x00, 1},
    // Stencil enable, depth enable
    {NV097_SET_STENCIL_OP_V_ZERO, "ZERO", true, true, 0xFF, 0},
    {NV097_SET_STENCIL_OP_V_REPLACE, "REPLACE", true, true, 0x00, 1},
    // Stencil disable, depth disable
    {NV097_SET_STENCIL_OP_V_ZERO, "ZERO", false, false, 0xFF, 0},
    {NV097_SET_STENCIL_OP_V_REPLACE, "REPLACE", false, false, 0x00, 1}};

StencilTests::StencilTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Stencil", config) {
  for (auto param : kStencilParams) {
    AddTestEntry(param);
  }
}

void StencilTests::Initialize() {
  TestSuite::Initialize();
  SetDefaultTextureFormat();

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void StencilTests::Test(const StencilParams &params) {
  int quadSize = 200;

  host_.SetupControl0(true);
  host_.SetSurfaceFormat(host_.GetColorBufferFormat(),
                         static_cast<TestHost::SurfaceZetaFormat>(NV097_SET_SURFACE_FORMAT_ZETA_Z24S8),
                         host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.PrepareDraw(0xFF000000, 0x0000FFFF, params.stencil_clear_value);

  // Disable color/zeta limit errors
  auto crash_register = reinterpret_cast<uint32_t *>(PGRAPH_REGISTER_BASE + 0x880);
  auto crash_register_pre_test = *crash_register;
  *crash_register = crash_register_pre_test & (~0x800);

  // Enable color, disable depth/stencil, set depth/stencil test params
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_COLOR_MASK, 0x01010101);
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
    p = pb_push1(p, NV097_SET_DEPTH_MASK, false);
    p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
    p = pb_push1(p, NV097_SET_STENCIL_MASK, 0xFF);
    p = pb_push1(p, NV097_SET_STENCIL_FUNC, 0x207);
    p = pb_push1(p, NV097_SET_STENCIL_FUNC_REF, params.stencil_ref_value);
    p = pb_push1(p, NV097_SET_STENCIL_FUNC_MASK, 0xFF);
    p = pb_push1(p, NV097_SET_STENCIL_OP_FAIL, NV097_SET_STENCIL_OP_V_KEEP);
    p = pb_push1(p, NV097_SET_STENCIL_OP_ZFAIL, NV097_SET_STENCIL_OP_V_KEEP);
    p = pb_push1(p, NV097_SET_STENCIL_OP_ZPASS, params.stencil_op_zpass);
    pb_end(p);
  }

  // Draw a red quad in the center of the screen
  this->CreateGeometry(quadSize, 1, 0, 0);
  host_.DrawArrays();

  // Disable all masks except stencil, setup stencil test
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_COLOR_MASK, 0);
    p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, params.stencil_test_enabled);
    p = pb_push1(p, NV097_SET_DEPTH_MASK, false);
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, params.depth_test_enabled);
    p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);
    pb_end(p);
  }

  // Draw a smaller quad to the zeta buffer
  this->CreateGeometry(quadSize / 2.0f, 0, 0, 1);
  host_.DrawArrays();

  // Reenable masks, stencil test
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_COLOR_MASK, 0x01010101);
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
    p = pb_push1(p, NV097_SET_DEPTH_MASK, false);
    p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, true);
    p = pb_push1(p, NV097_SET_STENCIL_FUNC, 0x202);
    p = pb_push1(p, NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_KEEP);
    pb_end(p);
  }

  // Draw the first quad again, with a different color
  this->CreateGeometry(quadSize, 0, 1, 0);
  host_.DrawArrays();

  pb_print("Stencil Op: %s\n", params.stencil_op_zpass_str);
  pb_print("Stencil test: %s\n", params.stencil_test_enabled ? "Enabled" : "Disabled");
  pb_print("Depth test: %s\n", params.depth_test_enabled ? "Enabled" : "Disabled");
  pb_draw_text_screen();

  std::string name = MakeTestName(params);
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name, true);

  // Restore pgraph register 0x880
  *crash_register = crash_register_pre_test;
}

void StencilTests::CreateGeometry(const float sideLength, const float r, const float g, const float b) {
  auto fb_width = host_.GetFramebufferWidthF();
  auto fb_height = host_.GetFramebufferHeightF();

  float centerX = floorf(fb_width / 2.0f);
  float centerY = floorf(fb_height / 2.0f);
  float halfLength = floorf(sideLength / 2.0f);

  uint32_t num_quads = 1;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);

  uint32_t idx = 0;

  Color ul{};
  Color ll{};
  Color lr{};
  Color ur{};

  {
    float z = 0.0f;

    ul.SetRGB(r, g, b);
    ll.SetRGB(r, g, b);
    ur.SetRGB(r, g, b);
    lr.SetRGB(r, g, b);

    buffer->DefineBiTri(idx++, centerX - halfLength, centerY - halfLength, centerX + halfLength, centerY + halfLength,
                        z, z, z, z, ul, ll, lr, ur);
  }
}

std::string StencilTests::MakeTestName(const StencilParams &params) {
  char buf[128] = {0};
  snprintf(buf, 127, "Stencil_%s%s%s", params.stencil_op_zpass_str, params.stencil_test_enabled ? "_ST" : "",
           params.depth_test_enabled ? "_DT" : "");
  return buf;
}

void StencilTests::AddTestEntry(const StencilTests::StencilParams &format) {
  std::string name = MakeTestName(format);

  auto test = [this, format]() { this->Test(format); };

  tests_[name] = test;
}
