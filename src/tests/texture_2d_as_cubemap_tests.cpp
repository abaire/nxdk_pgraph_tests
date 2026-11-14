#include "texture_2d_as_cubemap_tests.h"

#include <SDL.h>
#include <SDL_image.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "shaders/perspective_vertex_shader_no_lighting.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"
#include "xbox-swizzle/swizzle.h"
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
static constexpr char kTestCubemapZeroVector[] = "CubemapZeroVector_Bad2D";
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
 * @tc CubemapZeroVector_Bad2D
 *   Renders screen-space quads point sampling a 2D texture in cube map mode (PS_TEXTUREMODES_CUBEMAP) with a constant
 *   texture coordinate per quad. Every texel has a unique color that encodes its position: R = x * 4 + 2,
 *   G = y * 4 + 2, B = 128. The exception is texel (0, 0), which is pure magenta. The large quads use zero length
 *   directions: (0, 0, 0, 1), (0, 0, 0, 0), and (-0, -0, -0, 1). Below them, the same texture is point sampled in
 *   PS_TEXTUREMODES_PROJECT2D mode across a 192x192 quad (3x3 pixels per texel) with texture coordinates spanning
 *   (0, 0) to (1, 1), so that texel (0, 0) is shown as the magenta block at the top left.
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
  tests_[kTestCubemapZeroVector] = [this]() { TestCubemapZeroVector(); };
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

  host_.SetupTextureStages();
}

void Texture2DAsCubemapTests::LoadStage3Checkerboard() {
  // Load a 2D texture into stage3. All shader stages in these tests actually expect a cubemap.
  GenerateSwizzledRGBACheckerboard(host_.GetTextureMemoryForStage(3), 0, 0, kTextureWidth, kTextureHeight,
                                   kTexturePitch, 0xFF44FFFF, 0xFFAA9933, 2);
  auto &stage = host_.GetTextureStage(3);
  stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
  stage.SetCubemapEnable(false);
}

void Texture2DAsCubemapTests::Deinitialize() {
  TestSuite::Deinitialize();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DOT_RGBMAPPING, 0);
  Pushbuffer::End();
}

void Texture2DAsCubemapTests::TestCubemap() {
  LoadStage3Checkerboard();
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

  FinishDraw(kTestCubemap);
}

void Texture2DAsCubemapTests::TestCubemapZeroVector() {
  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                              TestHost::STAGE_CUBE_MAP);

  // Each texel encodes its position so that the exact texel sampled can be identified from the output:
  // R = x * 4 + 2, G = y * 4 + 2, B = 128.
  // Texel (0, 0) is instead pure magenta so that it is easily identified. R = 255 is never produced by the encoding
  // above, so this is unambiguous.
  {
    static_assert(kTextureWidth == 64 && kTextureHeight == 64, "Texel encoding assumes a 64x64 texture");
    auto linear = std::make_unique<uint32_t[]>(kTextureWidth * kTextureHeight);
    auto texel = linear.get();
    for (uint32_t y = 0; y < kTextureHeight; ++y) {
      for (uint32_t x = 0; x < kTextureWidth; ++x) {
        // A8B8G8R8
        *texel++ = 0xFF800000 + ((y * 4 + 2) << 8) + (x * 4 + 2);
      }
    }
    linear[0] = 0xFFFF00FF;
    swizzle_rect(reinterpret_cast<const uint8_t *>(linear.get()), kTextureWidth, kTextureHeight,
                 host_.GetTextureMemoryForStage(3), kTexturePitch, 4);
  }
  {
    auto &stage = host_.GetTextureStage(3);
    stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetCubemapEnable(false);
  }
  // Point sample so that the output is a single unfiltered texel.
  host_.GetTextureStage(3).SetFilter();
  host_.SetupTextureStages();

  auto original_shader = host_.GetShaderProgram();
  host_.SetVertexShaderProgram(std::make_shared<PassthroughVertexShader>());

  host_.PrepareDraw(0xFE303030);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX3);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX3, true);

  auto draw_quad = [this](const float *coord, float left, float top, float size) {
    static constexpr float kZ = 0.f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord3(coord[0], coord[1], coord[2], coord[3]);
    host_.SetVertex(left, top, kZ, 1.f);
    host_.SetTexCoord3(coord[0], coord[1], coord[2], coord[3]);
    host_.SetVertex(left + size, top, kZ, 1.f);
    host_.SetTexCoord3(coord[0], coord[1], coord[2], coord[3]);
    host_.SetVertex(left + size, top + size, kZ, 1.f);
    host_.SetTexCoord3(coord[0], coord[1], coord[2], coord[3]);
    host_.SetVertex(left, top + size, kZ, 1.f);
    host_.End();
  };

  // The quads under test.
  {
    struct ZeroVector {
      const char *label;
      float coord[4];
    };
    static constexpr ZeroVector kZeroVectors[] = {
        {"0,0,0,1", {0.f, 0.f, 0.f, 1.f}},
        {"0,0,0,0", {0.f, 0.f, 0.f, 0.f}},
        {"-0,-0,-0,1", {-0.f, -0.f, -0.f, 1.f}},
    };
    static constexpr float kQuadSize = 64.f;
    static constexpr float kQuadSpacing = 150.f;
    static constexpr float kTop = 100.f;
    static constexpr int kLabelRow = 6;

    float left = 30.f;
    for (auto &zero_vector : kZeroVectors) {
      draw_quad(zero_vector.coord, left, kTop, kQuadSize);
      pb_printat(kLabelRow, static_cast<int>((left - 20.f) / 10.f), "%s", zero_vector.label);
      left += kQuadSpacing;
    }
  }

  // The texture itself, sampled as a plain 2D texture so that the location of the magenta texel is visible.
  {
    host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE,
                                TestHost::STAGE_2D_PROJECTIVE);

    // 3 pixels per texel.
    static constexpr float kQuadSize = 192.f;
    static constexpr float kLeft = 20.f;
    static constexpr float kTop = 275.f;
    static constexpr float kZ = 0.f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord3(0.f, 0.f, 0.f, 1.f);
    host_.SetVertex(kLeft, kTop, kZ, 1.f);
    host_.SetTexCoord3(1.f, 0.f, 0.f, 1.f);
    host_.SetVertex(kLeft + kQuadSize, kTop, kZ, 1.f);
    host_.SetTexCoord3(1.f, 1.f, 0.f, 1.f);
    host_.SetVertex(kLeft + kQuadSize, kTop + kQuadSize, kZ, 1.f);
    host_.SetTexCoord3(0.f, 1.f, 0.f, 1.f);
    host_.SetVertex(kLeft, kTop + kQuadSize, kZ, 1.f);
    host_.End();
  }

  // Note: the pbkit text grid is 16 rows by 60 columns.
  pb_printat(0, 0, "%s", kTestCubemapZeroVector);
  pb_printat(2, 0, "Zero length directions, 2D texture");
  pb_printat(7, 0, "x=R/4 y=G/4");
  pb_printat(8, 0, "Texel (0,0) is magenta");
  pb_printat(9, 0, "Texture (texel 0,0 is top left)");
  pb_draw_text_screen();

  FinishDraw(kTestCubemapZeroVector);

  host_.SetVertexShaderProgram(original_shader);
}

void Texture2DAsCubemapTests::TestDotSTR3D(const std::string &name) {
  LoadStage3Checkerboard();
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

  FinishDraw(name);
}

void Texture2DAsCubemapTests::TestDotReflect(const std::string &name, ReflectTest mode) {
  LoadStage3Checkerboard();
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

  FinishDraw(name);
}

void Texture2DAsCubemapTests::TestDotSTRCubemap(const std::string &name) {
  LoadStage3Checkerboard();
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

  FinishDraw(name);
}
