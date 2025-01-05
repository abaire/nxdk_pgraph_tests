#include "depth_clamp_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "texture_generator.h"
#include "vertex_buffer.h"

static constexpr const char kTestDepthClamp[] = "depth_clamp";
static constexpr const char kTestEqualDepth[] = "depth_equal";
static constexpr float kZBias = -500000.0f;
static constexpr float kWBufferZBias = -5.0f;

static std::string MakeTestName(bool w_buffered, bool clamp, bool zbias, bool full_range) {
  return std::string(kTestDepthClamp) + (w_buffered ? "_WBuf" : "") + (clamp ? "_Clamp" : "") +
         (zbias ? "_ZBias" : "") + (full_range ? "_FullR" : "");
}

static std::string MakeEqualDepthTestName(bool w_buffered, float ofs) {
  char ofs_buf[32] = {0};
  snprintf(ofs_buf, sizeof(ofs_buf), "_Ofs%.03f", ofs);
  return std::string(kTestEqualDepth) + (w_buffered ? "_WBuf" : "") + ofs_buf;
}

DepthClampTests::DepthClampTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Depth Clamp", config) {
  for (auto w_buffered : {false, true}) {
    for (auto clamp : {false, true}) {
      for (auto zbias : {false, true}) {
        for (auto full_range : {false, true}) {
          tests_[MakeTestName(w_buffered, clamp, zbias, full_range)] = [this, w_buffered, clamp, zbias, full_range]() {
            this->Test(w_buffered, clamp, zbias, full_range);
          };
        }
      }
    }
  }

  for (auto w_buffered : {false, true}) {
    for (auto ofs : {0.685f, 0.822f, 3.015f, 3.216f}) {
      tests_[MakeEqualDepthTestName(w_buffered, ofs)] = [this, w_buffered, ofs]() {
        this->TestEqualDepth(w_buffered, ofs);
      };
    }
  }
}

void DepthClampTests::Initialize() { TestSuite::Initialize(); }

void DepthClampTests::Deinitialize() { TestSuite::Deinitialize(); }

void DepthClampTests::Test(bool w_buffered, bool clamp, bool zbias, bool full_range) {
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.PrepareDraw(0xFE251135);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, true);
  p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  p = pb_push1(p, NV097_SET_COMPRESS_ZBUFFER_EN, false);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, false);
  p = pb_push1(p, NV097_SET_ZMIN_MAX_CONTROL, NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL);
  p = pb_push1f(p, NV097_SET_CLIP_MIN, 0.0f);
  p = pb_push1f(p, NV097_SET_CLIP_MAX, 16777215.0f);
  p = pb_push1(p, NV097_SET_CONTROL0, 0x100000 | (w_buffered ? NV097_SET_CONTROL0_Z_PERSPECTIVE_ENABLE : 0));
  p = pb_push1f(p, NV097_SET_POLYGON_OFFSET_BIAS, 0.0f);
  p = pb_push1(p, NV097_SET_POLY_OFFSET_FILL_ENABLE, 0);
  pb_end(p);

  // Generate a distinct texture.
  constexpr uint32_t kTextureSize = 256;
  {
    memset(host_.GetTextureMemory(), 0, host_.GetTextureMemorySize());
    GenerateRGBACheckerboard(host_.GetTextureMemory(), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
  }

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(-2.0f, 2.0f, 2.0f, 1.0f);

    host_.SetTexCoord0(kTextureSize, 0.0f);
    host_.SetVertex(-1.0f, 2.0f, 2.0f, 1.0f);

    host_.SetTexCoord0(kTextureSize, kTextureSize);
    host_.SetVertex(-1.0f, -6.5f, 2.0f, 1.0f);

    host_.SetTexCoord0(0.0f, kTextureSize);
    host_.SetVertex(-2.0f, -6.5f, 2.0f, 1.0f);
    host_.End();
  }

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(1.0f, 2.0f, 10.0f, 1.0f);

    host_.SetTexCoord0(kTextureSize, 0.0f);
    host_.SetVertex(2.0f, 2.0f, 10.0f, 1.0f);

    host_.SetTexCoord0(kTextureSize, kTextureSize);
    host_.SetVertex(2.0f, -6.5f, 10.0f, 1.0f);

    host_.SetTexCoord0(0.0f, kTextureSize);
    host_.SetVertex(1.0f, -6.5f, 10.0f, 1.0f);
    host_.End();
  }

  while (pb_busy()) {
    /* Wait for completion... */
  }

  float zbias_value = zbias ? (w_buffered ? kWBufferZBias : kZBias) : 0.0f;

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_ZMIN_MAX_CONTROL,
                 clamp ? NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CLAMP : NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL);
    p = pb_push1f(p, NV097_SET_CLIP_MIN, full_range ? 0.0f : (w_buffered ? 9.01f : 14988020.0f));
    p = pb_push1f(p, NV097_SET_CLIP_MAX, full_range ? 16777215.0f : (w_buffered ? 16.49f : 15869665.0f));
    if (zbias) {
      p = pb_push1f(p, NV097_SET_POLYGON_OFFSET_BIAS, zbias_value);
      p = pb_push1(p, NV097_SET_POLY_OFFSET_FILL_ENABLE, 1);
    }
    pb_end(p);
  }

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(-10.0f, -2.5f, 20.0f, 1.0f);

    host_.SetTexCoord0(kTextureSize, 0.0f);
    host_.SetVertex(10.0f, -2.5f, 20.0f, 1.0f);

    host_.SetTexCoord0(kTextureSize, kTextureSize);
    host_.SetVertex(10.0f, -2.5f, 0.0f, 1.0f);

    host_.SetTexCoord0(0.0f, kTextureSize);
    host_.SetVertex(-10.0f, -2.5f, 0.0f, 1.0f);
    host_.End();
  }

  pb_print("Wbuf: %d\n", w_buffered);
  pb_print("Clamp: %d\n", clamp);
  pb_print("ZBias: %.1f\n", zbias_value);
  pb_print("FullR: %d\n", full_range);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, MakeTestName(w_buffered, clamp, zbias, full_range));

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  {
    auto p = pb_begin();
    p = pb_push1f(p, NV097_SET_POLYGON_OFFSET_BIAS, 0.0f);
    p = pb_push1(p, NV097_SET_POLY_OFFSET_FILL_ENABLE, 0);
    pb_end(p);
  }
}

void DepthClampTests::TestEqualDepth(bool w_buffered, float ofs) {
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.PrepareDraw(0xFE251135);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, true);
  p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  p = pb_push1(p, NV097_SET_COMPRESS_ZBUFFER_EN, false);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, false);
  p = pb_push1(p, NV097_SET_ZMIN_MAX_CONTROL, NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL);
  p = pb_push1f(p, NV097_SET_CLIP_MIN, 0.0f);
  p = pb_push1f(p, NV097_SET_CLIP_MAX, 16777215.0f);
  p = pb_push1(p, NV097_SET_CONTROL0, 0x100000 | (w_buffered ? NV097_SET_CONTROL0_Z_PERSPECTIVE_ENABLE : 0));
  p = pb_push1f(p, NV097_SET_POLYGON_OFFSET_BIAS, 0.0f);
  p = pb_push1(p, NV097_SET_POLY_OFFSET_FILL_ENABLE, 0);
  pb_end(p);

  // Generate a distinct texture.
  constexpr uint32_t kTextureSize = 256;
  {
    memset(host_.GetTextureMemory(), 0, host_.GetTextureMemorySize());
    GenerateRGBACheckerboard(host_.GetTextureMemory(), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
  }

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_ZMIN_MAX_CONTROL, NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL);
    p = pb_push1f(p, NV097_SET_CLIP_MIN, w_buffered ? 255.99f : 14988020.0f);
    pb_end(p);
  }

  float sc = w_buffered ? 100.0f : 1.0f;

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(-10.0f * sc, -2.5f * sc, 200.0f * sc - ofs, 1.0f);

    host_.SetTexCoord0(kTextureSize, 0.0f);
    host_.SetVertex(10.0f * sc, -2.5f * sc, 200.0f * sc - ofs, 1.0f);

    host_.SetTexCoord0(kTextureSize, kTextureSize);
    host_.SetVertex(10.0f * sc, -2.5f * sc, -20.0f + ofs, 1.0f);

    host_.SetTexCoord0(0.0f, kTextureSize);
    host_.SetVertex(-10.0f * sc, -2.5f * sc, -20.0f + ofs, 1.0f);
    host_.End();
  }

  while (pb_busy()) {
    /* Wait for completion... */
  }

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_ZMIN_MAX_CONTROL, NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CLAMP);
    p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_EQUAL);
    pb_end(p);
  }

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(1.0f, 0.0f, 0.0f);
    host_.SetVertex(-10.0f * sc, -2.5f * sc, 200.0f * sc - ofs, 1.0f);

    host_.SetDiffuse(1.0f, 0.0f, 0.0f);
    host_.SetVertex(10.0f * sc, -2.5f * sc, 200.0f * sc - ofs, 1.0f);

    host_.SetDiffuse(1.0f, 0.0f, 0.0f);
    host_.SetVertex(10.0f * sc, -2.5f * sc, -20.0f + ofs, 1.0f);

    host_.SetDiffuse(1.0f, 0.0f, 0.0f);
    host_.SetVertex(-10.0f * sc, -2.5f * sc, -20.0f + ofs, 1.0f);
    host_.End();
  }

  pb_print("Equal Depth Test\n");
  pb_print("Wbuf: %d\n", w_buffered);
  pb_print("Ofs: %f\n", ofs);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, MakeEqualDepthTestName(w_buffered, ofs));
}
