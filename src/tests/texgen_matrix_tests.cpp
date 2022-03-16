#include "texgen_matrix_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "texture_format.h"
#include "vertex_buffer.h"

static int GenerateSurface(SDL_Surface **surface, int width, int height);

static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 128;

static TextureStage::TexGen kTestModes[] = {
    TextureStage::TG_DISABLE,
    TextureStage::TG_EYE_LINEAR,
    // TextureStage::TG_OBJECT_LINEAR, // xemu "untested" assert (generate_fixed_function) (works on HW)
    // TextureStage::TG_SPHERE_MAP,  // xemu "channel < 2" assert (kelvin_map_texgen) (works on HW)
    TextureStage::TG_NORMAL_MAP,
    TextureStage::TG_REFLECTION_MAP,
};

TexgenMatrixTests::TexgenMatrixTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texgen with texture matrix") {
  for (auto mode : kTestModes) {
    std::string name = MakeTestName(mode);
    tests_[name] = [this, mode]() { Test(mode); };
  }
}

void TexgenMatrixTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8G8B8A8));
  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetBorderColor(0xFF7F007F);
  texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
  texture_stage.SetTextureMatrixEnable(true);
  auto matrix = texture_stage.GetTextureMatrix();
  matrix_unit(matrix);
  matrix[_11] = 0.708939;
  matrix[_12] = 0.000000;
  matrix[_13] = 0.515000;
  matrix[_14] = 0.000000;
  matrix[_21] = 0.000000;
  matrix[_22] = 1.260336;
  matrix[_23] = 0.490000;
  matrix[_24] = 0.000000;
  matrix[_31] = 0.000000;
  matrix[_32] = 0.000000;
  matrix[_33] = 0.000000;
  matrix[_34] = 0.000000;
  matrix[_41] = 0.000000;
  matrix[_42] = 0.000000;
  matrix[_43] = 1.000000;
  matrix[_44] = 0.00000;

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

void TexgenMatrixTests::CreateGeometry() {
  static constexpr float left = -2.75f;
  static constexpr float right = 2.75f;
  static constexpr float top = 1.75f;
  static constexpr float bottom = -1.75f;

  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6);

  buffer->DefineBiTri(0, left, top, right, bottom);
}

void TexgenMatrixTests::Test(TextureStage::TexGen mode) {
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

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

std::string TexgenMatrixTests::MakeTestName(TextureStage::TexGen mode) {
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

static int GenerateSurface(SDL_Surface **surface, int width, int height) {
  *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  if (!(*surface)) {
    return 1;
  }

  if (SDL_LockSurface(*surface)) {
    SDL_FreeSurface(*surface);
    *surface = nullptr;
    return 2;
  }

  auto pixels = static_cast<uint32_t *>((*surface)->pixels);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x, ++pixels) {
      int x_normal = static_cast<int>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      int y_normal = static_cast<int>(static_cast<float>(y) * 255.0f / static_cast<float>(height));
      *pixels = SDL_MapRGBA((*surface)->format, y_normal, x_normal, 255 - y_normal, x_normal + y_normal);
    }
  }

  SDL_UnlockSurface(*surface);

  return 0;
}
