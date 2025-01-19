#include "clipping_precision_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "texture_generator.h"

static constexpr const char kTestClippingPrecision[] = "clip_prec_large_xy";

static std::string MakeTestName(bool perspective_corrected) {
  return std::string(kTestClippingPrecision) + (perspective_corrected ? "" : "_nopers");
}

ClippingPrecisionTests::ClippingPrecisionTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Clipping precision", config) {
  for (auto perspective_corrected : {false, true}) {
    tests_[MakeTestName(perspective_corrected)] = [this, perspective_corrected]() {
      this->TestClippingPrecision(perspective_corrected);
    };
  }
}

void ClippingPrecisionTests::Initialize() {
  TestSuite::Initialize();
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void ClippingPrecisionTests::Deinitialize() { TestSuite::Deinitialize(); }

void ClippingPrecisionTests::TestClippingPrecisionFrame(float ofs, bool perspective_corrected, bool done) {
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
    p = pb_push1(p, NV097_SET_CONTROL0, 1 | (perspective_corrected ? 0x100000 : 0));
    pb_end(p);
  }

  {
    float tu = perspective_corrected ? 1.0 : 10000.0;

    host_.Begin(TestHost::PRIMITIVE_TRIANGLES);

    host_.SetTexCoord0(tu, 0.0, 0.0, 1.0);
    host_.SetVertex(480.0, 100.0, 0.95 * 16777215.0, 30000.0);

    host_.SetTexCoord0(tu, 0.5, 0.0, 1.0);
    host_.SetVertex(480.0, 240.0, 0.95 * 16777215.0, 30000.0);

    host_.SetTexCoord0(0.0, 0.0, 0.0, 1.0);
    float w2 = (8.0 + ofs) / 4.0;
    host_.SetVertex(-15000000.0 / w2, -2500000.0 / w2, 0.95 * 16777215.0, w2);

    host_.SetTexCoord0(tu, 0.0, 0.0, 1.0);
    host_.SetVertex(480.0, 240.0, 0.95 * 16777215.0, 30000.0);

    host_.SetTexCoord0(tu, 0.5, 0.0, 1.0);
    host_.SetVertex(480.0, 380.0, 0.95 * 16777215.0, 30000.0);

    host_.SetTexCoord0(0.0, 0.0, 0.0, 1.0);
    host_.SetVertex(-15000000.0 / w2, 2500000.0 / w2, 0.95 * 16777215.0, w2);

    host_.End();
  }

  pb_printat(8, 0, (char *)(done ? "Done!" : "Textures should not twitch!"));
  pb_print("\nOfs: %f\n", ofs);
  pb_print("Pers: %d\n", perspective_corrected);
  pb_draw_text_screen();

  std::string test_name = MakeTestName(perspective_corrected);
  host_.FinishDraw(false, output_dir_, suite_name_, test_name);

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
}

void ClippingPrecisionTests::TestClippingPrecision(bool perspective_corrected) {
  float ofs = 0.0;
  int num_frames = 200;

  for (int i = 0; i < num_frames; i++) {
    TestClippingPrecisionFrame(ofs, perspective_corrected, i == num_frames - 1);
    ofs += 0.001;
  }
}
