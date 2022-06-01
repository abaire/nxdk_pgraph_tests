#include "texture_cubemap_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "math3d.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"

static constexpr float kSize = 0.75f;
static constexpr uint32_t kRightSide[] = {3, 7, 6, 2};
static constexpr uint32_t kLeftSide[] = {1, 5, 4, 0};
static constexpr uint32_t kTopSide[] = {4, 5, 6, 7};
static constexpr uint32_t kBottomSide[] = {0, 3, 2, 1};
static constexpr uint32_t kBackSide[] = {7, 6, 5, 4};
static constexpr uint32_t kFrontSide[] = {0, 4, 7, 3};
static constexpr const uint32_t *kCubeFaces[] = {
    kRightSide, kLeftSide, kTopSide, kBottomSide, kBackSide, kFrontSide,
};
static constexpr float kCubePoints[][3] = {
    {-kSize, -kSize, -kSize}, {-kSize, -kSize, kSize}, {kSize, -kSize, kSize}, {kSize, -kSize, -kSize},
    {-kSize, kSize, -kSize},  {-kSize, kSize, kSize},  {kSize, kSize, kSize},  {kSize, kSize, -kSize},
};

static constexpr uint32_t kTextureWidth = 64;
static constexpr uint32_t kTexturePitch = kTextureWidth * 4;
static constexpr uint32_t kTextureHeight = 64;
static constexpr const char kCubemapTest[] = "Cubemap";

static void GenerateCubemap(uint8_t *buffer);

TextureCubemapTests::TextureCubemapTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture cubemap") {
  tests_[kCubemapTest] = [this]() { TestCubemap(); };
}

void TextureCubemapTests::Initialize() {
  TestSuite::Initialize();

  GenerateCubemap(host_.GetTextureMemory());

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  host_.SetTextureStageEnabled(0, true);
  auto &stage = host_.GetTextureStage(0);
  stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
  stage.SetCubemapEnable();
  host_.SetupTextureStages();
  host_.SetShaderStageProgram(TestHost::STAGE_CUBE_MAP);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
  pb_end(p);
}

void TextureCubemapTests::TestCubemap() {
  host_.PrepareDraw(0xFE121212);

  auto draw = [this](float x, float y, float z, float r_x, float r_y, float r_z) {
    MATRIX matrix = {0.0f};
    VECTOR eye{0.0f, 0.0f, -7.0f, 1.0f};
    VECTOR at{0.0f, 0.0f, 0.0f, 1.0f};
    VECTOR up{0.0f, 1.0f, 0.0f, 1.0f};
    host_.GetD3DModelViewMatrix(matrix, eye, at, up);

    MATRIX model_matrix = {0.0f};
    matrix_unit(model_matrix);
    VECTOR rotation = {r_x, r_y, r_z};
    matrix_rotate(model_matrix, model_matrix, rotation);
    VECTOR translation = {x, y, z};
    matrix_translate(model_matrix, model_matrix, translation);

    matrix_multiply(matrix, model_matrix, matrix);
    host_.SetFixedFunctionModelViewMatrix(matrix);

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    for (auto face : kCubeFaces) {
      for (auto i = 0; i < 4; ++i) {
        uint32_t index = face[i];
        const float *vertex = kCubePoints[index];
        host_.SetTexCoord0(vertex[0], vertex[1], vertex[2], 1.0f);
        host_.SetVertex(vertex[0], vertex[1], vertex[2], 1.0f);
      }
    }

    host_.End();
  };

  const float z = 2.0f;
  draw(-1.5f, 0.0f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  draw(1.5f, 0.0f, z, M_PI * -0.25, M_PI * -0.25f, 0.0f);

  pb_print("%s\n", kCubemapTest);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kCubemapTest);
}

static void GenerateCubemap(uint8_t *buffer) {
  static constexpr uint32_t kSliceSize = kTexturePitch * kTextureHeight;
  // +X
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, 0xFF5555FF, 0xFF666666,
                                   4);
  buffer += kSliceSize;

  // -X
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, 0xFF3333AA, 0xFF444444,
                                   4);
  buffer += kSliceSize;

  // +Y
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, 0xFF55FF55, 0xFF666666,
                                   4);
  buffer += kSliceSize;

  // -Y
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, 0xFF33AA33, 0xFF444444,
                                   4);
  buffer += kSliceSize;

  // +Z
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, 0xFFFF5555, 0xFF666666,
                                   4);
  buffer += kSliceSize;

  // -Z
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, 0xFFAA3333, 0xFF444444,
                                   4);
}
