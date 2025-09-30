#include "texture_cubemap_tests.h"

#include <SDL.h>
#include <SDL_image.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/perspective_vertex_shader_no_lighting.h"
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

static void GenerateCubemap(uint8_t *buffer, uint32_t box_size = 4, bool use_alternate_colors = false);

struct DotProductMappedTest {
  const char *name;
  uint32_t dot_rgbmapping;
};

static const DotProductMappedTest kDotSTRCubeTests[] = {
    {"DotSTRCube_0to1", 0x000},
    {"DotSTRCube_-1to1D3D", 0x111},
    {"DotSTRCube_-1to1GL", 0x222},
    {"DotSTRCube_-1to1", 0x333},
    {"DotSTRCube_HiLo_1", 0x444},
    //    {"DotSTRCube_HiLoHemiD3D", 0x555},  // Does not appear to be supported on HW (invalid data error)
    //    {"DotSTRCube_HiLoHemiGL", 0x666},  // Does not appear to be supported on HW (invalid data error)
    {"DotSTRCube_HiLoHemi", 0x777},
};

static const DotProductMappedTest kDotSTRCube3DTests[] = {
    {"DotSTR3D_0to1", 0x000},
    {"DotSTR3D_-1to1D3D", 0x111},
    {"DotSTR3D_-1to1GL", 0x222},
    {"DotSTR3D_-1to1", 0x333},
    {"DotSTR3D_HiLo_1", 0x444},
    //    {"DotSTR3D_HiLoHemiD3D", 0x555},  // Does not appear to be supported on HW (invalid data error)
    //    {"DotSTR3D_HiLoHemiGL", 0x666},  // Does not appear to be supported on HW (invalid data error)
    {"DotSTR3D_HiLoHemi", 0x777},
};

static const DotProductMappedTest kDotReflectSpecularTests[] = {
    {"DotReflectSpec_0to1", 0x000},
    {"DotReflectSpec_-1to1D3D", 0x111},
    {"DotReflectSpec_-1to1GL", 0x222},
    {"DotReflectSpec_-1to1", 0x333},
    {"DotReflectSpec_HiLo_1", 0x444},
    //    {"DotReflectSpec_HiLoHemiD3D", 0x555},  // Does not appear to be supported on HW (invalid data error)
    //    {"DotReflectSpec_HiLoHemiGL", 0x666},  // Does not appear to be supported on HW (invalid data error)
    {"DotReflectSpec_HiLoHemi", 0x777},
};

static const DotProductMappedTest kDotReflectDiffuseTests[] = {
    {"DotReflectDiffuse_0to1", 0x000},
    {"DotReflectDiffuse_-1to1D3D", 0x111},
    {"DotReflectDiffuse_-1to1GL", 0x222},
    {"DotReflectDiffuse_-1to1", 0x333},
    {"DotReflectDiffuse_HiLo_1", 0x444},
    //    {"DotReflectDiffuse_HiLoHemiD3D", 0x555},  // Does not appear to be supported on HW (invalid data error)
    //    {"DotReflectDiffuse_HiLoHemiGL", 0x666},  // Does not appear to be supported on HW (invalid data error)
    {"DotReflectDiffuse_HiLoHemi", 0x777},
};

/**
 * Initializes the test suite and creates test cases.
 *
 */
TextureCubemapTests::TextureCubemapTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture cubemap", config) {
  tests_[kCubemapTest] = [this]() { TestCubemap(); };

  for (auto &test : kDotSTRCubeTests) {
    tests_[test.name] = [this, &test] { TestDotSTRCubemap(test.name, test.dot_rgbmapping); };
  }

  for (auto &test : kDotSTRCube3DTests) {
    tests_[test.name] = [this, &test] { TestDotSTR3D(test.name, test.dot_rgbmapping); };
  }

  for (auto &test : kDotReflectSpecularTests) {
    tests_[test.name] = [this, &test] { TestDotReflect(test.name, test.dot_rgbmapping); };
  }

  for (auto &test : kDotReflectDiffuseTests) {
    tests_[test.name] = [this, &test] { TestDotReflect(test.name, test.dot_rgbmapping, true); };
  }
}

void TextureCubemapTests::Initialize() {
  TestSuite::Initialize();

  float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
  auto shader =
      std::make_shared<PerspectiveVertexShaderNoLighting>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                          0.0f, depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  {
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
    stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    host_.SetTexture(normal_map);
    SDL_FreeSurface(normal_map);
  }

  // Load the diffuse map into stage2 (only used by PS_TEXTUREMODES_DOT_RFLCT_DIFF tests)
  {
    GenerateCubemap(host_.GetTextureMemoryForStage(2), 2, true);
    auto &stage = host_.GetTextureStage(2);
    stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetCubemapEnable();
  }

  // Load the cube map into stage3
  {
    GenerateCubemap(host_.GetTextureMemoryForStage(3));
    auto &stage = host_.GetTextureStage(3);
    stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetCubemapEnable();
  }
  host_.SetupTextureStages();

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

void TextureCubemapTests::TestDotSTR3D(const std::string &name, uint32_t dot_rgb_mapping) {
  /*
   * See https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/texm3x3tex---ps
   * texm3x3tex is used to orient a normal vector to the correct tangent space for the surface being rendered
   *
   * - tex0 is used to look up the normal for a given pixel
   * - The tex1, 2, and 3 combined contain the matrix needed to unproject a fragment back to the world-space point (used
       to position the normal in worldspace)
   * - The final value is a lookup from the cube map in t3 using the world adjusted normal.
  */

  auto shader = std::static_pointer_cast<PerspectiveVertexShader>(host_.GetShaderProgram());

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_DOT_PRODUCT, TestHost::STAGE_DOT_PRODUCT,
                              TestHost::STAGE_DOT_STR_3D);
  host_.SetupTextureStages();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DOT_RGBMAPPING, dot_rgb_mapping);
  Pushbuffer::End();

  host_.PrepareDraw(0xFE131313);

  matrix4_t model_view_matrix;
  vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
  vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
  vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
  TestHost::BuildD3DModelViewMatrix(model_view_matrix, eye, at, up);

  Pushbuffer::Begin();
  Pushbuffer::Push3F(NV097_SET_EYE_VECTOR, eye);
  Pushbuffer::End();

  auto draw = [this, &shader, model_view_matrix](float x, float y, float z, float r_x, float r_y, float r_z) {
    auto &model_matrix = shader->GetModelMatrix();
    MatrixSetIdentity(model_matrix);
    vector_t rotation = {r_x, r_y, r_z};
    MatrixRotate(model_matrix, rotation);
    vector_t translation = {x, y, z};
    MatrixTranslate(model_matrix, translation);

    matrix4_t mv_matrix;
    MatrixMultMatrix(model_matrix, model_view_matrix, mv_matrix);
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

        host_.SetTexCoord0(normal_st[0], normal_st[1]);
        host_.SetTexCoord1(inv_projection[0][0], inv_projection[0][1], inv_projection[0][2], 1.f);
        host_.SetTexCoord2(inv_projection[1][0], inv_projection[1][1], inv_projection[1][2], 1.f);
        host_.SetTexCoord3(inv_projection[2][0], inv_projection[2][1], inv_projection[2][2], 1.f);
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

void TextureCubemapTests::TestDotReflect(const std::string &name, uint32_t dot_rgb_mapping, bool reflect_diffuse) {
  /*
   * See https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/texm3x3spec---ps
   * texm3x3spec is used for specular reflection and environment mapping.
   *
   * - tex0 is used to look up the normal for a given pixel
   * - The tex1, 2, and 3 combined contain the matrix needed to unproject a fragment back to the world-space point (used
       to position the normal in worldspace)
   * - The final value is a lookup from the cube map in t3 using the world adjusted normal.
   * - The eye-ray vector is provided via NV097_SET_EYE_VECTOR.
  */

  auto shader = std::static_pointer_cast<PerspectiveVertexShader>(host_.GetShaderProgram());

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_DOT_PRODUCT,
                              reflect_diffuse ? TestHost::STAGE_DOT_REFLECT_DIFFUSE : TestHost::STAGE_DOT_PRODUCT,
                              TestHost::STAGE_DOT_REFLECT_SPECULAR);
  host_.SetupTextureStages();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DOT_RGBMAPPING, dot_rgb_mapping);
  Pushbuffer::End();

  host_.PrepareDraw(0xFE131313);

  matrix4_t model_view_matrix;
  vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
  vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
  vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
  TestHost::BuildD3DModelViewMatrix(model_view_matrix, eye, at, up);

  Pushbuffer::Begin();
  Pushbuffer::Push3F(NV097_SET_EYE_VECTOR, eye);
  Pushbuffer::End();

  auto draw = [this, &shader, model_view_matrix](float x, float y, float z, float r_x, float r_y, float r_z) {
    auto &model_matrix = shader->GetModelMatrix();
    MatrixSetIdentity(model_matrix);
    vector_t rotation = {r_x, r_y, r_z};
    MatrixRotate(model_matrix, rotation);
    vector_t translation = {x, y, z};
    MatrixTranslate(model_matrix, translation);

    matrix4_t mv_matrix;
    MatrixMultMatrix(model_matrix, model_view_matrix, mv_matrix);
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

        host_.SetTexCoord0(normal_st[0], normal_st[1]);
        host_.SetTexCoord1(inv_projection[0][0], inv_projection[0][1], inv_projection[0][2], 1.f);
        host_.SetTexCoord2(inv_projection[1][0], inv_projection[1][1], inv_projection[1][2], 1.f);
        host_.SetTexCoord3(inv_projection[2][0], inv_projection[2][1], inv_projection[2][2], 1.f);
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

void TextureCubemapTests::TestDotSTRCubemap(const std::string &name, uint32_t dot_rgb_mapping) {
  // https://xboxdevwiki.net/NV2A/Pixel_Combiner indicates that PS_TEXTUREMODES_DOT_STR_CUBE should
  // perform a reflected lookup using texm3x3vspec.
  // In practice, it seems that the w tex coordinate is ignored in this test and this is analogous to teerm3x3spec with
  // the eye coordinate taken from NV097_SET_EYE_VECTOR.

  // See https://web.archive.org/web/20240629060504/https://www.nvidia.com/docs/IO/8228/D3DTutorial2_FX_HLSL.pdf
  // See https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/texm3x3spec---ps
  // See https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/texm3x3vspec---ps

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

  matrix4_t model_view_matrix;
  vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
  vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
  vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
  TestHost::BuildD3DModelViewMatrix(model_view_matrix, eye, at, up);
  Pushbuffer::Begin();
  Pushbuffer::Push3F(NV097_SET_EYE_VECTOR, eye);
  Pushbuffer::End();

  auto draw = [this, &shader, model_view_matrix, eye](float x, float y, float z, float r_x, float r_y, float r_z) {
    auto &model_matrix = shader->GetModelMatrix();
    MatrixSetIdentity(model_matrix);
    vector_t rotation = {r_x, r_y, r_z};
    MatrixRotate(model_matrix, rotation);
    vector_t translation = {x, y, z};
    MatrixTranslate(model_matrix, translation);

    matrix4_t mv_matrix;
    MatrixMultMatrix(model_matrix, model_view_matrix, mv_matrix);
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

        vector_t padded_vertex = {vertex[0], vertex[1], vertex[2], 1.0f};
        VectorMultMatrix(padded_vertex, model_matrix);
        padded_vertex[0] /= padded_vertex[3];
        padded_vertex[1] /= padded_vertex[3];
        padded_vertex[2] /= padded_vertex[3];
        padded_vertex[3] = 1.0f;

        vector_t eye_vec = {0.0f};
        VectorSubtractVector(eye, padded_vertex, eye_vec);
        VectorNormalize(eye_vec);

        host_.SetTexCoord0(normal_st[0], normal_st[1]);
        host_.SetTexCoord1(inv_projection[0][0], inv_projection[0][1], inv_projection[0][2], eye_vec[0]);
        host_.SetTexCoord2(inv_projection[1][0], inv_projection[1][1], inv_projection[1][2], eye_vec[1]);
        host_.SetTexCoord3(inv_projection[2][0], inv_projection[2][1], inv_projection[2][2], eye_vec[2]);
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

static void GenerateCubemap(uint8_t *buffer, uint32_t box_size, bool use_alternate_colors) {
  static constexpr uint32_t kSliceSize = kTexturePitch * kTextureHeight;

  static constexpr uint32_t kStandardColors[6][2] = {
      {0xFF5555FF, 0xFF666666}, {0xFF3333AA, 0xFF444444}, {0xFF55FF55, 0xFF666666},
      {0xFF33AA33, 0xFF444444}, {0xFFFF5555, 0xFF666666}, {0xFFAA3333, 0xFF444444},
  };

  static constexpr uint32_t kAlternateColors[6][2] = {
      {0xFFCCCCCC, 0xFF00007F}, {0xFFAAAAAA, 0xFF7F007F}, {0xFFCCCCCC, 0xFF007F00},
      {0xFFAAAAAA, 0xFF007F7F}, {0xFFCCCCCC, 0xFF7F0000}, {0xFFAAAAAA, 0xFF7F7F00},
  };

  auto color_set = use_alternate_colors ? kAlternateColors : kStandardColors;

  // +X
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, color_set[0][0],
                                   color_set[0][1], box_size);
  buffer += kSliceSize;

  // -X
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, color_set[1][0],
                                   color_set[1][1], box_size);
  buffer += kSliceSize;

  // +Y
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, color_set[2][0],
                                   color_set[2][1], box_size);
  buffer += kSliceSize;

  // -Y
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, color_set[3][0],
                                   color_set[3][1], box_size);
  buffer += kSliceSize;

  // +Z
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, color_set[4][0],
                                   color_set[4][1], box_size);
  buffer += kSliceSize;

  // -Z
  GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch, color_set[5][0],
                                   color_set[5][1], box_size);
}
