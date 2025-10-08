#include "texture_3d_as_2d_tests.h"

#include <SDL.h>
#include <SDL_image.h>
#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "shaders/perspective_vertex_shader_no_lighting.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "xbox_math_types.h"

using namespace XboxMath;

static constexpr uint32_t kTextureWidth = 256;
static constexpr uint32_t kTextureHeight = 256;

static constexpr char kTestCubemap[] = "Cubemap_as_2D";
static constexpr char kTestVolumetric[] = "Volumetric_as_2D";

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc Cubemap_as_2D
 *   Renders a textured quad using a cubemap texture sampled as 2D_PROJECTIVE.
 *
 * @tc Volumetric_as_2D
 *   Renders a textured quad using a volumetric texture sampled as 2D_PROJECTIVE.
 */
Texture3DAs2DTests::Texture3DAs2DTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture 3D as 2D", config) {
  tests_[kTestCubemap] = [this]() { TestCubemap(); };
  tests_[kTestVolumetric] = [this]() { TestVolumetric(); };
}

void Texture3DAs2DTests::Initialize() {
  TestSuite::Initialize();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
}

void Texture3DAs2DTests::TestCubemap() {
  SDL_Surface *normal_map = IMG_Load("D:\\texture_cubemap\\cube_normals_object_space.png");
  ASSERT(normal_map && "Failed to load normal map");
  auto &stage = host_.GetTextureStage(0);
  stage.SetEnabled();
  stage.SetTextureDimensions(normal_map->w, normal_map->h);
  stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  stage.SetCubemapEnable();
  host_.SetTexture(normal_map);
  SDL_FreeSurface(normal_map);

  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetupTextureStages();

  host_.PrepareDraw(0xFE121212);

  float left = host_.CenterX(kTextureWidth);
  float top = host_.CenterY(kTextureHeight);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  host_.DrawSwizzledTexturedScreenQuad(left, top, left + kTextureWidth, top + kTextureHeight, 0.f);

  stage.SetEnabled(false);
  stage.SetCubemapEnable(false);
  host_.SetupTextureStages();

  pb_print("%s\n", kTestCubemap);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestCubemap);
}

void Texture3DAs2DTests::TestVolumetric() {
  auto texture_memory = host_.GetTextureMemoryForStage(0);
  GenerateSwizzledRGBACheckerboard(texture_memory, 0, 0, kTextureWidth, kTextureHeight, kTextureWidth * 4, 0xEFBBCC33,
                                   0x33449988);
  texture_memory += kTextureHeight * kTextureWidth * 4;
  GenerateSwizzledRGBATestPattern(texture_memory, kTextureWidth, kTextureHeight);

  auto &stage = host_.GetTextureStage(0);
  stage.SetEnabled();
  stage.SetImageDimensions(kTextureWidth, kTextureHeight, 2);
  stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  host_.SetupTextureStages();

  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.PrepareDraw(0xFE131313);

  float left = host_.CenterX(kTextureWidth);
  float top = host_.CenterY(kTextureHeight);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  host_.DrawSwizzledTexturedScreenQuad(left, top, left + kTextureWidth, top + kTextureHeight, 0.f);

  pb_print("%s\n", kTestVolumetric);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestVolumetric);
}
