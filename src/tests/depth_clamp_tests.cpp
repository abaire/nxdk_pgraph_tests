#include "depth_clamp_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/perspective_vertex_shader.h"
#include "texture_generator.h"

static constexpr char kTestDepthClamp[] = "depth_clamp";
static constexpr char kTestEqualDepth[] = "depth_equal";
static constexpr float kZBias = -500000.0f;
static constexpr float kWBufferZBias = -5.0f;

static std::string MakeTestName(bool w_buffered, bool clamp, bool zbias, bool full_range, bool vsh) {
  return std::string(kTestDepthClamp) + "_W" + (w_buffered ? "1" : "0") + "_C" + (clamp ? "1" : "0") + "_ZB" +
         (zbias ? "1" : "0") + "_FR" + (full_range ? "1" : "0") + "_VSH" + (vsh ? "1" : "0");
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
          for (auto vsh : {false, true}) {
            tests_[MakeTestName(w_buffered, clamp, zbias, full_range, vsh)] = [this, w_buffered, clamp, zbias,
                                                                               full_range, vsh]() {
              this->Test(w_buffered, clamp, zbias, full_range, vsh);
            };
          }
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

void DepthClampTests::Test(bool w_buffered, bool clamp, bool zbias, bool full_range, bool vsh) {
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());

  if (vsh) {
    float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
    auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                            0.0f, depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
    shader->SetUseD3DStyleViewport();
    vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
    vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);

    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
  }

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.PrepareDraw(0xFE251135);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  Pushbuffer::Push(NV097_SET_COMPRESS_ZBUFFER_EN, false);
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_STENCIL_MASK, false);
  Pushbuffer::Push(NV097_SET_ZMIN_MAX_CONTROL, NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL);
  Pushbuffer::PushF(NV097_SET_CLIP_MIN, 0.0f);
  Pushbuffer::PushF(NV097_SET_CLIP_MAX, 16777215.0f);
  Pushbuffer::Push(NV097_SET_CONTROL0, 0x100000 | (w_buffered ? NV097_SET_CONTROL0_Z_PERSPECTIVE_ENABLE : 0));
  Pushbuffer::Push(NV097_SET_POLYGON_OFFSET_BIAS, 0.0f);
  Pushbuffer::Push(NV097_SET_POLY_OFFSET_FILL_ENABLE, 0);
  Pushbuffer::End();

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
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_ZMIN_MAX_CONTROL,
                     clamp ? NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CLAMP : NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL);
    auto clip_min = full_range ? 0.0f : (w_buffered ? 9.01f : 14988020.0f);
    Pushbuffer::PushF(NV097_SET_CLIP_MIN, clip_min);
    auto clip_max = full_range ? 16777215.0f : (w_buffered ? 16.49f : 15869663.0f);
    Pushbuffer::PushF(NV097_SET_CLIP_MAX, clip_max);
    if (zbias) {
      Pushbuffer::PushF(NV097_SET_POLYGON_OFFSET_BIAS, zbias_value);
      Pushbuffer::Push(NV097_SET_POLY_OFFSET_FILL_ENABLE, 1);
    }
    Pushbuffer::End();
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
  pb_print("VSH: %d\n", vsh);
  pb_draw_text_screen();

  FinishDraw(MakeTestName(w_buffered, clamp, zbias, full_range, vsh));

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  {
    Pushbuffer::Begin();
    Pushbuffer::PushF(NV097_SET_POLYGON_OFFSET_BIAS, 0.0f);
    Pushbuffer::Push(NV097_SET_POLY_OFFSET_FILL_ENABLE, 0);
    Pushbuffer::End();
  }
}

void DepthClampTests::TestEqualDepth(bool w_buffered, float ofs) {
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.PrepareDraw(0xFE251135);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  Pushbuffer::Push(NV097_SET_COMPRESS_ZBUFFER_EN, false);
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_STENCIL_MASK, false);
  Pushbuffer::Push(NV097_SET_ZMIN_MAX_CONTROL, NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL);
  Pushbuffer::PushF(NV097_SET_CLIP_MIN, 0.0f);
  Pushbuffer::PushF(NV097_SET_CLIP_MAX, 16777215.0f);
  Pushbuffer::Push(NV097_SET_CONTROL0, 0x100000 | (w_buffered ? NV097_SET_CONTROL0_Z_PERSPECTIVE_ENABLE : 0));
  Pushbuffer::PushF(NV097_SET_POLYGON_OFFSET_BIAS, 0.0f);
  Pushbuffer::Push(NV097_SET_POLY_OFFSET_FILL_ENABLE, 0);
  Pushbuffer::End();

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
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_ZMIN_MAX_CONTROL, NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL);
    Pushbuffer::PushF(NV097_SET_CLIP_MIN, w_buffered ? 255.99f : 14988020.0f);
    Pushbuffer::End();
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
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_ZMIN_MAX_CONTROL, NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CLAMP);
    Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_EQUAL);
    Pushbuffer::End();
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

  FinishDraw(MakeEqualDepthTestName(w_buffered, ofs));
}
