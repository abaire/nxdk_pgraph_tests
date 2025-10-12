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

static constexpr uint32_t kTextureWidth = 64;
static constexpr uint32_t kTextureHeight = 64;
static constexpr uint32_t kTexturePitch = kTextureWidth * 4;

static constexpr uint32_t kGeometryWidth = kTextureWidth * 4;
static constexpr uint32_t kGeometryHeight = kTextureHeight * 4;

static constexpr char kTestCubemap[] = "Cubemap_as_2D";
static constexpr char kTestVolumetric[] = "Volumetric_as_2D";

static void GenerateCubemap(uint8_t *buffer);

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc Cubemap_as_2D
 *   Renders a textured quad using a cubemap texture sampled as 2D_PROJECTIVE. The +X face is rendered as a native-sized
 *   reference image. The quad is rendered at 4x the native size of the texture.
 *
 * @tc Volumetric_as_2D
 *   Renders a textured quad using a volumetric texture sampled as 2D_PROJECTIVE. The first two layers are rendered as
 *   native sized reference images on the left. The quad is rendered at 4x the native size of the texture.
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
  GenerateCubemap(host_.GetTextureMemoryForStage(0));

  auto &stage = host_.GetTextureStage(0);
  stage.SetEnabled();
  stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
  stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  stage.SetCubemapEnable();

  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetupTextureStages();

  host_.PrepareDraw(0xFE121212);

  float left = host_.CenterX(kGeometryWidth);
  float top = host_.CenterY(kGeometryHeight);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  host_.DrawSwizzledTexturedScreenQuad(left, top, left + kGeometryWidth, top + kGeometryHeight, 0.f);

  // Render a smaller version of the primary face as a reference
  stage.SetCubemapEnable(false);
  host_.SetupTextureStages();

  left = 32.f;
  top = host_.CenterY(kTextureHeight);
  host_.DrawSwizzledTexturedScreenQuad(left, top, left + kTextureWidth, top + kTextureHeight, 0.f);

  stage.SetEnabled(false);
  stage.SetCubemapEnable(false);
  host_.SetupTextureStages();

  pb_print("%s\n", kTestCubemap);
  pb_printat(6, 0, "Reference");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestCubemap);
}

void Texture3DAs2DTests::TestVolumetric() {
  auto texture_memory = host_.GetTextureMemoryForStage(0);
  GenerateSwizzledRGBACheckerboard(texture_memory, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, 0xEFBBCC33,
                                   0x33449988);
  texture_memory += kTextureHeight * kTexturePitch;
  GenerateSwizzledRGBATestPattern(texture_memory, kTextureWidth, kTextureHeight);

  auto &stage = host_.GetTextureStage(0);
  stage.SetEnabled();
  stage.SetTextureDimensions(kTextureWidth, kTextureHeight, 2);
  stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  host_.SetupTextureStages();

  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  host_.PrepareDraw(0xFE131313);

  float left = host_.CenterX(kGeometryWidth);
  float top = host_.CenterY(kGeometryHeight);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  // Clear out the STQR coordinates
  host_.SetTexCoord0(0.f, 0.f, 0.f, 0.f);
  host_.DrawSwizzledTexturedScreenQuad(left, top, left + kGeometryWidth, top + kGeometryHeight, 0.f);

  stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
  host_.SetupTextureStages();

  left = 32.f;
  top = host_.CenterY(kTextureHeight * 3);
  host_.DrawSwizzledTexturedScreenQuad(left, top, left + kTextureWidth, top + kTextureHeight, 0.f);

  TestHost::PBKitBusyWait();
  memcpy(host_.GetTextureMemoryForStage(0), texture_memory, kTextureHeight * kTexturePitch);
  top += kTextureHeight * 2.f;
  host_.DrawSwizzledTexturedScreenQuad(left, top, left + kTextureWidth, top + kTextureHeight, 0.f);

  pb_print("%s\n", kTestVolumetric);
  pb_printat(3, 0, "Reference");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestVolumetric);
}

static void GenerateCubemap(uint8_t *buffer) {
  static constexpr uint32_t kSliceSize = kTextureHeight * kTexturePitch;

  // Generate very distinct faces in order +X, -X, +Y, -Y, +Z, -Z
  GenerateSwizzledRGBTestPattern(buffer, kTextureWidth, kTextureHeight);
  buffer += kSliceSize;

  GenerateSwizzledRGBRadialGradient(buffer, kTextureWidth, kTextureHeight, 0x00FFFF, 0xFF, false);
  buffer += kSliceSize;

  GenerateSwizzledRGBMaxContrastNoisePattern(buffer, kTextureWidth, kTextureHeight, 0xFF00FF);
  buffer += kSliceSize;

  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, 0xFFFFFF00, 0xFF00FFFF);
  buffer += kSliceSize;

  GenerateSwizzledRGBMaxContrastNoisePattern(buffer, kTextureWidth, kTextureHeight, 0x0000FF);
  buffer += kSliceSize;

  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, 0xFF777777, 0xFF000000);
}
