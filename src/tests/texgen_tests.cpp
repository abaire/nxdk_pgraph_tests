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
    TextureStage::TG_DISABLE,    TextureStage::TG_EYE_LINEAR, TextureStage::TG_OBJECT_LINEAR,
    TextureStage::TG_SPHERE_MAP, TextureStage::TG_NORMAL_MAP, TextureStage::TG_REFLECTION_MAP,
};

struct ViewModelConfig {
  const char *name;
  uint32_t value;
};

static constexpr ViewModelConfig kViewModels[] = {
    {"", NV097_SET_TEXGEN_VIEW_MODEL_LOCAL_VIEWER},
    {"InfiniteViewer", NV097_SET_TEXGEN_VIEW_MODEL_INFINITE_VIEWER},
};

/**
 * @tc Disabled
 *  TexGen is disabled with NV097_SET_TEXGEN_VIEW_MODEL_LOCAL_VIEWER. Tests default texcoords with local viewer model.
 *
 * @tc Disabled_InfiniteViewer
 *  TexGen is disabled with NV097_SET_TEXGEN_VIEW_MODEL_INFINITE_VIEWER. Tests default texcoords with infinite viewer
 *  model.
 *
 * @tc EyeLinear
 *  Tests EyeLinear TexGen with NV097_SET_TEXGEN_VIEW_MODEL_LOCAL_VIEWER. Coordinates are generated from eye-space
 *  vertex positions using eye plane equations.
 *
 * @tc EyeLinear_InfiniteViewer
 *  Tests EyeLinear TexGen with NV097_SET_TEXGEN_VIEW_MODEL_INFINITE_VIEWER. Coordinates are generated from eye-space
 *  vertex positions using eye plane equations; the viewer model should not affect coordinate generation.
 *
 * @tc ObjectLinear
 *  Tests ObjectLinear TexGen with NV097_SET_TEXGEN_VIEW_MODEL_LOCAL_VIEWER. Coordinates are generated from object-space
 *  vertex positions using object plane equations.
 *
 * @tc ObjectLinear_InfiniteViewer
 *  Tests ObjectLinear TexGen with NV097_SET_TEXGEN_VIEW_MODEL_INFINITE_VIEWER. Coordinates are generated from
 *  object-space vertex positions using object plane equations; the viewer model should not affect coordinate
 *  generation.
 *
 * @tc NormalMap
 *  Tests NormalMap TexGen with NV097_SET_TEXGEN_VIEW_MODEL_LOCAL_VIEWER. Coordinates are generated directly from
 *  eye-space vertex normals.
 *
 * @tc NormalMap_InfiniteViewer
 *  Tests NormalMap TexGen with NV097_SET_TEXGEN_VIEW_MODEL_INFINITE_VIEWER. Coordinates are generated directly from
 *  eye-space vertex normals; the viewer model should not affect coordinate generation.
 *
 * @tc ReflectionMap
 *  Tests ReflectionMap TexGen with NV097_SET_TEXGEN_VIEW_MODEL_LOCAL_VIEWER. Reflection vectors are computed using a
 *  local viewer model where the eye vector originates from (0,0,0,1) towards the vertex.
 *
 * @tc ReflectionMap_InfiniteViewer
 *  Tests ReflectionMap TexGen with NV097_SET_TEXGEN_VIEW_MODEL_INFINITE_VIEWER. Reflection vectors are computed using
 *  an infinite parallel viewer model with a fixed direction along the +Z axis.
 *
 * @tc SphereMap.png
 *  Tests SphereMap TexGen with NV097_SET_TEXGEN_VIEW_MODEL_LOCAL_VIEWER. Sphere map vectors are computed using a
 *  local viewer model where the eye vector originates from (0,0,0,1) towards the vertex.
 *
 * @tc SphereMap_InfiniteViewer.png
 *  Tests SphereMap TexGen with NV097_SET_TEXGEN_VIEW_MODEL_INFINITE_VIEWER. Sphere map vectors are computed using
 *  an infinite parallel viewer model with a fixed direction along the +Z axis.
 */
TexgenTests::TexgenTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texgen", config) {
  for (auto mode : kTestModes) {
    for (const auto &vm : kViewModels) {
      std::string name = MakeTestName(mode, vm.name);
      uint32_t view_model = vm.value;
      tests_[name] = [this, mode, view_model, name]() { Test(mode, view_model, name); };
    }
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

void TexgenTests::Test(TextureStage::TexGen mode, uint32_t view_model, const std::string &test_name) {
  host_.PrepareDraw(0xFE202020);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_TEXGEN_VIEW_MODEL, view_model);
  Pushbuffer::End();

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTexgenS(mode);
  texture_stage.SetTexgenT(mode);
  if (mode == TextureStage::TG_SPHERE_MAP) {
    texture_stage.SetTexgenR(TextureStage::TG_DISABLE);
  } else {
    texture_stage.SetTexgenR(mode);
  }

  host_.SetupTextureStages();
  host_.DrawArrays();

  pb_print("M: %s", test_name.c_str());
  pb_draw_text_screen();

  FinishDraw(test_name);
}

std::string TexgenTests::MakeTestName(TextureStage::TexGen mode, const std::string &view_model_name) {
  std::string mode_name;
  switch (mode) {
    case TextureStage::TG_DISABLE:
      mode_name = "Disabled";
      break;

    case TextureStage::TG_EYE_LINEAR:
      mode_name = "EyeLinear";
      break;

    case TextureStage::TG_OBJECT_LINEAR:
      mode_name = "ObjectLinear";
      break;

    case TextureStage::TG_SPHERE_MAP:
      mode_name = "SphereMap";
      break;

    case TextureStage::TG_NORMAL_MAP:
      mode_name = "NormalMap";
      break;

    case TextureStage::TG_REFLECTION_MAP:
      mode_name = "ReflectionMap";
      break;
  }
  return view_model_name.empty() ? mode_name : mode_name + "_" + view_model_name;
}
