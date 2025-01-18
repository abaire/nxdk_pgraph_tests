#include "clipping_precision_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "texture_generator.h"

static constexpr const char kTestClippingPrecision[] = "clip_prec_large_xy";

ClippingPrecisionTests::ClippingPrecisionTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Clipping precision", config) {
  tests_[kTestClippingPrecision] = [this]() { this->TestClippingPrecision(); };
}

void ClippingPrecisionTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void ClippingPrecisionTests::Deinitialize() { TestSuite::Deinitialize(); }

void ClippingPrecisionTests::TestClippingPrecisionFrame(float ofs, bool done) {
  host_.PrepareDraw(0xFE251135);

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
    auto p = pb_begin();
    p = pb_push1f(p, NV097_SET_CLIP_MIN, 0.0f);
    p = pb_push1f(p, NV097_SET_CLIP_MAX, 16777215.0f);
    p = pb_push1(p, NV097_SET_CONTROL0, 0x100001 | NV097_SET_CONTROL0_Z_PERSPECTIVE_ENABLE);
    pb_end(p);
  }

  {
    host_.Begin(TestHost::PRIMITIVE_TRIANGLES);

    host_.SetTexCoord0(1.0, 0.0, 0.0, 1.0);
    host_.SetVertex(480.0, 100.0, 0.95 * 16777215.0, 30000.0);

    host_.SetTexCoord0(1.0, 0.5, 0.0, 1.0);
    host_.SetVertex(480.0, 240.0, 0.95 * 16777215.0, 30000.0);

    host_.SetTexCoord0(0.0, 0.0, 0.0, 1.0);
    float w2 = (8.0 + ofs) / 4.0;
    host_.SetVertex(-15000000.0 / w2, -2500000.0 / w2, 0.95 * 16777215.0, w2);

    host_.SetTexCoord0(1.0, 0.0, 0.0, 1.0);
    host_.SetVertex(480.0, 240.0, 0.95 * 16777215.0, 30000.0);

    host_.SetTexCoord0(1.0, 0.5, 0.0, 1.0);
    host_.SetVertex(480.0, 380.0, 0.95 * 16777215.0, 30000.0);

    host_.SetTexCoord0(0.0, 0.0, 0.0, 1.0);
    host_.SetVertex(-15000000.0 / w2, 2500000.0 / w2, 0.95 * 16777215.0, w2);

    host_.End();
  }

  pb_printat(8, 0, (char *)(done ? "Done!" : "Textures should not twitch!"));
  pb_print("\nOfs: %f\n", ofs);
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kTestClippingPrecision);

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
}

void ClippingPrecisionTests::TestClippingPrecision() {
  float ofs = 0.0;
  int num_frames = 200;

  for (int i = 0; i < num_frames; i++) {
    TestClippingPrecisionFrame(ofs, i == num_frames - 1);
    ofs += 0.001;
  }
}
