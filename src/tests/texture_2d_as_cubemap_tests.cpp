#include "texture_2d_as_cubemap_tests.h"

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

static constexpr uint32_t kCubeIndices[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
};

static constexpr float kCubeVertices[24][3] = {
    {-1.00f, -1.00f, 1.00f},  {1.00f, -1.00f, 1.00f},   {1.00f, 1.00f, 1.00f},   {-1.00f, 1.00f, 1.00f},
    {1.00f, -1.00f, -1.00f},  {-1.00f, -1.00f, -1.00f}, {-1.00f, 1.00f, -1.00f}, {1.00f, 1.00f, -1.00f},
    {1.00f, -1.00f, 1.00f},   {1.00f, -1.00f, -1.00f},  {1.00f, 1.00f, -1.00f},  {1.00f, 1.00f, 1.00f},
    {-1.00f, -1.00f, -1.00f}, {-1.00f, -1.00f, 1.00f},  {-1.00f, 1.00f, 1.00f},  {-1.00f, 1.00f, -1.00f},
    {-1.00f, 1.00f, 1.00f},   {1.00f, 1.00f, 1.00f},    {1.00f, 1.00f, -1.00f},  {-1.00f, 1.00f, -1.00f},
    {-1.00f, -1.00f, -1.00f}, {1.00f, -1.00f, -1.00f},  {1.00f, -1.00f, 1.00f},  {-1.00f, -1.00f, 1.00f},
};

static constexpr float kCubeTextureCoords[24][2] = {
    {0.2500f, 0.3750f}, {0.5000f, 0.3750f}, {0.5000f, 0.6250f}, {0.2500f, 0.6250f}, {0.7500f, 0.3750f},
    {1.0000f, 0.3750f}, {1.0000f, 0.6250f}, {0.7500f, 0.6250f}, {0.5000f, 0.3750f}, {0.7500f, 0.3750f},
    {0.7500f, 0.6250f}, {0.5000f, 0.6250f}, {0.0000f, 0.3750f}, {0.2500f, 0.3750f}, {0.2500f, 0.6250f},
    {0.0000f, 0.6250f}, {0.2500f, 0.6250f}, {0.5000f, 0.6250f}, {0.5000f, 0.8750f}, {0.2500f, 0.8750f},
    {0.2500f, 0.1250f}, {0.5000f, 0.1250f}, {0.5000f, 0.3750f}, {0.2500f, 0.3750f},
};

static constexpr uint32_t kTextureWidth = 64;
static constexpr uint32_t kTexturePitch = kTextureWidth * 4;
static constexpr uint32_t kTextureHeight = 64;

static constexpr uint32_t kDotRGBMapping = 0x111;
static constexpr char kTestCubemap[] = "Cubemap_Bad2D";
static constexpr char kTestDotSTRCube[] = "DotSTRCube_Bad2D";
static constexpr char kTestDotSTR3D[] = "DotSTR3D_Bad2D";
static constexpr char kTestDotReflectSpec[] = "DotReflectSpec_Bad2D";
static constexpr char kTestDotReflectSpecConst[] = "DotReflectSpecConst_Bad2D";
static constexpr char kTestDotReflectDiffuse[] = "DotReflectDiffuse_Bad2D";

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc Cubemap_Bad2D
 *   Renders two angles of a cube utilizing a cubemap texture.
 *
 * @tc DotReflectDiffuse_Bad2D
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_DIFF pixel shading mode but passing a 2d texture
 * instead of a cubemap for the final lookup.
 *
 * @tc DotReflectSpec_Bad2D
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC pixel shading mode but passing a 2d texture
 * instead of a cubemap for the final lookup.
 *
 * @tc DotReflectSpecConst_Bad2D
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC_CONST but passing a 2d texture instead of a
 * cubemap for the final lookup.
 *
 * @tc DotSTR3D_Bad2D
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_3D pixel shading mode but passing a 2d texture
 * instead of a cubemap for the final lookup.
 *
 * @tc DotSTRCube_Bad2D
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_CUBE pixel shading mode but passing a 2d texture
 * instead of a cubemap for the final lookup.
 */
Texture2DAsCubemapTests::Texture2DAsCubemapTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture 2D as cubemap", config) {
  tests_[kTestCubemap] = [this]() { TestCubemap(); };
  tests_[kTestDotSTRCube] = [this]() { TestDotSTRCubemap(kTestDotSTRCube); };
  tests_[kTestDotSTR3D] = [this]() { TestDotSTR3D(kTestDotSTR3D); };
  tests_[kTestDotReflectSpec] = [this]() { TestDotReflect(kTestDotReflectSpec, ReflectTest::kSpecular); };
  tests_[kTestDotReflectSpecConst] = [this] { TestDotReflect(kTestDotReflectSpecConst, ReflectTest::kSpecularConst); };
  tests_[kTestDotReflectDiffuse] = [this] { TestDotReflect(kTestDotReflectDiffuse, ReflectTest::kDiffuse); };
}

void Texture2DAsCubemapTests::Initialize() {
  TestSuite::Initialize();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DOT_RGBMAPPING, kDotRGBMapping);
  Pushbuffer::End();

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

  // Load a 2D diffuse texture into stage2 (only used by PS_TEXTUREMODES_DOT_RFLCT_DIFF tests). This should be a cubemap
  // instead.
  {
    GenerateSwizzledRGBRadialGradient(host_.GetTextureMemoryForStage(2), kTextureWidth, kTextureHeight);
    auto &stage = host_.GetTextureStage(2);
    stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetCubemapEnable(false);
  }

  // Load a 2D texture into stage3. All shader stages in these tests actually expect a cubemap.
  {
    GenerateSwizzledRGBACheckerboard(host_.GetTextureMemoryForStage(3), 0, 0, kTextureWidth, kTextureHeight,
                                     kTexturePitch, 0xFF44FFFF, 0xFFAA9933, 2);
    auto &stage = host_.GetTextureStage(3);
    stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetCubemapEnable(false);
  }

  host_.SetupTextureStages();
}

void Texture2DAsCubemapTests::Deinitialize() {
  TestSuite::Deinitialize();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DOT_RGBMAPPING, 0);
  Pushbuffer::End();
}

void Texture2DAsCubemapTests::TestCubemap() {
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

    for (auto index : kCubeIndices) {
      const float *vertex = kCubeVertices[index];
      host_.SetTexCoord3(vertex[0], vertex[1], vertex[2], 1.0f);
      host_.SetVertex(vertex[0], vertex[1], vertex[2], 1.0f);
    }

    host_.End();
  };

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);

  const float z = 2.0f;
  draw(-1.5f, 0.0f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  draw(1.5f, 0.0f, z, M_PI * 1.25f, M_PI * 0.25f, 0.0f);

  pb_print("%s\n", kTestCubemap);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestCubemap);
}

void Texture2DAsCubemapTests::TestDotSTR3D(const std::string &name) {
  auto shader = std::static_pointer_cast<PerspectiveVertexShader>(host_.GetShaderProgram());

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_DOT_PRODUCT, TestHost::STAGE_DOT_PRODUCT,
                              TestHost::STAGE_DOT_STR_3D);
  host_.SetupTextureStages();

  host_.PrepareDraw(0xFE141414);

  matrix4_t model_view_matrix;
  vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
  vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
  vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
  TestHost::BuildD3DModelViewMatrix(model_view_matrix, eye, at, up);

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

    for (auto index : kCubeIndices) {
      const float *vertex = kCubeVertices[index];
      const float *normal_st = kCubeTextureCoords[index];

      host_.SetTexCoord0(normal_st[0], normal_st[1]);
      host_.SetTexCoord1(inv_projection[0][0], inv_projection[0][1], inv_projection[0][2], 0.f);
      host_.SetTexCoord2(inv_projection[1][0], inv_projection[1][1], inv_projection[1][2], 0.f);
      host_.SetTexCoord3(inv_projection[2][0], inv_projection[2][1], inv_projection[2][2], 0.f);
      host_.SetVertex(vertex[0], vertex[1], vertex[2], 1.0f);
    }

    host_.End();
  };

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);

  const float z = 2.0f;
  draw(-1.5f, 0.0f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  draw(1.5f, 0.0f, z, M_PI * 1.25f, M_PI * 0.25f, 0.0f);

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void Texture2DAsCubemapTests::TestDotReflect(const std::string &name, ReflectTest mode) {
  auto shader = std::static_pointer_cast<PerspectiveVertexShader>(host_.GetShaderProgram());

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(
      TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_DOT_PRODUCT,
      mode == ReflectTest::kDiffuse ? TestHost::STAGE_DOT_REFLECT_DIFFUSE : TestHost::STAGE_DOT_PRODUCT,
      mode == ReflectTest::kSpecularConst ? TestHost::STAGE_DOT_REFLECT_SPECULAR_CONST
                                          : TestHost::STAGE_DOT_REFLECT_SPECULAR);
  host_.SetupTextureStages();

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

    for (auto index : kCubeIndices) {
      const float *vertex = kCubeVertices[index];
      const float *normal_st = kCubeTextureCoords[index];

      host_.SetTexCoord0(normal_st[0], normal_st[1]);
      host_.SetTexCoord1(inv_projection[0][0], inv_projection[0][1], inv_projection[0][2], 1.f);
      host_.SetTexCoord2(inv_projection[1][0], inv_projection[1][1], inv_projection[1][2], 1.f);
      host_.SetTexCoord3(inv_projection[2][0], inv_projection[2][1], inv_projection[2][2], 1.f);
      host_.SetVertex(vertex[0], vertex[1], vertex[2], 1.0f);
    }

    host_.End();
  };

  if (mode == ReflectTest::kDiffuse) {
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX2);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX2, true);
  } else {
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);
  }

  const float z = 2.0f;
  draw(-1.5f, 0.0f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  draw(1.5f, 0.0f, z, M_PI * 1.25f, M_PI * 0.25f, 0.0f);

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void Texture2DAsCubemapTests::TestDotSTRCubemap(const std::string &name) {
  auto shader = std::static_pointer_cast<PerspectiveVertexShader>(host_.GetShaderProgram());

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_DOT_PRODUCT, TestHost::STAGE_DOT_PRODUCT,
                              TestHost::STAGE_DOT_STR_CUBE);
  host_.SetupTextureStages();

  host_.PrepareDraw(0xFE131313);

  matrix4_t model_view_matrix;
  vector_t eye{0.0f, 0.0f, -7.f, 1.0f};
  vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
  vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
  TestHost::BuildD3DModelViewMatrix(model_view_matrix, eye, at, up);

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

    for (auto index : kCubeIndices) {
      const float *vertex = kCubeVertices[index];
      const float *normal_st = kCubeTextureCoords[index];

      vector_t padded_vertex = {vertex[0], vertex[1], vertex[2], 1.0f};
      VectorMultMatrix(padded_vertex, model_matrix);
      padded_vertex[0] /= padded_vertex[3];
      padded_vertex[1] /= padded_vertex[3];
      padded_vertex[2] /= padded_vertex[3];
      padded_vertex[3] = 1.0f;

      vector_t eye_vec = {0.0f};
      VectorSubtractVector(padded_vertex, eye, eye_vec);
      VectorNormalize(eye_vec);

      host_.SetTexCoord0(normal_st[0], normal_st[1]);
      host_.SetTexCoord1(inv_projection[0][0], inv_projection[0][1], inv_projection[0][2], eye_vec[0]);
      host_.SetTexCoord2(inv_projection[1][0], inv_projection[1][1], inv_projection[1][2], eye_vec[1]);
      host_.SetTexCoord3(inv_projection[2][0], inv_projection[2][1], inv_projection[2][2], eye_vec[2]);
      host_.SetVertex(vertex[0], vertex[1], vertex[2], 1.0f);
    }

    host_.End();
  };

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);

  const float z = 2.0f;
  draw(-1.5f, 0.0f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  draw(1.5f, 0.0f, z, M_PI * 1.25f, M_PI * 0.25f, 0.0f);

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
