#include "color_zeta_disable_tests.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF000000;

static constexpr const char kTestName[] = "MaskOff";

ColorZetaDisableTests::ColorZetaDisableTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Color Zeta Disable", config) {
  tests_[kTestName] = [this]() { Test(); };
}

void ColorZetaDisableTests::Initialize() {
  TestSuite::Initialize();
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  host_.SetCombinerControl(1);
}

void ColorZetaDisableTests::Test() {
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.PrepareDraw(0xFE312F31);

  DrawCheckerboardBackground();

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  const float left = host_.GetFramebufferWidthF() * 0.25f;
  const float right = host_.GetFramebufferWidthF() - left;
  const float top = host_.GetFramebufferHeightF() * 0.25f;
  const float bottom = host_.GetFramebufferHeightF() - top;
  const float z = 10.0f;

  // This seems to clear the check that triggers a color/zeta limit error.
  // Spongebob sets this register early on in the game launch.
  auto crash_register = reinterpret_cast<uint32_t *>(PGRAPH_REGISTER_BASE + 0x880);
  auto crash_register_pre_test = *crash_register;
  *crash_register = crash_register_pre_test & (~0x800);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_COLOR_MASK, 0);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_STENCIL_FUNC, 2);
  Pushbuffer::Push(NV097_SET_STENCIL_OP_ZPASS, NV097_SET_STENCIL_OP_V_KEEP);
  Pushbuffer::Push(NV097_SET_STENCIL_OP_ZFAIL, NV097_SET_STENCIL_OP_V_KEEP);
  Pushbuffer::End();

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  // Set RGBA to a distinct pattern.
  host_.SetDiffuse(0.5f, 0.5f, 1.0f, 0.85f);
  host_.SetVertex(left, top, z, 1.0f);
  host_.SetVertex(right, top, z, 1.0f);
  host_.SetVertex(right, bottom, z, 1.0f);
  host_.SetVertex(left, bottom, z, 1.0f);
  host_.End();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_COLOR_MASK,
                   NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE | NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
                       NV097_SET_COLOR_MASK_RED_WRITE_ENABLE | NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE);
  Pushbuffer::End();

  pb_print("%s\n\n", kTestName);
  pb_draw_text_screen();

  std::string z_name = std::string(kTestName) + "_ZB";
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, z_name);

  *crash_register = crash_register_pre_test;
}

void ColorZetaDisableTests::DrawCheckerboardBackground() const {
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  constexpr uint32_t kTextureSize = 256;
  constexpr uint32_t kCheckerSize = 24;

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  GenerateSwizzledRGBACheckerboard(host_.GetTextureMemory(), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4,
                                   kCheckerboardA, kCheckerboardB, kCheckerSize);

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetTexCoord0(0.0f, 0.0f);
  host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);
  host_.SetTexCoord0(1.0f, 0.0f);
  host_.SetVertex(host_.GetFramebufferWidthF(), 0.0f, 0.1f, 1.0f);
  host_.SetTexCoord0(1.0f, 1.0f);
  host_.SetVertex(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF(), 0.1f, 1.0f);
  host_.SetTexCoord0(0.0f, 1.0f);
  host_.SetVertex(0.0f, host_.GetFramebufferHeightF(), 0.1f, 1.0f);
  host_.End();

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
}
