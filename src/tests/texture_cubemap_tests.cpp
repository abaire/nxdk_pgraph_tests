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

static constexpr char kCubemapTest[] = "Cubemap";

enum class CubemapGeneratorMode {
  kRadialGradient,
  kCheckerboard,
  kNoise,
};

static void GenerateCubemap(uint8_t *buffer, CubemapGeneratorMode mode);

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

static const DotProductMappedTest kDotReflectSpecTests[] = {
    {"0to1", 0x000}, {"-1to1D3D", 0x111}, {"-1to1GL", 0x222}, {"-1to1", 0x333}, {"HiLo_1", 0x444}, {"HiLoHemi", 0x777},
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

static const DotProductMappedTest kDotReflectSpecularConstTests[] = {
    {"DotReflectSpecConst_0to1", 0x000},
    {"DotReflectSpecConst_-1to1D3D", 0x111},
    {"DotReflectSpecConst_-1to1GL", 0x222},
    {"DotReflectSpecConst_-1to1", 0x333},
    {"DotReflectSpecConst_HiLo_1", 0x444},
    //    {"DotReflectSpecConst_HiLoHemiD3D", 0x555},  // Does not appear to be supported on HW (invalid data error)
    //    {"DotReflectSpecConst_HiLoHemiGL", 0x666},  // Does not appear to be supported on HW (invalid data error)
    {"DotReflectSpecConst_HiLoHemi", 0x777},
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
 * @tc Cubemap
 *   Renders two angles of a cube utilizing a cubemap texture.
 *
 * @tc DotReflectDiffuse_-1to1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_DIFF pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_minus1_to_1.
 *
 * @tc DotReflectDiffuse_-1to1D3D
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_DIFF pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_minus1_to_1_d3d.
 *
 * @tc DotReflectDiffuse_-1to1GL
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_DIFF pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_minus1_to_1_gl.
 *
 * @tc DotReflectDiffuse_0to1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_DIFF pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_zero_to_one.
 *
 * @tc DotReflectDiffuse_HiLo_1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_DIFF pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_hilo_1.
 *
 * @tc DotReflectDiffuse_HiLoHemi
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_DIFF pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_hilo_hemisphere.
 *
 * @tc DotReflectSpec_-1to1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_minus1_to_1.
 *
 * @tc DotReflectSpec_-1to1D3D
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_minus1_to_1_d3df.
 *
 * @tc DotReflectSpec_-1to1GL
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_minus1_to_1_glf.
 *
 * @tc DotReflectSpec_0to1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_zero_to_onef.
 *
 * @tc DotReflectSpec_HiLo_1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_hilo_1f.
 *
 * @tc DotReflectSpec_HiLoHemi
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC pixel shading mode. The NV097_DOT_RGBMAPPING
 *   is set to dotmap_hilo_hemispheref.
 *
 * @tc DotReflectSpecConst_-1to1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC_CONST pixel shading mode. The
 *   NV097_DOT_RGBMAPPING is set to dotmap_minus1_to_1.
 *
 * @tc DotReflectSpecConst_-1to1D3D
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC_CONST pixel shading mode. The
 *   NV097_DOT_RGBMAPPING is set to dotmap_minus1_to_1_d3d.
 *
 * @tc DotReflectSpecConst_-1to1GL
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC_CONST pixel shading mode. The
 *   NV097_DOT_RGBMAPPING is set to dotmap_minus1_to_1_gl.
 *
 * @tc DotReflectSpecConst_0to1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC_CONST pixel shading mode. The
 *   NV097_DOT_RGBMAPPING is set to dotmap_zero_to_one.
 *
 * @tc DotReflectSpecConst_HiLo_1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC_CONST pixel shading mode. The
 *   NV097_DOT_RGBMAPPING is set to dotmap_hilo_1.
 *
 * @tc DotReflectSpecConst_HiLoHemi
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_RFLCT_SPEC_CONST pixel shading mode. The
 *   NV097_DOT_RGBMAPPING is set to dotmap_hilo_hemisphere.
 *
 * @tc DotSTR3D_-1to1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_3D pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_minus1_to_1.
 *
 * @tc DotSTR3D_-1to1D3D
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_3D pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_minus1_to_1_d3d.
 *
 * @tc DotSTR3D_-1to1GL
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_3D pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_minus1_to_1_gl.
 *
 * @tc DotSTR3D_0to1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_3D pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_zero_to_one.
 *
 * @tc DotSTR3D_HiLo_1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_3D pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_hilo_1.
 *
 * @tc DotSTR3D_HiLoHemi
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_3D pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_hilo_hemisphere.
 *
 * @tc DotSTRCube_-1to1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_CUBE pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_minus1_to_1.
 *
 * @tc DotSTRCube_-1to1D3D
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_CUBE pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_minus1_to_1_d3d.
 *
 * @tc DotSTRCube_-1to1GL
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_CUBE pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_minus1_to_1_gl.
 *
 * @tc DotSTRCube_0to1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_CUBE pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_zero_to_one.
 *
 * @tc DotSTRCube_HiLo_1
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_CUBE pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_hilo_1.
 *
 * @tc DotSTRCube_HiLoHemi
 *   Renders two angles of a cube utilizing PS_TEXTUREMODES_DOT_STR_CUBE pixel shading mode. The NV097_DOT_RGBMAPPING is
 *   set to dotmap_hilo_hemisphere.
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
    tests_[test.name] = [this, &test] { TestDotReflect(test.name, test.dot_rgbmapping, ReflectTest::kSpecular); };
  }

  for (auto const_eye : {false, true}) {
    for (auto &test : kDotReflectSpecTests) {
      for (int eye_comp : {0, 1, 2}) {
        vector_t eye_vec = {0.0f, 0.0f, 0.0f};
        eye_vec[eye_comp] = 1.0f;
        std::string test_name = std::string(const_eye ? "DotReflectConst_" : "DotReflectSpec_") +
                                std::to_string(eye_comp) + "_" + test.name;
        tests_[test_name] = [this, test_name, &test, eye_vec, const_eye] {
          TestDotReflectSpec(test_name, test.dot_rgbmapping, eye_vec, const_eye);
        };
      }
    }
  }

  for (auto &test : kDotReflectSpecularConstTests) {
    tests_[test.name] = [this, &test] { TestDotReflect(test.name, test.dot_rgbmapping, ReflectTest::kSpecularConst); };
  }

  for (auto &test : kDotReflectDiffuseTests) {
    tests_[test.name] = [this, &test] { TestDotReflect(test.name, test.dot_rgbmapping, ReflectTest::kDiffuse); };
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
    GenerateCubemap(host_.GetTextureMemoryForStage(2), CubemapGeneratorMode::kCheckerboard);
    auto &stage = host_.GetTextureStage(2);
    stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    stage.SetCubemapEnable();
  }

  // Load the cube map into stage3. Tests are responsible for populating the texture.
  {
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

  // Use a simple checkerboard texture with a different color for each face of the cube.
  GenerateCubemap(host_.GetTextureMemoryForStage(3), CubemapGeneratorMode::kCheckerboard);
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

  GenerateCubemap(host_.GetTextureMemoryForStage(3), CubemapGeneratorMode::kNoise);

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

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DOT_RGBMAPPING, 0);
  Pushbuffer::End();

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void TextureCubemapTests::TestDotReflect(const std::string &name, uint32_t dot_rgb_mapping, ReflectTest mode) {
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

  GenerateCubemap(host_.GetTextureMemoryForStage(3), CubemapGeneratorMode::kRadialGradient);

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

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DOT_RGBMAPPING, 0);
  Pushbuffer::End();

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void TextureCubemapTests::TestDotSTRCubemap(const std::string &name, uint32_t dot_rgb_mapping) {
  // See https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/texm3x3vspec---ps
  //
  // texm3x3vspec:
  // tex0 is used to look up the normal for a given pixel
  // The tex1, 2, and 3 combine:
  //   1) The matrix needed to unproject a fragment back to the world-space point (used to position the normal in
  //      worldspace)
  //   2) The q component of the three are combined to provide the eye vector for the vertex (in worldspace).
  //
  // The hardware then creates a reflection vector using the normal and eye vectors and uses that to index into the
  // cubemap to find the final pixel value.

  GenerateCubemap(host_.GetTextureMemoryForStage(3), CubemapGeneratorMode::kRadialGradient);

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
  vector_t eye{0.0f, 0.0f, -7.f, 1.0f};
  vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
  vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
  TestHost::BuildD3DModelViewMatrix(model_view_matrix, eye, at, up);

  // Set an arbitrary but incorrect eye vector to confirm that it is not used by this mode.
  Pushbuffer::Begin();
  Pushbuffer::PushF(NV097_SET_EYE_VECTOR, -1.f, 10.f, 3.f);
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

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DOT_RGBMAPPING, 0);
  Pushbuffer::End();

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void TextureCubemapTests::TestDotReflectSpec(const std::string &name, uint32_t dot_rgb_mapping, const vector_t &eye_vec,
                                             bool const_eye) {
  auto shader = std::static_pointer_cast<PerspectiveVertexShader>(host_.GetShaderProgram());

  host_.SetTextureStageEnabled(0, true);
  host_.SetTextureStageEnabled(1, true);
  host_.SetTextureStageEnabled(2, true);
  host_.SetTextureStageEnabled(3, true);
  host_.SetShaderStageProgram(
      TestHost::STAGE_2D_PROJECTIVE, TestHost::STAGE_DOT_PRODUCT, TestHost::STAGE_DOT_PRODUCT,
      const_eye ? TestHost::STAGE_DOT_REFLECT_SPECULAR_CONST : TestHost::STAGE_DOT_REFLECT_SPECULAR);
  host_.SetupTextureStages();

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DOT_RGBMAPPING, dot_rgb_mapping);
  if (const_eye) {
    p = pb_push3f(p, NV097_SET_EYE_VECTOR, eye_vec[0], eye_vec[1], eye_vec[2]);
  } else {
    p = pb_push3f(p, NV097_SET_EYE_VECTOR, 0.0f, 0.0f, 0.0f);
  }
  pb_end(p);

  host_.PrepareDraw(0xFE131313);

  auto draw = [this, &shader, eye_vec, const_eye](float x, float y, float z, float r_x, float r_y, float r_z) {
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

    for (auto index : kCubeIndices) {
      const float *vertex = kCubeVertices[index];
      const float *normal_st = kCubeTextureCoords[index];

      host_.SetTexCoord0(normal_st[0], normal_st[1]);
      host_.SetTexCoord1(inv_projection[0][0], inv_projection[0][1], inv_projection[0][2],
                         const_eye ? 1.0f : eye_vec[0]);
      host_.SetTexCoord2(inv_projection[1][0], inv_projection[1][1], inv_projection[1][2],
                         const_eye ? 1.0f : eye_vec[1]);
      host_.SetTexCoord3(inv_projection[2][0], inv_projection[2][1], inv_projection[2][2],
                         const_eye ? 1.0f : eye_vec[2]);
      host_.SetVertex(vertex[0], vertex[1], vertex[2], 1.0f);
    }

    host_.End();
  };

  const float z = 2.0f;
  draw(-1.5f, 0.0f, z, M_PI * 0.25f, M_PI * 0.25f, 0.0f);
  draw(1.5f, 0.0f, z, M_PI * 1.25f, M_PI * 0.25f, 0.0f);

  p = pb_begin();
  p = pb_push1(p, NV097_SET_DOT_RGBMAPPING, 0);
  pb_end(p);

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

static void GenerateCubemap(uint8_t *buffer, CubemapGeneratorMode mode) {
  static constexpr uint32_t kSliceSize = kTexturePitch * kTextureHeight;

  static constexpr uint32_t kBoxSize = 4;
  static constexpr uint32_t kAlpha = 0xFF;

  static constexpr uint32_t kColorMasks[] = {0x0000FF, 0xFF00FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFFFF00};

  static constexpr uint32_t kCheckboxColors[6][2] = {
      {0xFF7777FF, 0xFF222222}, {0xFF553366, 0xFFDDDDDD}, {0xFF55FF55, 0xFF222222},
      {0xFF33AAAA, 0xFFDDDDDD}, {0xFFFF5555, 0xFF222222}, {0xFFAAAA33, 0xFFDDDDDD},
  };

  // +X, -X, +Y, -Y, +Z, -Z
  for (auto index = 0; index < 6; ++index) {
    switch (mode) {
      case CubemapGeneratorMode::kRadialGradient:
        GenerateSwizzledRGBRadialGradient(buffer, kTextureWidth, kTextureHeight, kColorMasks[index], kAlpha, false);
        break;
      case CubemapGeneratorMode::kCheckerboard:
        GenerateSwizzledRGBACheckerboard(buffer, 0, 0, kTextureWidth, kTextureHeight, kTexturePitch,
                                         kCheckboxColors[index][0], kCheckboxColors[index][1], kBoxSize);
        break;
      case CubemapGeneratorMode::kNoise:
        GenerateSwizzledRGBMaxContrastNoisePattern(buffer, kTextureWidth, kTextureHeight, kColorMasks[index]);
        break;
    }

    buffer += kSliceSize;
  }
}
