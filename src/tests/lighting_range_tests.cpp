#include "lighting_range_tests.h"

#include <light.h>
#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "models/flat_mesh_grid_model.h"
#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "xbox_math_matrix.h"

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF909090;

static constexpr vector_t kSceneAmbientColor{0.031373f, 0.031373f, 0.031373f, 0.f};

static constexpr vector_t kLightAmbientColor{0.05f, 0.05f, 0.05f, 0.f};
static constexpr vector_t kLightDiffuseColor{1.f, 1.f, 0.f, 0.f};
static constexpr vector_t kLightSpecularColor{0.f, 0.f, 1.f, 0.f};

// Pointing into the screen.
static constexpr vector_t kDirectionalLightDir{0.f, 0.f, 1.f, 1.f};

static constexpr vector_t kPositionalLightPosition{0.f, 0.f, -7.f, 1.f};
static constexpr float kLightRange = 8.f;
// Force attentuation to be tiny so that range is the only value impacting whether a vertex is lit or not.
// It is possible to set attenuation to 0 (making the final term inf), but it results in a blue specular hue.
static constexpr float kAttenuationConstant = 0.0001f;
static constexpr float kAttenuationLinear = 0.f;
static constexpr float kAttenuationQuadratic = 0.f;

static constexpr float kFalloffPenumbraDegrees = 45.f;
static constexpr float kFalloffUmbraDegrees = 10.f;

static constexpr char kDirectionalName[] = "Directional";
static constexpr char kPointName[] = "Point";
static constexpr char kSpotName[] = "Spot";

static constexpr vector_t kVertexDiffuse{1.f, 0.5f, 1.f, 0.25f};
static constexpr vector_t kVertexSpecular{0.f, 0.5f, 1.f, 0.25f};

/**
 * @tc Directional
 *  Tests behavior using the fixed function pipeline with a directional (infinite) light.
 *
 * @tc Point
 *  Tests behavior using the fixed function pipeline with a point (local) light. Attenuation is set to 0 for the light,
 * resulting in hard edges.
 *
 * @tc Spot
 *  Tests behavior using the fixed function pipeline with a spotlight. Attenuation is set to 0 for the light, resulting
 * in hard edges.
 */
LightingRangeTests::LightingRangeTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Lighting range", config) {
  auto light_common_setup = [](std::shared_ptr<Light> light) {
    light->SetAmbient(kLightAmbientColor);
    light->SetDiffuse(kLightDiffuseColor);
    light->SetSpecular(kLightSpecularColor);
  };

  tests_[kDirectionalName] = [this, light_common_setup]() {
    auto light = std::make_shared<DirectionalLight>(0, kDirectionalLightDir);
    light_common_setup(light);
    Test(kDirectionalName, light);
  };

  tests_[kPointName] = [this, light_common_setup]() {
    auto light = std::make_shared<PointLight>(0, kPositionalLightPosition, kLightRange, kAttenuationConstant,
                                              kAttenuationLinear, kAttenuationQuadratic);
    light_common_setup(light);
    Test(kPointName, light);
  };

  tests_[kSpotName] = [this, light_common_setup]() {
    auto light = std::make_shared<Spotlight>(0, kPositionalLightPosition, kDirectionalLightDir, kLightRange,
                                             kFalloffPenumbraDegrees, kFalloffUmbraDegrees, kAttenuationConstant,
                                             kAttenuationLinear, kAttenuationQuadratic, 0.f, -0.494592f, 1.494592f);
    light_common_setup(light);
    Test(kSpotName, light);
  };
}

void LightingRangeTests::Deinitialize() { vertex_buffer_mesh_.reset(); }

void LightingRangeTests::CreateGeometry() {
  // SET_COLOR_MATERIAL below causes per-vertex diffuse color to be ignored entirely.
  vector_t diffuse{0.f, 0.f, 0.0f, 0.f};
  vector_t specular{1.f, 1.f, 1.f, 0.25f};

  auto model = FlatMeshGridModel(diffuse, specular);
  vertex_buffer_mesh_ = host_.AllocateVertexBuffer(model.GetVertexCount());
  matrix4_t transfomation;
  MatrixSetIdentity(transfomation);
  vector_t rot = {0.f, M_PI * -0.25f, 0.f, 0.f};
  MatrixRotate(transfomation, rot);
  model.PopulateVertexBuffer(vertex_buffer_mesh_, *transfomation);
}

void LightingRangeTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  host_.SetCombinerControl(1);
  host_.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT, TestHost::SRC_SPECULAR, false,
                              TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                              TestHost::MAP_UNSIGNED_INVERT);

  host_.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT, TestHost::SRC_SPECULAR, true,
                              TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                              TestHost::MAP_UNSIGNED_INVERT);

  host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);
  host_.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);

  host_.SetFinalCombiner0Just(TestHost::SRC_R0);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                          false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);

  {
    Pushbuffer::Begin();

    Pushbuffer::Push3F(NV097_SET_SCENE_AMBIENT_COLOR, kSceneAmbientColor);

    Pushbuffer::Push(NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
    Pushbuffer::Push(NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
    Pushbuffer::PushF(NV097_SET_MATERIAL_ALPHA, 0.75f);

    // Values taken from MechAssault
    Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x00, 0xBF56C33A);  // -0.838916
    Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x04, 0xC038C729);  // -2.887156
    Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x08, 0x4043165A);  // 3.048239
    Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x0c, 0xBF34DCE5);  // -0.706496
    Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x10, 0xC020743F);  // -2.507095
    Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x14, 0x40333D06);  // 2.800600

    Pushbuffer::End();
  }
}

void LightingRangeTests::Test(const std::string& name, std::shared_ptr<Light> light) {
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetVertexShaderProgram(nullptr);

  static constexpr uint32_t kBackgroundColor = 0xFF232623;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawCheckerboardUnproject(kCheckerboardA, kCheckerboardB, 24);

  vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
  vector_t at{0.0f, 0.0f, 0.0f, 1.0f};

  vector_t look_dir{0.f, 0.f, 0.f, 1.f};
  VectorSubtractVector(at, eye, look_dir);
  VectorNormalize(look_dir);
  light->Commit(host_, look_dir);

  auto light_mode_bitvector = light->light_enable_mask();

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_LIGHT_CONTROL, NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR |
                                                  NV097_SET_LIGHT_CONTROL_V_ALPHA_FROM_MATERIAL_SPECULAR);

    Pushbuffer::Push(NV097_SET_LIGHTING_ENABLE, true);
    Pushbuffer::Push(NV097_SET_SPECULAR_ENABLE, true);

    Pushbuffer::Push(NV097_SET_LIGHT_ENABLE_MASK, light_mode_bitvector);

    Pushbuffer::End();
  }

  host_.SetVertexBuffer(vertex_buffer_mesh_);
  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

  static constexpr float kQuadWidth = 128.f;
  static constexpr float kQuadHeight = 256.f;

  auto unproject = [this](vector_t& world_point, float x, float y, float z) {
    vector_t screen_point{x, y, z, 1.f};
    host_.UnprojectPoint(world_point, screen_point, z);
  };

  auto draw_quad = [this, unproject](float left, float top, float z) {
    const auto right = left + kQuadWidth;
    const auto bottom = top + kQuadHeight;

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    vector_t world_point{0.f, 0.f, 0.f, 1.f};

    host_.SetDiffuse(kVertexDiffuse);
    host_.SetSpecular(kVertexSpecular);
    host_.SetNormal(-0.099014754297667f, -0.099014754297667, -0.990147542976674f);
    host_.SetTexCoord0(0.f, 0.f);
    unproject(world_point, left, top, z);
    host_.SetVertex(world_point);

    host_.SetNormal(0.099014754297667f, -0.099014754297667, -0.990147542976674f);
    host_.SetTexCoord0(1.f, 0.f);
    unproject(world_point, right, top, z);
    host_.SetVertex(world_point);

    host_.SetNormal(0.099014754297667f, 0.099014754297667, -0.990147542976674f);
    host_.SetTexCoord0(1.f, 1.f);
    unproject(world_point, right, bottom, z);
    host_.SetVertex(world_point);

    host_.SetNormal(-0.099014754297667f, 0.099014754297667, -0.990147542976674f);
    host_.SetTexCoord0(0.f, 1.f);
    unproject(world_point, left, bottom, z);
    host_.SetVertex(world_point);

    host_.End();
  };

  draw_quad(24.f, 64.f, -2.f);
  draw_quad(host_.GetFramebufferWidthF() - (24.f + kQuadWidth), 64.f, 2.f);

  pb_print("%s\n", name.c_str());
  pb_printat(12, 4, "z=-2");
  pb_printat(12, 53, "z=2");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
