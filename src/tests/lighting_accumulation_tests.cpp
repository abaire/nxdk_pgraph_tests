#include "lighting_accumulation_tests.h"

#include <light.h>
#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "models/flat_mesh_grid_model.h"
#include "models/light_control_test_mesh_suzanne_model.h"
#include "pbkit_ext.h"
#include "shaders/perspective_vertex_shader.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"
#include "vertex_buffer.h"
#include "xbox_math_matrix.h"

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF909090;

static constexpr vector_t kSceneAmbientColor{0.f, 0.f, 0.f, 0.f};

static constexpr vector_t kLightAmbientColor{0.f, 0.1f, 0.f, 0.f};
static constexpr vector_t kLightDiffuseColor{0.2f, 0.f, 0.f, 0.f};
static constexpr vector_t kLightSpecularColor{0.f, 0.f, 0.2f, 0.f};

// Pointing into the screen.
static constexpr vector_t kDirectionalLightDir{0.f, 0.f, 1.f, 1.f};

static constexpr vector_t kPositionalLightPosition{0.f, 0.f, -2.f, 1.f};
static constexpr float kLightRange = 3.f;
static constexpr float kAttenuationConstant = 1.f;
static constexpr float kAttenuationLinear = 0.f;
static constexpr float kAttenuationQuadratic = 0.f;

static constexpr float kFalloffPenumbraDegrees = 45.f;
static constexpr float kFalloffUmbraDegrees = 10.f;
static constexpr float kFalloffParam0 = 0.f;
static constexpr float kFalloffParam1 = -0.494592f;
static constexpr float kFalloffParam2 = 1.494592f;

static constexpr char kDirectionalName[] = "Directional";
static constexpr char kPointName[] = "Point";
static constexpr char kSpotName[] = "Spot";
static constexpr char kMixed[] = "All";

static std::string MakeTestName(const std::string& prefix, uint32_t num_lights) {
  char buf[32];
  snprintf(buf, sizeof(buf), "-%d", num_lights);
  return prefix + buf;
}

/**
 */
LightingAccumulationTests::LightingAccumulationTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Lighting accumulation", config) {
  auto light_common_setup = [](std::shared_ptr<Light> light) {
    light->SetAmbient(kLightAmbientColor);
    light->SetDiffuse(kLightDiffuseColor);
    light->SetSpecular(kLightSpecularColor);
  };

  for (auto num_lights : {2, 4, 5}) {
    std::string name = MakeTestName(kDirectionalName, num_lights);
    tests_[name] = [this, light_common_setup, name, num_lights]() {
      std::vector<std::shared_ptr<Light>> lights;
      for (auto i = 0; i < num_lights; ++i) {
        auto light = std::make_shared<DirectionalLight>(i, kDirectionalLightDir);
        light_common_setup(light);
        lights.emplace_back(light);
      }

      this->Test(name, lights);
    };

    name = MakeTestName(kPointName, num_lights);
    tests_[name] = [this, light_common_setup, name, num_lights]() {
      std::vector<std::shared_ptr<Light>> lights;
      for (auto i = 0; i < num_lights; ++i) {
        auto light = std::make_shared<PointLight>(i, kPositionalLightPosition, kLightRange, kAttenuationConstant,
                                                  kAttenuationLinear, kAttenuationQuadratic);
        light_common_setup(light);
        lights.emplace_back(light);
      }
      this->Test(name, lights);
    };

    name = MakeTestName(kSpotName, num_lights);
    tests_[name] = [this, light_common_setup, name, num_lights]() {
      std::vector<std::shared_ptr<Light>> lights;
      for (auto i = 0; i < num_lights; ++i) {
        auto light = std::make_shared<Spotlight>(i, kPositionalLightPosition, kDirectionalLightDir, kLightRange,
                                                 kFalloffPenumbraDegrees, kFalloffUmbraDegrees, kAttenuationConstant,
                                                 kAttenuationLinear, kAttenuationQuadratic, kFalloffParam0,
                                                 kFalloffParam1, kFalloffParam2);
        light_common_setup(light);
        lights.emplace_back(light);
      }
      this->Test(name, lights);
    };
  }

  tests_[kMixed] = [this, light_common_setup] {
    std::vector<std::shared_ptr<Light>> lights;
    {
      auto light = std::make_shared<DirectionalLight>(0, kDirectionalLightDir);
      light_common_setup(light);
      lights.emplace_back(light);
    }
    {
      auto light = std::make_shared<PointLight>(1, kPositionalLightPosition, kLightRange, kAttenuationConstant,
                                                kAttenuationLinear, kAttenuationQuadratic);
      light_common_setup(light);
      lights.emplace_back(light);
    }
    {
      auto light = std::make_shared<Spotlight>(2, kPositionalLightPosition, kDirectionalLightDir, kLightRange,
                                               kFalloffPenumbraDegrees, kFalloffUmbraDegrees, kAttenuationConstant,
                                               kAttenuationLinear, kAttenuationQuadratic, kFalloffParam0,
                                               kFalloffParam1, kFalloffParam2);
      light_common_setup(light);
      lights.emplace_back(light);
    }
    this->Test(kMixed, lights);
  };
}

void LightingAccumulationTests::Deinitialize() { vertex_buffer_mesh_.reset(); }

void LightingAccumulationTests::CreateGeometry() {
  // SET_COLOR_MATERIAL below causes per-vertex diffuse color to be ignored entirely.
  vector_t diffuse{0.f, 0.f, 0.0f, 0.f};
  vector_t specular{1.f, 1.f, 1.f, 0.25f};

  // auto model = FlatMeshGridModel(diffuse, specular);
  auto model = LightControlTestMeshSuzanneModel(diffuse, specular);
  vertex_buffer_mesh_ = host_.AllocateVertexBuffer(model.GetVertexCount());
  model.PopulateVertexBuffer(vertex_buffer_mesh_);
}

void LightingAccumulationTests::Initialize() {
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

  host_.SetFinalCombiner0Just(TestHost::SRC_SPECULAR);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                          false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);

  {
    auto p = pb_begin();

    p = pb_push3fv(p, NV097_SET_SCENE_AMBIENT_COLOR, kSceneAmbientColor);

    p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
    p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
    p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.f);

    // Power 2.0
    p = pb_push3f(p, NV097_SET_SPECULAR_PARAMS + 0x00, -0.170208f, -0.855843f, 1.68563f);
    p = pb_push3f(p, NV097_SET_SPECULAR_PARAMS + 0x0c, -0.0f, -0.494592f, 1.49459f);

    pb_end(p);
  }
}

void LightingAccumulationTests::Test(const std::string& name, std::vector<std::shared_ptr<Light>> lights) {
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
  uint32_t light_mode_bitvector = 0;
  for (auto& light : lights) {
    light->Commit(host_, look_dir);
    light_mode_bitvector |= light->light_enable_mask();
  }

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, light_mode_bitvector);

    p = pb_push1(p, NV097_SET_LIGHT_CONTROL,
                 NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR | NV097_SET_LIGHT_CONTROL_V_ALPHA_FROM_MATERIAL_SPECULAR);

    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);

    pb_end(p);
  }

  host_.SetVertexBuffer(vertex_buffer_mesh_);
  host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
