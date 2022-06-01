#include "texture_cubemap_tests.h"

#include <SDL.h>
#include <SDL_image.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "math3d.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"

static constexpr float kSize = 0.75f;

// Must be ordered to match kCubeSTPoints for each face.
static constexpr uint32_t kRightSide[] = {3, 7, 6, 2};
static constexpr uint32_t kLeftSide[] = {1, 5, 4, 0};
static constexpr uint32_t kTopSide[] = {4, 5, 6, 7};
static constexpr uint32_t kBottomSide[] = {1, 0, 3, 2};
static constexpr uint32_t kBackSide[] = {2, 6, 5, 1};
static constexpr uint32_t kFrontSide[] = {0, 4, 7, 3};

// clang-format off
static constexpr const uint32_t *kCubeFaces[] = {
  kRightSide,
  kLeftSide,
  kTopSide,
  kBottomSide,
  kBackSide,
  kFrontSide,
};

static constexpr float kCubePoints[][3] = {
  {-kSize, -kSize, -kSize},
  {-kSize, -kSize, kSize},
  {kSize, -kSize, kSize},
  {kSize, -kSize, -kSize},
  {-kSize, kSize, -kSize},
  {-kSize, kSize, kSize},
  {kSize, kSize, kSize},
  {kSize, kSize, -kSize},
};

static constexpr float kCubeSTPoints[][2] = {
  {0.0f, 0.0f},
  {0.0f, 1.0f},
  {1.0f, 1.0f},
  {1.0f, 0.0f},
};

// clang-format on

static constexpr uint32_t kTextureWidth = 64;
static constexpr uint32_t kTexturePitch = kTextureWidth * 4;
static constexpr uint32_t kTextureHeight = 64;

static constexpr const char kCubemapTest[] = "Cubemap";

static void GenerateCubemap(uint8_t *buffer);

struct DotSTRTest {
  const char *name;
  uint32_t dot_rgbmapping;
};

static const DotSTRTest kDotSTRTests[] = {
    {"DotSTRCube_0to1", 0x000},
    {"DotSTRCube_-1to1D3D", 0x111},
    {"DotSTRCube_-1to1GL", 0x222},
    {"DotSTRCube_-1to1", 0x333},
    {"DotSTRCube_HiLo_1", 0x444},
    //    {"DotSTRCube_HiLoHemiD3D", 0x555},  // Does not appear to be supported on HW (invalid data error)
    //    {"DotSTRCube_HiLoHemiGL", 0x666},  // Does not appear to be supported on HW (invalid data error)
    {"DotSTRCube_HiLoHemi", 0x777},
};

TextureCubemapTests::TextureCubemapTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture cubemap") {
  tests_[kCubemapTest] = [this]() { TestCubemap(); };

  for (auto &test : kDotSTRTests) {
    tests_[test.name] = [this, &test] { TestDotSTRCubemap(test.name, test.dot_rgbmapping); };
  }
}

void TextureCubemapTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  // Load the normal map into stage0
  {
    SDL_Surface *normal_map = IMG_Load("D:\\texture_cubemap\\WaveNormalMap.png");
    ASSERT(normal_map && "Failed to load normal map");
    auto &stage = host_.GetTextureStage(0);
    stage.SetTextureDimensions(128, 128);
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8), 0);
    host_.SetTexture(normal_map);
    SDL_free(normal_map);
  }

  // Load the cube map into stage3
  {
    GenerateCubemap(host_.GetTextureMemoryForStage(3));
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8), 3);
    auto &stage = host_.GetTextureStage(3);
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetCubemapEnable();
    host_.SetupTextureStages();
  }

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
  pb_end(p);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);
}

void TextureCubemapTests::TestCubemap() {
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                              TestHost::STAGE_CUBE_MAP);
  host_.SetupTextureStages();

  host_.PrepareDraw(0xFE121212);

  const float z = 2.0f;
  Draw(-1.5f, 0.0f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  Draw(1.5f, 0.0f, z, M_PI * -0.25, M_PI * -0.25f, 0.0f);

  pb_print("%s\n", kCubemapTest);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kCubemapTest);
}

void TextureCubemapTests::TestDotSTRCubemap(const std::string &name, uint32_t dot_rgb_mapping) {
  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_DOT_PRODUCT, TestHost::STAGE_DOT_PRODUCT,
                              TestHost::STAGE_DOT_STR_CUBE);
  host_.SetupTextureStages();

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DOT_RGBMAPPING, dot_rgb_mapping);
  pb_end(p);

  host_.PrepareDraw(0xFE131313);

  const float z = 2.0f;
  Draw(-1.5f, 0.0f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  Draw(1.5f, 0.0f, z, M_PI * -0.25, M_PI * -0.25f, 0.0f);

  p = pb_begin();
  p = pb_push1(p, NV097_SET_DOT_RGBMAPPING, 0);
  pb_end(p);

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

void TextureCubemapTests::Draw(float x, float y, float z, float r_x, float r_y, float r_z) const {
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
      const float *normal_st = kCubeSTPoints[i];
      host_.SetTexCoord0(normal_st[0], normal_st[1]);
      host_.SetTexCoord1(vertex[0], vertex[1], vertex[2], 1.0f);
      host_.SetTexCoord2(vertex[0], vertex[1], vertex[2], 1.0f);
      host_.SetTexCoord3(vertex[0], vertex[1], vertex[2], 1.0f);
      host_.SetVertex(vertex[0], vertex[1], vertex[2], 1.0f);
    }
  }

  host_.End();
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
