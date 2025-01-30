#include "texgen_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "vertex_buffer.h"

static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 128;

static TextureStage::TexGen kTestModes[] = {
    TextureStage::TG_DISABLE,
    TextureStage::TG_EYE_LINEAR,
    TextureStage::TG_OBJECT_LINEAR,  // xemu "untested" assert (generate_fixed_function) (works on HW)
    //     TextureStage::TG_SPHERE_MAP,  // xemu "channel < 2" assert (kelvin_map_texgen) (works on HW)
    TextureStage::TG_NORMAL_MAP,
    TextureStage::TG_REFLECTION_MAP,
};

TexgenTests::TexgenTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texgen", config) {
  for (auto mode : kTestModes) {
    std::string name = MakeTestName(mode);
    tests_[name] = [this, mode]() { Test(mode); };
  }
}

void TexgenTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8G8B8A8));
  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetBorderColor(0xFF7F007F);
  texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);

  SDL_Surface *gradient_surface;
  int update_texture_result = GenerateSurface(&gradient_surface, kTextureWidth, kTextureHeight);
  ASSERT(!update_texture_result && "Failed to generate SDL surface");
  update_texture_result = host_.SetTexture(gradient_surface, 0);
  SDL_FreeSurface(gradient_surface);
  ASSERT(!update_texture_result && "Failed to set texture");

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
}

void TexgenTests::CreateGeometry() {
  static constexpr float left = -2.75f;
  static constexpr float right = 2.75f;
  static constexpr float top = 1.75f;
  static constexpr float bottom = -1.75f;

  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6);

  buffer->DefineBiTri(0, left, top, right, bottom);
}

void TexgenTests::Test(TextureStage::TexGen mode) {
  std::string test_name = MakeTestName(mode);

  host_.PrepareDraw(0xFE202020);

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTexgenS(mode);
  texture_stage.SetTexgenT(mode);
  texture_stage.SetTexgenR(mode);

  host_.SetupTextureStages();
  host_.DrawArrays();

  pb_print("M: %s\n", test_name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, test_name);
}

std::string TexgenTests::MakeTestName(TextureStage::TexGen mode) {
  switch (mode) {
    case TextureStage::TG_DISABLE:
      return "Disabled";

    case TextureStage::TG_EYE_LINEAR:
      return "EyeLinear";

    case TextureStage::TG_OBJECT_LINEAR:
      return "ObjectLinear";

    case TextureStage::TG_SPHERE_MAP:
      return "SphereMap";

    case TextureStage::TG_NORMAL_MAP:
      return "NormalMap";

    case TextureStage::TG_REFLECTION_MAP:
      return "ReflectionMap";
  }
}
