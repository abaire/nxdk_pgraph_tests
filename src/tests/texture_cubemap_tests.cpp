#include "texture_cubemap_tests.h"

#include <SDL.h>
#include <SDL_image.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/perspective_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"

using namespace XboxMath;

static constexpr float kSize = 1.0f;

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

static constexpr const float kCubeSTPoints[][4][2] = {
  {{0.000000f, 0.000000f}, {0.000000f, 0.333333f}, {0.333333f, 0.333333f}, {0.333333f, 0.000000f}},
  {{0.333333f, 0.000000f}, {0.333333f, 0.333333f}, {0.666667f, 0.333333f}, {0.666667f, 0.000000f}},
  {{0.333333f, 0.333333f}, {0.333333f, 0.666667f}, {0.666667f, 0.666667f}, {0.666667f, 0.333333f}},
  {{0.000000f, 0.333333f}, {0.000000f, 0.666667f}, {0.333333f, 0.666667f}, {0.333333f, 0.333333f}},
  {{0.000000f, 0.666667f}, {0.000000f, 1.000000f}, {0.333333f, 1.000000f}, {0.333333f, 0.666667f}},
  {{0.666667f, 0.000000f}, {0.666667f, 0.333333f}, {1.000000f, 0.333333f}, {1.000000f, 0.000000f}},
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

TextureCubemapTests::TextureCubemapTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture cubemap", config) {
  tests_[kCubemapTest] = [this]() { TestCubemap(); };

  for (auto &test : kDotSTRTests) {
    tests_[test.name] = [this, &test] { TestDotSTRCubemap(test.name, test.dot_rgbmapping); };
  }
}

void TextureCubemapTests::Initialize() {
  TestSuite::Initialize();

  float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                          0.0f, depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  {
    shader->SetLightingEnabled(false);
    shader->SetUseD3DStyleViewport();
    vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
    vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);
  }
  host_.SetVertexShaderProgram(shader);

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  // Load the normal map into stage0
  {
    SDL_Surface *normal_map = IMG_Load("D:\\texture_cubemap\\cube_normals_object_space.png");
    ASSERT(normal_map && "Failed to load normal map");
    auto &stage = host_.GetTextureStage(0);
    stage.SetTextureDimensions(normal_map->w, normal_map->h);
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

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::End();

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);
}

void TextureCubemapTests::TestCubemap() {
  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                              TestHost::STAGE_CUBE_MAP);
  host_.SetupTextureStages();

  host_.PrepareDraw(0xFE121212);

  auto shader = std::static_pointer_cast<PerspectiveVertexShader>(host_.GetShaderProgram());

  auto draw = [this, &shader](float x, float y, float z, float r_x, float r_y, float r_z) {
    matrix4_t matrix;
    vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
    vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
    vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
    TestHost::BuildD3DModelViewMatrix(matrix, eye, at, up);

    auto &model_matrix = shader->GetModelMatrix();
    MatrixSetIdentity(model_matrix);
    vector_t rotation = {r_x, r_y, r_z};
    MatrixRotate(model_matrix, rotation);
    vector_t translation = {x, y, z};
    MatrixTranslate(model_matrix, translation);

    matrix4_t mv_matrix;
    MatrixMultMatrix(matrix, shader->GetModelMatrix(), mv_matrix);
    host_.SetFixedFunctionModelViewMatrix(mv_matrix);

    shader->PrepareDraw();

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    for (auto face : kCubeFaces) {
      for (auto i = 0; i < 4; ++i) {
        uint32_t index = face[i];
        const float *vertex = kCubePoints[index];
        host_.SetTexCoord3(vertex[0], vertex[1], vertex[2], 1.0f);
        host_.SetVertex(vertex[0], vertex[1], vertex[2], 1.0f);
      }
    }

    host_.End();
  };

  const float z = 2.0f;
  draw(-1.5f, 0.0f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  draw(1.5f, 0.0f, z, M_PI * 1.25f, M_PI * 0.25f, 0.0f);

  pb_print("%s\n", kCubemapTest);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kCubemapTest);
}

void TextureCubemapTests::TestDotSTRCubemap(const std::string &name, uint32_t dot_rgb_mapping) {
  /*
  // https://xboxdevwiki.net/NV2A/Pixel_Combiner seemingly erroneously indicates that PS_TEXTUREMODES_DOT_STR_CUBE
  should
  // perform a reflected lookup using texm3x3vspec. In practice, it seems that the w tex coordinate is ignored in this
  // test, so it is more likely that a texm3x3tex is the analog.

  // See https://web.archive.org/web/20210614122128/https://www.nvidia.com/docs/IO/8228/D3DTutorial2_FX_HLSL.pdf

  // texm3x3tex:
  // tex0 is used to look up the normal for a given pixel
  // The tex1, 2, and 3 combine contain the matrix needed to unproject a fragment back to the world-space point (used
  // to position the normal in worldspace)
  // The final value is a lookup from the cube map in t3 using the world adjusted normal.

  // texm3x3vspec:
  // tex0 is used to look up the normal for a given pixel
  // The tex1, 2, and 3 combine:
  //   1) The matrix needed to unproject a fragment back to the world-space point (used to position the normal in
  //      worldspace)
  //   2) The q component of the three are combined to provide the eye vector for the vertex (in worldspace).
  //
  // The hardware then creates a reflection vector using the normal and eye vectors and uses that to index into the
  // cubemap to find the final pixel value.
   */

  auto shader = std::static_pointer_cast<PerspectiveVertexShader>(host_.GetShaderProgram());

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_DOT_PRODUCT, TestHost::STAGE_DOT_PRODUCT,
                              TestHost::STAGE_DOT_STR_CUBE);
  host_.SetupTextureStages();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DOT_RGBMAPPING, dot_rgb_mapping);
  Pushbuffer::End();

  host_.PrepareDraw(0xFE131313);

  auto draw = [this, &shader](float x, float y, float z, float r_x, float r_y, float r_z) {
    matrix4_t matrix;
    vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
    vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
    vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
    TestHost::BuildD3DModelViewMatrix(matrix, eye, at, up);

    auto &model_matrix = shader->GetModelMatrix();
    MatrixSetIdentity(model_matrix);
    vector_t rotation = {r_x, r_y, r_z};
    MatrixRotate(model_matrix, rotation);
    vector_t translation = {x, y, z};
    MatrixTranslate(model_matrix, translation);

    matrix4_t mv_matrix;
    MatrixMultMatrix(model_matrix, matrix, mv_matrix);
    host_.SetFixedFunctionModelViewMatrix(mv_matrix);

    auto inv_projection = host_.GetFixedFunctionInverseCompositeMatrix();
    shader->PrepareDraw();

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    uint32_t face_index = 0;
    for (auto face : kCubeFaces) {
      const auto normals = kCubeSTPoints[face_index++];
      for (auto i = 0; i < 4; ++i) {
        uint32_t index = face[i];
        const float *vertex = kCubePoints[index];
        const float *normal_st = normals[i];
        // These would be necessary if the method actually used eye-relative reflection.
        //        vector_t padded_vertex = {vertex[0], vertex[1], vertex[2], 1.0f};
        //        vector_apply(padded_vertex, padded_vertex, model_matrix);
        //        padded_vertex[0] /= padded_vertex[3];
        //        padded_vertex[1] /= padded_vertex[3];
        //        padded_vertex[2] /= padded_vertex[3];
        //        padded_vertex[3] = 1.0f;
        //        vector_t eye_vec = {0.0f};
        //        vector_subtract(eye_vec, eye, padded_vertex);
        //        vector_normalize(eye_vec);

        host_.SetTexCoord0(normal_st[0], normal_st[1]);
        host_.SetTexCoord1(inv_projection[0][0], inv_projection[0][1], inv_projection[0][2], 1);
        host_.SetTexCoord2(inv_projection[1][0], inv_projection[1][1], inv_projection[1][2], 0);
        host_.SetTexCoord3(inv_projection[2][0], inv_projection[2][1], inv_projection[2][2], 0);
        host_.SetVertex(vertex[0], vertex[1], vertex[2], 1.0f);
      }
    }

    host_.End();
  };

  const float z = 2.0f;
  draw(-1.5f, 0.0f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  draw(1.5f, 0.0f, z, M_PI * 1.25f, M_PI * 0.25f, 0.0f);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DOT_RGBMAPPING, 0);
  Pushbuffer::End();

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
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
