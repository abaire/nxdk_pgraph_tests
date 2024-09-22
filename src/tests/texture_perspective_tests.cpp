#include "texture_perspective_tests.h"

#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "texture_generator.h"
#include "vertex_buffer.h"

static constexpr const char kTestTexturePerspective[] = "tex_tex_pers_";
static constexpr const char kTestDiffusePerspective[] = "tex_diff_pers_";

static std::string MakeTexPersTestName(bool draw_quad, bool perspective_corrected) {
  return std::string(kTestTexturePerspective) + (perspective_corrected ? "y" : "n") + (draw_quad ? "_quad" : "_bitri");
}

static std::string MakeTexDiffTestName(bool draw_quad, bool perspective_corrected) {
  return std::string(kTestDiffusePerspective) + (perspective_corrected ? "y" : "n") + (draw_quad ? "_quad" : "_bitri");
}

TexturePerspectiveTests::TexturePerspectiveTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture perspective", config) {
  for (auto perspective_corrected : {false, true}) {
    for (auto quad : {false, true}) {
      tests_[MakeTexPersTestName(quad, perspective_corrected)] = [this, quad, perspective_corrected]() {
        this->TestTexturePerspective(quad, perspective_corrected);
      };
      tests_[MakeTexDiffTestName(quad, perspective_corrected)] = [this, quad, perspective_corrected]() {
        this->TestDiffusePerspective(quad, perspective_corrected);
      };
    }
  }
}

void TexturePerspectiveTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void TexturePerspectiveTests::Deinitialize() { TestSuite::Deinitialize(); }

void TexturePerspectiveTests::TestTexturePerspective(bool draw_quad, bool perspective_corrected) {
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.PrepareDraw(0xFE251135);

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
    p = pb_push1(p, NV097_SET_CONTROL0, 1 | (perspective_corrected ? 0x100000 : 0));
    pb_end(p);
  }

  {
    const float left = -1.75f;
    const float top = 1.75f;
    const float right = 1.75f;
    const float bottom = -1.75f;
    const auto depth = 0.0f;

    if (draw_quad) {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord0(0.0f, 0.0f);
      host_.SetVertex(left, top, depth, 0.3f);

      host_.SetTexCoord0(kTextureSize, 0.0f);
      host_.SetVertex(right, top, depth, 1.0f);

      host_.SetTexCoord0(kTextureSize, kTextureSize);
      host_.SetVertex(right, bottom, depth, 3.0f);

      host_.SetTexCoord0(0.0f, kTextureSize);
      host_.SetVertex(left, bottom, depth, 1.0f);
      host_.End();
    } else {
      host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
      host_.SetTexCoord0(0.0f, 0.0f);
      host_.SetVertex(left, top, depth, 0.3f);

      host_.SetTexCoord0(kTextureSize, 0.0f);
      host_.SetVertex(right, top, depth, 1.0f);

      host_.SetTexCoord0(0.0f, kTextureSize);
      host_.SetVertex(left, bottom, depth, 1.0f);

      host_.SetTexCoord0(kTextureSize, kTextureSize);
      host_.SetVertex(right, bottom, depth, 3.0f);

      host_.SetTexCoord0(0.0f, kTextureSize);
      host_.SetVertex(left, bottom, depth, 1.0f);

      host_.SetTexCoord0(kTextureSize, 0.0f);
      host_.SetVertex(right, top, depth, 1.0f);

      host_.End();
    }
  }

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, MakeTexPersTestName(draw_quad, perspective_corrected));

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.SetDefaultViewportAndFixedFunctionMatrices();

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
}

void TexturePerspectiveTests::TestDiffusePerspective(bool draw_quad, bool perspective_corrected) {
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.PrepareDraw(0xFE251135);

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_CONTROL0, 1 | (perspective_corrected ? 0x100000 : 0));
    pb_end(p);
  }

  {
    const float left = -1.75f;
    const float top = 1.75f;
    const float right = 1.75f;
    const float bottom = -1.75f;
    const auto depth = 0.0f;

    if (draw_quad) {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetDiffuse(1.0f, 0.0f, 0.0f, 1.0f);
      host_.SetVertex(left, top, depth, 0.3f);

      host_.SetDiffuse(0.0f, 1.0f, 0.0f, 1.0f);
      host_.SetVertex(right, top, depth, 1.0f);

      host_.SetDiffuse(0.0f, 0.0f, 1.0f, 1.0f);
      host_.SetVertex(right, bottom, depth, 0.3f);

      host_.SetDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
      host_.SetVertex(left, bottom, depth, 1.0f);
      host_.End();
    } else {
      host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
      host_.SetDiffuse(1.0f, 0.0f, 0.0f, 1.0f);
      host_.SetVertex(left, top, depth, 0.3f);

      host_.SetDiffuse(0.0f, 1.0f, 0.0f, 1.0f);
      host_.SetVertex(right, top, depth, 1.0f);

      host_.SetDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
      host_.SetVertex(left, bottom, depth, 1.0f);

      host_.SetDiffuse(0.0f, 0.0f, 1.0f, 1.0f);
      host_.SetVertex(right, bottom, depth, 0.3f);

      host_.SetDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
      host_.SetVertex(left, bottom, depth, 1.0f);

      host_.SetDiffuse(0.0f, 1.0f, 0.0f, 1.0f);
      host_.SetVertex(right, top, depth, 1.0f);

      host_.End();
    }
  }

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, MakeTexDiffTestName(draw_quad, perspective_corrected));

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.SetDefaultViewportAndFixedFunctionMatrices();
}
