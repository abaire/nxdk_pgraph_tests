#include "alpha_func_tests.h"

#include <memory>

#include "shaders/precalculated_vertex_shader.h"

struct TestConfig {
  const char* name;
  uint32_t alpha_func;
};

static const TestConfig testConfigs[]{
    {"AlphaFuncNever", NV097_SET_ALPHA_FUNC_V_NEVER},
    {"AlphaFuncLessThan", NV097_SET_ALPHA_FUNC_V_LESS},
    {"AlphaFuncEqual", NV097_SET_ALPHA_FUNC_V_EQUAL},
    {"AlphaFuncLessThanOrEqual", NV097_SET_ALPHA_FUNC_V_LEQUAL},
    {"AlphaFuncGreaterThan", NV097_SET_ALPHA_FUNC_V_GREATER},
    {"AlphaFuncNotEqual", NV097_SET_ALPHA_FUNC_V_NOTEQUAL},
    {"AlphaFuncGreaterThanOrEqual", NV097_SET_ALPHA_FUNC_V_GEQUAL},
    {"AlphaFuncAlways", NV097_SET_ALPHA_FUNC_V_ALWAYS},
};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc AlphaFuncNever_Enabled
 *  Tests NV097_SET_ALPHA_FUNC_V_NEVER with NV097_SET_ALPHA_TEST_ENABLE = true
 *
 * @tc AlphaFuncLessThan_Enabled
 *  Tests NV097_SET_ALPHA_FUNC_V_LESS with NV097_SET_ALPHA_TEST_ENABLE = true
 *
 * @tc AlphaFuncEqual_Enabled
 *  Tests NV097_SET_ALPHA_FUNC_V_EQUAL with NV097_SET_ALPHA_TEST_ENABLE = true
 *
 * @tc AlphaFuncLessThanOrEqual_Enabled
 *  Tests NV097_SET_ALPHA_FUNC_V_LEQUAL with NV097_SET_ALPHA_TEST_ENABLE = true
 *
 * @tc AlphaFuncGreaterThan_Enabled
 *  Tests NV097_SET_ALPHA_FUNC_V_GREATER with NV097_SET_ALPHA_TEST_ENABLE = true
 *
 * @tc AlphaFuncNotEqual_Enabled
 *  Tests NV097_SET_ALPHA_FUNC_V_NOTEQUAL with NV097_SET_ALPHA_TEST_ENABLE =
 *  true
 *
 * @tc AlphaFuncGreaterThanOrEqual_Enabled
 *  Tests NV097_SET_ALPHA_FUNC_V_GEQUAL with NV097_SET_ALPHA_TEST_ENABLE = true
 *
 * @tc AlphaFuncAlways_Enabled
 *  Tests NV097_SET_ALPHA_FUNC_V_ALWAYS with NV097_SET_ALPHA_TEST_ENABLE = true
 *
 * @tc AlphaFuncNever_Disabled
 *  Tests NV097_SET_ALPHA_FUNC_V_NEVER with NV097_SET_ALPHA_TEST_ENABLE = false
 *
 * @tc AlphaFuncLessThan_Disabled
 *  Tests NV097_SET_ALPHA_FUNC_V_LESS with NV097_SET_ALPHA_TEST_ENABLE = false
 *
 * @tc AlphaFuncEqual_Disabled
 *  Tests NV097_SET_ALPHA_FUNC_V_EQUAL with NV097_SET_ALPHA_TEST_ENABLE = false
 *
 * @tc AlphaFuncLessThanOrEqual_Disabled
 *  Tests NV097_SET_ALPHA_FUNC_V_LEQUAL with NV097_SET_ALPHA_TEST_ENABLE = false
 *
 * @tc AlphaFuncGreaterThan_Disabled
 *  Tests NV097_SET_ALPHA_FUNC_V_GREATER with NV097_SET_ALPHA_TEST_ENABLE =
 *  false
 *
 * @tc AlphaFuncNotEqual_Disabled
 *  Tests NV097_SET_ALPHA_FUNC_V_NOTEQUAL with NV097_SET_ALPHA_TEST_ENABLE =
 *  false
 *
 * @tc AlphaFuncGreaterThanOrEqual_Disabled
 *  Tests NV097_SET_ALPHA_FUNC_V_GEQUAL with NV097_SET_ALPHA_TEST_ENABLE = false
 *
 * @tc AlphaFuncAlways_Disabled
 *  Tests NV097_SET_ALPHA_FUNC_V_ALWAYS with NV097_SET_ALPHA_TEST_ENABLE = false
 *
 */
AlphaFuncTests::AlphaFuncTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Alpha func", config) {
  for (auto testConfig : testConfigs) {
    {
      std::string test_name = testConfig.name;
      test_name += "_Enabled";
      tests_[test_name] = [this, testConfig, test_name]() { this->Test(test_name, testConfig.alpha_func, true); };
    }

    {
      std::string test_name = testConfig.name;
      test_name += "_Disabled";
      tests_[test_name] = [this, testConfig, test_name]() { this->Test(test_name, testConfig.alpha_func, false); };
    }
  }
}

void AlphaFuncTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, false);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, false);
  pb_end(p);
}

void AlphaFuncTests::Test(const std::string& name, uint32_t alpha_func, bool enable) {
  const auto kFBHeight = host_.GetFramebufferHeightF();
  const auto kTop = (kFBHeight - 256.f) / 3.f * 2.f;

  host_.PrepareDraw(0xFF222322);

  host_.SetAlphaReference(0x7F);
  host_.SetAlphaFunc(enable, alpha_func);
  auto top = kTop;
  auto bottom = top + 64.f;
  Draw(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, top, bottom);
  top = bottom;
  bottom += 64.f;
  Draw(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, top, bottom);
  top = bottom;
  bottom += 64.f;
  Draw(0.0f, 1.0f, 0.0f, 0.495f, 0.505f, top, bottom);

  top = bottom;
  bottom += 32.f;
  {
    const auto kFBWidth = host_.GetFramebufferWidthF();
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0x7FFFFFFF);
    host_.SetVertex(0.f, top, 0.1f, 1.0f);
    host_.SetVertex(kFBWidth, top, 0.1f, 1.0f);
    host_.SetVertex(kFBWidth, bottom, 0.1f, 1.0f);
    host_.SetVertex(0.f, bottom, 0.1f, 1.0f);
    host_.End();
  }

  // TODO: Needed?
  host_.WaitForGPU();
  host_.SetAlphaFunc(false);

  pb_print("%s\n", name.c_str());
  pb_print("Alpha ref 0x7F\n");
  pb_print("Blue rect alpha 1 -> 0\n");
  pb_print("Red rect alpha 0 -> 1\n");
  pb_print("Green rect alpha 0.495 - 0.505\n");
  pb_print("White rect alpha 0x7F\n");
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void AlphaFuncTests::Draw(float red, float green, float blue, float left_alpha, float right_alpha, float top,
                          float bottom) const {
  const auto kFBWidth = host_.GetFramebufferWidthF();

  const auto kLeft = (kFBWidth - 512.f) / 2.f;
  const auto kRight = kLeft + 512.f;

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(red, green, blue, left_alpha);
  host_.SetVertex(kLeft, top, 0.1f, 1.0f);
  host_.SetDiffuse(red, green, blue, right_alpha);
  host_.SetVertex(kRight, top, 0.1f, 1.0f);
  host_.SetDiffuse(red, green, blue, right_alpha);
  host_.SetVertex(kRight, bottom, 0.1f, 1.0f);
  host_.SetDiffuse(red, green, blue, left_alpha);
  host_.SetVertex(kLeft, bottom, 0.1f, 1.0f);
  host_.End();
}
