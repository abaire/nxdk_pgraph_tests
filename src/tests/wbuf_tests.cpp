#include "wbuf_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "shaders/perspective_vertex_shader.h"
#include "texture_generator.h"

static constexpr const char kTestWBuf[] = "wbuf";
static constexpr float kWBufferZBias = -16.0f;

static std::string MakeTestName(bool zbias, bool vsh) {
  return std::string(kTestWBuf) + "_VSH" + (vsh ? "1" : "0") + "_ZB" + (zbias ? "1" : "0");
}

WBufTests::WBufTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "W buffering", config) {
  for (auto zbias : {false, true}) {
    for (auto vsh : {false, true}) {
      tests_[MakeTestName(zbias, vsh)] = [this, zbias, vsh]() { this->Test(zbias, vsh); };
    }
  }
}

void WBufTests::Initialize() { TestSuite::Initialize(); }

void WBufTests::Deinitialize() { TestSuite::Deinitialize(); }

void WBufTests::Test(bool zbias, bool vsh) {
  float zbias_value = 0.0f;

  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());

  if (vsh) {
    float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
    auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                            0.0f, depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
    shader->SetUseD3DStyleViewport();
    vector_t camera_position = {0.0f, 50.0f, -7.0f, 1.0f};
    vector_t camera_look_at = {0.0f, 47.4f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);

    host_.SetVertexShaderProgram(shader);
  } else {
    host_.SetVertexShaderProgram(nullptr);
    host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
    vector_t eye{0.0f, 50.0f, -7.0f, 1.0f};
    vector_t at{0.0f, 47.4f, 0.0f, 1.0f};
    vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
    matrix4_t matrix;
    host_.BuildD3DModelViewMatrix(matrix, eye, at, up);
    host_.SetFixedFunctionModelViewMatrix(matrix);
  }

  host_.SetWindowClip(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(), 150, 0, 0);
  host_.PrepareDraw(0xFF251135);

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
  p = pb_push1(p, NV097_SET_CONTROL0, 0x100000 | NV097_SET_CONTROL0_Z_PERSPECTIVE_ENABLE);
  p = pb_push1f(p, NV097_SET_POLYGON_OFFSET_BIAS, 0.0f);
  p = pb_push1(p, NV097_SET_POLY_OFFSET_FILL_ENABLE, 0);
  if (zbias) {
    zbias_value = kWBufferZBias;
    p = pb_push1f(p, NV097_SET_POLYGON_OFFSET_BIAS, zbias_value);
    p = pb_push1(p, NV097_SET_POLY_OFFSET_FILL_ENABLE, 1);
  }
  pb_end(p);

  // Generate a distinct texture.
  constexpr uint32_t kTextureSize = 256;
  {
    memset(host_.GetTextureMemory(), 0, host_.GetTextureMemorySize());
    GenerateSwizzledRGBACheckerboard(host_.GetTextureMemory(), 0, 0, kTextureSize, kTextureSize, kTextureSize * 4);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
    texture_stage.SetUWrap(TextureStage::WRAP_REPEAT);
    texture_stage.SetVWrap(TextureStage::WRAP_REPEAT);
    host_.SetupTextureStages();

    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
  }

  {
    float tu = 100.0;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(-15000.0f, 0.0f, 30000.0f, 1.0f);

    host_.SetTexCoord0(tu, 0.0f);
    host_.SetVertex(15000.0f, 0.0f, 30000.0f, 1.0f);

    host_.SetTexCoord0(tu, tu);
    host_.SetVertex(15000.0f, 0.0f, -10.0f, 1.0f);

    host_.SetTexCoord0(0.0f, tu);
    host_.SetVertex(-15000.0f, 0.0f, -10.0f, 1.0f);
    host_.End();
  }

  while (pb_busy()) {
    /* Wait for completion... */
  }

  {
    unsigned int depth_pitch = pb_depth_stencil_pitch();
    unsigned int depth_size = depth_pitch * host_.GetFramebufferHeight();
    auto depth_buf = std::make_unique<uint8_t[]>(depth_size);
    memcpy(depth_buf.get(), pb_agp_access(pb_depth_stencil_buffer()), depth_size);

    auto shader = std::make_shared<PassthroughVertexShader>();
    host_.SetVertexShaderProgram(shader);
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetWindowClip(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
    p = pb_push1(p, NV097_SET_DEPTH_MASK, false);
    p = pb_push1f(p, NV097_SET_POLYGON_OFFSET_BIAS, 0.0f);
    p = pb_push1(p, NV097_SET_POLY_OFFSET_FILL_ENABLE, 0);
    pb_end(p);

    const int leftx = 122;
    const int rightx = 159;

    for (int n = 0, y = 32; n < 15 && y < 480; n++, y += 25) {
      uint32_t val = *(uint32_t *)&depth_buf.get()[y * depth_pitch + rightx * 4];
      pb_print("Z=0x%06x\n", val >> 8);
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetDiffuse(1.0f, 1.0f, 0.0f);
      host_.SetVertex(leftx, y, 0.0f, 1.0f);
      host_.SetVertex(rightx + 1, y, 0.0f, 1.0f);
      host_.SetVertex(rightx + 1, y + 1, 0.0f, 1.0f);
      host_.SetVertex(leftx, y + 1, 0.0f, 1.0f);
      host_.End();
    }
  }

  pb_printat(12, 20, "W-buffering value test");
  pb_printat(13, 20, "VSH: %s", vsh ? "Prog" : "FF");
  pb_printat(14, 20, "");
  pb_print("ZBias: %.1f", zbias_value);
  pb_draw_text_screen();

  std::string name = MakeTestName(zbias, vsh);
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name, true);
}
