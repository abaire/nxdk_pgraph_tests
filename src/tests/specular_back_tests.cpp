#include "specular_back_tests.h"

#include <light.h>
#include <models/light_control_test_mesh_cone_model.h>
#include <models/light_control_test_mesh_cylinder_model.h>
#include <models/light_control_test_mesh_sphere_model.h>
#include <models/light_control_test_mesh_suzanne_model.h>
#include <models/light_control_test_mesh_torus_model.h>
#include <pbkit/pbkit.h>
#include <shaders/perspective_vertex_shader.h>
#include <xbox_math_matrix.h>
#include <xbox_math_vector.h>

#include "../test_host.h"
#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "texture_generator.h"

static constexpr char kTestControlFlagsLightOffFixedFunction[] = "ControlFlagsLightDisable_FF";
static constexpr char kTestControlLightOffFlagsShader[] = "ControlFlagsLightDisable_VS";
static constexpr char kTestControlFlagsNoLightFixedFunction[] = "ControlFlagsNoLight_FF";
static constexpr char kTestControlNoLightFlagsShader[] = "ControlFlagsNoLight_VS";
static constexpr char kTestControlFlagsFixedFunction[] = "ControlFlags_FF";
static constexpr char kTestControlFlagsShader[] = "ControlFlags_VS";

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF000000;
static constexpr uint32_t kCheckerboardC = 0xFF909090;

static constexpr vector_t kSceneAmbientColor{0.031373f, 0.031373f, 0.031373f, 0.f};

static constexpr vector_t kSpecularUL{1.f, 0.f, 0.f, 0.15f};
static constexpr vector_t kSpecularUR{1.f, 0.f, 0.f, 0.75f};
static constexpr vector_t kSpecularLR{0.f, 1.f, 0.f, 1.f};
static constexpr vector_t kSpecularLL{0.f, 1.f, 0.f, 0.20f};

static constexpr vector_t kDiffuseUL{0.5f, 0.f, 0.f, 1.f};
static constexpr vector_t kDiffuseUR{0.75f, 0.f, 0.f, 1.f};
static constexpr vector_t kDiffuseLR{1.f, 0.f, 0.f, 1.f};
static constexpr vector_t kDiffuseLL{0.25f, 0.f, 0.f, 1.f};

// From the left, pointing right and into the screen.
static constexpr vector_t kDirectionalLightDir{1.f, 0.f, 1.f, 1.f};
static constexpr vector_t kDirectionalLightAmbientColor{0.05f, 0.05f, 0.f, 0.f};
static constexpr vector_t kDirectionalLightDiffuseColor{0.7f, 0.f, 0.f, 0.f};
static constexpr vector_t kDirectionalLightSpecularColor{0.f, 0.75f, 0.f, 0.f};

static constexpr vector_t kPointLightAmbientColor{0.05f, 0.05f, 0.f, 0.f};
static constexpr vector_t kPointLightDiffuseColor{0.9f, 0.f, 0.f, 0.f};
static constexpr vector_t kPointLightSpecularColor{0.f, 0.9f, 0.f, 0.f};

struct SpecularParamTestCase {
  const char name[32];
  const float params[6];
};

static constexpr SpecularParamTestCase kSpecularParamSets[]{
    {"MechAssault", {-0.838916f, -2.887156f, 3.048239f, -0.706496f, -2.507095f, 2.800600f}},
    {"NinjaGaidenBlack", {-0.932112f, -3.097628f, 3.165516f, -0.869210f, -2.935466f, 3.066255f}},
    {"AllZero", {0.f, 0.f, 0.f, 0.f, 0.f, 0.f}},
    {"InvMechAssault", {0.838916f, 2.887156f, -3.048239f, 0.706496f, 2.507095f, -2.800600f}},
    {"Pow0_1", {-0.0, 0.9, 0.1, -0.0, 0.95, 0.05}},
    {"Pow1_0", {-0.0, -0.494592, 1.49459, -0.000244, 0.500122, 0.499634}},
    {"Pow2_0", {-0.170208, -0.855843, 1.68563, -0.0, -0.494592, 1.49459}},
    {"Pow4_0", {-0.421539, -1.70592, 2.28438, -0.170208, -0.855842, 1.68563}},
    {"Pow8_0", {-0.64766, -2.36199, 2.71433, -0.421539, -1.70592, 2.28438}},
    {"Pow16_0", {-0.803673, -2.7813, 2.97762, -0.64766, -2.36199, 2.71433}},
    {"Pow24_0", {-0.863702, -2.92701, 3.06331, -0.747554, -2.6184, 2.87085}}};

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc ControlFlagsLightDisable_FF
 * \parblock
 *  Demonstrates that setting NV097_SET_LIGHT_TWO_SIDE_ENABLE will cause specular and diffuse channels to be
 *  {0, 0, 0, 1} for back facing polygons even with lighting disbled.
 *
 *  Renders two groups of two rows of 4 quads each with the fixed function pipeline. The first row is the specular
 *  component, the second is the diffuse. Lighting is disabled.
 *
 *  Each quad has back specular and back diffuse colors of:
 *
 *    UL: {1.f, 0.f, 0.0f, 0.15f}, {0.5f, 0.f, 0.f, 1.f}
 *    UR: {1.f, 0.f, 0.0f, 0.75f}, {0.75f, 0.f, 0.f, 1.f}
 *    LR: {0.f, 1.f, 0.0f, 1.f}, {1.f, 0.f, 0.f, 1.f}
 *    LL: {0.f, 1.f, 0.0f, 0.20f}, {0.25f, 0.f, 0.f, 1.f}
 *
 *  Front specular is set to {0.f, 0.f, 1.f, 0.33f} and diffuse {0.0f, 1.0f, 0.5f, 1.0f}.
 *
 *  The upper two rows have SET_SPECULAR_ENABLE = false. The lower rows have it true.
 *  The first column has SET_LIGHT_CONTROL = SEPARATE_SPECULAR + ALPHA_FROM_MATERIAL_SPECULAR
 *  The second has SET_LIGHT_CONTROL = SEPARATE_SPECULAR
 *  The third has SET_LIGHT_CONTROL = ALPHA_FROM_MATERIAL_SPECULAR
 *  The fourth has SET_LIGHT_CONTROL = 0
 * \endparblock
 *
 * @tc ControlFlagsLightDisable_VS
 * \parblock
 *  Demonstrates that SET_SPECULAR_ENABLE = false forces back specular values to {0, 0, 0, 1}. Also demonstrates that
 *  failing to set ALPHA_FROM_MATERIAL_SPECULAR will cause the specular alpha to be set to 1. The vertex shader back
 *  color outputs are applied directly.
 *
 *  Renders two groups of two rows of 4 quads each with the fixed function pipeline. The first row is the specular
 *  component, the second is the diffuse. Lighting is disabled.
 *
 *  Each quad has back specular and back diffuse colors of:
 *
 *    UL: {1.f, 0.f, 0.0f, 0.15f}, {0.5f, 0.f, 0.f, 1.f}
 *    UR: {1.f, 0.f, 0.0f, 0.75f}, {0.75f, 0.f, 0.f, 1.f}
 *    LR: {0.f, 1.f, 0.0f, 1.f}, {1.f, 0.f, 0.f, 1.f}
 *    LL: {0.f, 1.f, 0.0f, 0.20f}, {0.25f, 0.f, 0.f, 1.f}
 *
 *  Front specular is set to {0.f, 0.f, 1.f, 0.33f} and diffuse {0.0f, 1.0f, 0.5f, 1.0f}.
 *
 *  The upper row has SET_SPECULAR_ENABLE = false. The lower row has it true.
 *  The first column has SET_LIGHT_CONTROL = SEPARATE_SPECULAR + ALPHA_FROM_MATERIAL_SPECULAR
 *  The second has SET_LIGHT_CONTROL = SEPARATE_SPECULAR
 *  The third has SET_LIGHT_CONTROL = ALPHA_FROM_MATERIAL_SPECULAR
 *  The fourth has SET_LIGHT_CONTROL = 0
 * \endparblock
 *
 * @tc ControlFlagsNoLight_FF
 * \parblock
 *  Demonstrates that SET_SPECULAR_ENABLE = false forces specular values to {0, 0, 0, 1}. Also demonstrates that failing
 *  to set ALPHA_FROM_MATERIAL_SPECULAR will cause the specular alpha to be set to 1 and failing to set
 *  SEPARATE_SPECULAR will pass through vertex _front_ specular RGB in the specular channel instead of performing the
 *  light calculation (which is black due to lack of light). Diffuse alpha is defined via SET_MATERIAL_ALPHA_BACK.
 *
 *  Renders two groups of two rows of 4 quads each with the fixed function pipeline. The first row is the specular
 *  component, the second is the diffuse. Lighting is enabled but no light exists.
 *
 *  Each quad has back specular and back diffuse colors of:
 *
 *    UL: {1.f, 0.f, 0.0f, 0.15f}, {0.5f, 0.f, 0.f, 1.f}
 *    UR: {1.f, 0.f, 0.0f, 0.75f}, {0.75f, 0.f, 0.f, 1.f}
 *    LR: {0.f, 1.f, 0.0f, 1.f}, {1.f, 0.f, 0.f, 1.f}
 *    LL: {0.f, 1.f, 0.0f, 0.20f}, {0.25f, 0.f, 0.f, 1.f}
 *
 *  Front specular is set to {0.f, 0.f, 1.f, 0.33f} and diffuse {0.0f, 1.0f, 0.5f, 1.0f}.
 *
 *  The upper row has SET_SPECULAR_ENABLE = false. The lower row has it true.
 *  The first column has SET_LIGHT_CONTROL = SEPARATE_SPECULAR + ALPHA_FROM_MATERIAL_SPECULAR
 *  The second has SET_LIGHT_CONTROL = SEPARATE_SPECULAR
 *  The third has SET_LIGHT_CONTROL = ALPHA_FROM_MATERIAL_SPECULAR
 *  The fourth has SET_LIGHT_CONTROL = 0
 *
 *  Because lighting is enabled, the diffuse alpha is entirely dictated by SET_MATERIAL_ALPHA, and RGB is entirely set
 *  by the lighting calculations.
 * \endparblock
 *
 * @tc ControlFlagsNoLight_VS
 * \parblock
 *  Demonstrates that SET_SPECULAR_ENABLE = false forces specular values to {0, 0, 0, 1}. Also demonstrates that failing
 *  to set ALPHA_FROM_MATERIAL_SPECULAR will cause the back specular alpha to be set to 1 and failing to set
 *  SEPARATE_SPECULAR will pass through vertex back specular RGB in the specular channel instead of performing the
 *  light calculation (which is black due to lack of light). Diffuse alpha is defined via SET_MATERIAL_ALPHA_BACK.
 *
 *  Renders two groups of two rows of 4 quads each with the fixed function pipeline. The first row is the specular
 *  component, the second is the diffuse. Lighting is enabled but no light exists.
 *
 *  Each quad has back specular and back diffuse colors of:
 *
 *    UL: {1.f, 0.f, 0.0f, 0.15f}, {0.5f, 0.f, 0.f, 1.f}
 *    UR: {1.f, 0.f, 0.0f, 0.75f}, {0.75f, 0.f, 0.f, 1.f}
 *    LR: {0.f, 1.f, 0.0f, 1.f}, {1.f, 0.f, 0.f, 1.f}
 *    LL: {0.f, 1.f, 0.0f, 0.20f}, {0.25f, 0.f, 0.f, 1.f}
 *
 *  Front specular is set to {0.f, 0.f, 1.f, 0.33f} and diffuse {0.0f, 1.0f, 0.5f, 1.0f}.
 *
 *  The upper row has SET_SPECULAR_ENABLE = false. The lower row has it true.
 *  The first column has SET_LIGHT_CONTROL = SEPARATE_SPECULAR + ALPHA_FROM_MATERIAL_SPECULAR
 *  The second has SET_LIGHT_CONTROL = SEPARATE_SPECULAR
 *  The third has SET_LIGHT_CONTROL = ALPHA_FROM_MATERIAL_SPECULAR
 *  The fourth has SET_LIGHT_CONTROL = 0
 *
 *  Because lighting is enabled, the diffuse alpha is entirely dictated by SET_MATERIAL_ALPHA, and RGB is entirely set
 *  by the lighting calculations.
 * \endparblock
 *
 * @tc ControlFlags_FF
 * \parblock
 *  Demonstrates that SET_SPECULAR_ENABLE = false forces specular values to {0, 0, 0, 1}. Also demonstrates that failing
 *  to set ALPHA_FROM_MATERIAL_SPECULAR will cause the specular alpha to be set to 1 and failing to set
 *  SEPARATE_SPECULAR will pass through vertex _front_ specular RGB in the specular channel instead of
 *  performing the light calculation. Diffuse alpha is defined via SET_MATERIAL_ALPHA_BACK.
 *
 *  Renders two groups of two rows of 4 quads each with the fixed function pipeline. The first row is the specular
 *  component, the second is the diffuse. Lighting is enabled but with a directional light pointing at {1, 0, 1}.
 *
 *  Each quad has back specular and back diffuse colors of:
 *
 *    UL: {1.f, 0.f, 0.0f, 0.15f}, {0.5f, 0.f, 0.f, 1.f}
 *    UR: {1.f, 0.f, 0.0f, 0.75f}, {0.75f, 0.f, 0.f, 1.f}
 *    LR: {0.f, 1.f, 0.0f, 1.f}, {1.f, 0.f, 0.f, 1.f}
 *    LL: {0.f, 1.f, 0.0f, 0.20f}, {0.25f, 0.f, 0.f, 1.f}
 *
 *  Front specular is set to {0.f, 0.f, 1.f, 0.33f} and diffuse {0.0f, 1.0f, 0.5f, 1.0f}.
 *
 *  The upper row has SET_SPECULAR_ENABLE = false. The lower row has it true.
 *  The first column has SET_LIGHT_CONTROL = SEPARATE_SPECULAR + ALPHA_FROM_MATERIAL_SPECULAR
 *  The second has SET_LIGHT_CONTROL = SEPARATE_SPECULAR
 *  The third has SET_LIGHT_CONTROL = ALPHA_FROM_MATERIAL_SPECULAR
 *  The fourth has SET_LIGHT_CONTROL = 0
 *
 *  Because lighting is enabled, the diffuse alpha is entirely dictated by SET_MATERIAL_ALPHA, and RGB is entirely set
 *  by the lighting calculations.
 * \endparblock
 *
 * @tc ControlFlags_VS
 * \parblock
 *  Demonstrates that SET_SPECULAR_ENABLE = false forces specular values to {0, 0, 0, 1}. Also demonstrates that failing
 *  to set ALPHA_FROM_MATERIAL_SPECULAR will cause the specular alpha to be set to 1 and failing to set
 *  SEPARATE_SPECULAR will pass through vertex back specular RGB instead of performing the light calculation.
 *
 *  Renders two groups of two rows of 4 quads each with the fixed function pipeline. The first row is the specular
 *  component, the second is the diffuse. Lighting is enabled but with a directional light pointing at {1, 0, 1}.
 *
 *  Each quad has back specular and back diffuse colors of:
 *
 *    UL: {1.f, 0.f, 0.0f, 0.15f}, {0.5f, 0.f, 0.f, 1.f}
 *    UR: {1.f, 0.f, 0.0f, 0.75f}, {0.75f, 0.f, 0.f, 1.f}
 *    LR: {0.f, 1.f, 0.0f, 1.f}, {1.f, 0.f, 0.f, 1.f}
 *    LL: {0.f, 1.f, 0.0f, 0.20f}, {0.25f, 0.f, 0.f, 1.f}
 *
 *  Front specular is set to {0.f, 0.f, 1.f, 0.33f} and diffuse {0.0f, 1.0f, 0.5f, 1.0f}.
 *
 *  The upper row has SET_SPECULAR_ENABLE = false. The lower row has it true.
 *  The first column has SET_LIGHT_CONTROL = SEPARATE_SPECULAR + ALPHA_FROM_MATERIAL_SPECULAR
 *  The second has SET_LIGHT_CONTROL = SEPARATE_SPECULAR
 *  The third has SET_LIGHT_CONTROL = ALPHA_FROM_MATERIAL_SPECULAR
 *  The fourth has SET_LIGHT_CONTROL = 0
 *
 *  Because lighting is enabled, the diffuse alpha is entirely dictated by SET_MATERIAL_ALPHA, and RGB is entirely set
 *  by the lighting calculations.
 * \endparblock
 *
 * @tc SpecParams_FF_AllZero
 *  Renders a number of meshes with a directional light and a point light, shading the specular channel only.
 *  LIGHT_CONTROL is set to SEPARATE_SPECULAR and SET_SPECULAR_PARAMS_BACK is set to the values displayed in the image.
 *  The Z coordinate of model normals is inverted and winding is swapped to make front faces back.
 *
 * @tc SpecParams_FF_InvMechAssault
 *  Renders a number of meshes with a directional light and a point light, shading the specular channel only.
 *  LIGHT_CONTROL is set to SEPARATE_SPECULAR and SET_SPECULAR_PARAMS_BACK is set to the values displayed in the image.
 *  The Z coordinate of model normals is inverted and winding is swapped to make front faces back.
 *
 * @tc SpecParams_FF_MechAssault
 *  Renders a number of meshes with a directional light and a point light, shading the specular channel only.
 *  LIGHT_CONTROL is set to SEPARATE_SPECULAR and SET_SPECULAR_PARAMS_BACK is set to the values displayed in the image.
 *  The Z coordinate of model normals is inverted and winding is swapped to make front faces back.
 *
 * @tc SpecParams_FF_NinjaGaidenBlack
 *  Renders a number of meshes with a directional light and a point light, shading the specular channel only.
 *  LIGHT_CONTROL is set to SEPARATE_SPECULAR and SET_SPECULAR_PARAMS_BACK is set to the values displayed in the image.
 *  The Z coordinate of model normals is inverted and winding is swapped to make front faces back.
 *
 */
SpecularBackTests::SpecularBackTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Specular back", config) {
  tests_[kTestControlFlagsNoLightFixedFunction] = [this]() {
    this->TestControlFlags(kTestControlFlagsNoLightFixedFunction, true, true, false);
  };
  tests_[kTestControlNoLightFlagsShader] = [this]() {
    this->TestControlFlags(kTestControlNoLightFlagsShader, false, true, false);
  };
  tests_[kTestControlFlagsFixedFunction] = [this]() {
    this->TestControlFlags(kTestControlFlagsFixedFunction, true, true, true);
  };
  tests_[kTestControlFlagsShader] = [this]() { this->TestControlFlags(kTestControlFlagsShader, false, true, true); };

  tests_[kTestControlFlagsLightOffFixedFunction] = [this]() {
    this->TestControlFlags(kTestControlFlagsLightOffFixedFunction, true, false, false);
  };
  tests_[kTestControlLightOffFlagsShader] = [this]() {
    this->TestControlFlags(kTestControlLightOffFlagsShader, false, false, false);
  };

  for (auto& test_case : kSpecularParamSets) {
    std::string name = "SpecParams_FF_";
    name += test_case.name;

    tests_[name] = [this, name, &test_case]() { this->TestSpecularParams(name, test_case.params); };
  }
}

static void PixelShaderJustDiffuse(TestHost& host) {
  host.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                             false, TestHost::MAP_UNSIGNED_INVERT);

  host.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                             TestHost::MAP_UNSIGNED_INVERT);
}

static void PixelShaderJustSpecular(TestHost& host) {
  host.SetInputColorCombiner(0, TestHost::SRC_SPECULAR, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                             false, TestHost::MAP_UNSIGNED_INVERT);

  host.SetInputAlphaCombiner(0, TestHost::SRC_SPECULAR, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                             false, TestHost::MAP_UNSIGNED_INVERT);
}

void SpecularBackTests::Initialize() {
  TestSuite::Initialize();

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);

    p = pb_push3fv(p, NV097_SET_SCENE_AMBIENT_COLOR, kSceneAmbientColor);

    p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
    p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);

    p = pb_push1(p, NV097_SET_LIGHT_TWO_SIDE_ENABLE, true);
    pb_end(p);
  }

  // Setup pixel shader to just utilize specular component.
  host_.SetCombinerControl(1);
  PixelShaderJustSpecular(host_);

  host_.SetOutputColorCombiner(0, TestHost::DST_R0, TestHost::DST_DISCARD, TestHost::DST_DISCARD);
  host_.SetOutputAlphaCombiner(0, TestHost::DST_R0, TestHost::DST_DISCARD, TestHost::DST_DISCARD);

  host_.SetFinalCombiner0Just(TestHost::SRC_R0);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                          false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);

  CreateGeometry();
}

void SpecularBackTests::Deinitialize() {
  vertex_buffer_cone_.reset();
  vertex_buffer_cylinder_.reset();
  vertex_buffer_sphere_.reset();
  vertex_buffer_suzanne_.reset();
  vertex_buffer_torus_.reset();
}

static std::shared_ptr<PerspectiveVertexShader> SetupVertexShader(TestHost& host) {
  // Use a custom shader that approximates the interesting lighting portions of the fixed function pipeline.
  float depth_buffer_max_value = host.GetMaxDepthBufferValue();
  auto shader = std::make_shared<PerspectiveVertexShader>(host.GetFramebufferWidth(), host.GetFramebufferHeight(), 0.0f,
                                                          depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  shader->SetLightingEnabled(false);
  vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
  vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
  shader->LookAt(camera_position, camera_look_at);

  host.SetVertexShaderProgram(shader);

  return shader;
}

static void MakeBackFaceFront() {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_FRONT_FACE, NV097_SET_FRONT_FACE_V_CCW);
  p = pb_push1(p, NV097_SET_CULL_FACE, NV097_SET_CULL_FACE_V_FRONT);
  p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, true);
  pb_end(p);
}

static void MakeFrontFaceFront() {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_FRONT_FACE, NV097_SET_FRONT_FACE_V_CW);
  p = pb_push1(p, NV097_SET_CULL_FACE, NV097_SET_CULL_FACE_V_BACK);
  pb_end(p);
}

void SpecularBackTests::TestControlFlags(const std::string& name, bool use_fixed_function, bool enable_lighting,
                                         bool enable_light) {
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  std::shared_ptr<PerspectiveVertexShader> shader;
  if (use_fixed_function) {
    host_.SetVertexShaderProgram(nullptr);
  } else {
    shader = SetupVertexShader(host_);
  }

  host_.PrepareDraw(0xFE323232);

  host_.DrawCheckerboardUnproject(kCheckerboardC, kCheckerboardA, 14);

  uint32_t light_mode_bitvector = 0;
  if (enable_light) {
    {
      auto light = DirectionalLight(0, kDirectionalLightDir);
      // None of the front colors should be used, all geometry is back-side.
      light.SetAmbient(0.f, 0.f, 0.1f);
      light.SetDiffuse(0.f, 0.f, 0.2f);
      light.SetSpecular(0.f, 0.f, 0.7f);

      light.SetBackAmbient(kDirectionalLightAmbientColor);
      light.SetBackDiffuse(kDirectionalLightDiffuseColor);
      light.SetBackSpecular(kDirectionalLightSpecularColor);

      vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
      vector_t at{0.0f, 0.0f, 0.0f, 1.0f};

      vector_t look_dir{0.f, 0.f, 0.f, 1.f};
      VectorSubtractVector(at, eye, look_dir);
      VectorNormalize(look_dir);
      light.Commit(host_, look_dir);

      light_mode_bitvector |= light.light_enable_mask();
    }
  }

  MakeBackFaceFront();
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, enable_lighting);
    p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, light_mode_bitvector);

    p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 0.15f);  // Should not be used since only backs are rendered.
    p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA_BACK, 0.75f);

    // Pow 16
    const float specular_params[]{-0.803673, -2.7813, 2.97762, -0.64766, -2.36199, 2.71433};
    // Set up the back specular params and set the front specular to 0 so that it will be very obvious if anything
    // uses it.
    for (uint32_t i = 0, offset = 0; i < 6; ++i, offset += 4) {
      p = pb_push1f(p, NV097_SET_SPECULAR_PARAMS + offset, 0);
      p = pb_push1f(p, NV097_SET_SPECULAR_PARAMS_BACK + offset, specular_params[i]);
    }

    pb_end(p);
  }

  const auto fb_width = host_.GetFramebufferWidthF();
  const auto fb_height = host_.GetFramebufferHeightF();

  const auto quad_spacing = 15.f;
  const auto quad_width = fb_width / 6.f;
  const auto quad_height = fb_height / 6.f;
  static constexpr auto quad_z = 1.f;
  static constexpr auto quad_w = 1.f;

  auto unproject = [this, shader](vector_t& world_point, float x, float y) {
    vector_t screen_point{x, y, quad_z, quad_w};
    if (shader) {
      shader->UnprojectPoint(world_point, screen_point, quad_z);
    } else {
      host_.UnprojectPoint(world_point, screen_point, quad_z);
    }
  };

  auto draw_quad = [this, quad_width, quad_height, unproject](float left, float top) {
    const auto right = left + quad_width;
    const auto bottom = top + quad_height;

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    vector_t world_point{0.f, 0.f, 0.f, 1.f};

    host_.SetDiffuse(0.0f, 1.0f, 0.5f, 1.0f);
    host_.SetSpecular(0.f, 0.f, 1.f, 0.33f);

    host_.SetBackDiffuse(kDiffuseUL);
    host_.SetBackSpecular(kSpecularUL);
    {
      vector_t normal{-0.1f, 0.05f, 0.85f, 1.f};
      VectorNormalize(normal);
      host_.SetNormal(normal);
    }
    host_.SetTexCoord0(0.f, 0.f);
    unproject(world_point, left, top);
    host_.SetVertex(world_point);

    host_.SetBackDiffuse(kDiffuseUR);
    host_.SetBackSpecular(kSpecularUR);
    {
      vector_t normal{0.1f, 0.05f, 0.85f, 1.f};
      VectorNormalize(normal);
      host_.SetNormal(normal);
    }
    host_.SetTexCoord0(1.f, 0.f);
    unproject(world_point, right, top);
    host_.SetVertex(world_point);

    host_.SetBackDiffuse(kDiffuseLR);
    host_.SetBackSpecular(kSpecularLR);
    {
      vector_t normal{0.1f, -0.05f, 0.85f, 1.f};
      VectorNormalize(normal);
      host_.SetNormal(normal);
    }
    host_.SetTexCoord0(1.f, 1.f);
    unproject(world_point, right, bottom);
    host_.SetVertex(world_point);

    host_.SetBackDiffuse(kDiffuseLL);
    host_.SetBackSpecular(kSpecularLL);
    {
      vector_t normal{-0.1f, -0.05f, 0.85f, 1.f};
      VectorNormalize(normal);
      host_.SetNormal(normal);
    }
    host_.SetTexCoord0(0.f, 1.f);
    unproject(world_point, left, bottom);
    host_.SetVertex(world_point);

    host_.End();
  };

  auto draw_row = [this, quad_width, quad_spacing, draw_quad](float top) {
    auto left = quad_width;
    const auto inc_x = quad_width + quad_spacing;

    // Separate specular + alpha from material.
    {
      auto p = pb_begin();
      p = pb_push1(
          p, NV097_SET_LIGHT_CONTROL,
          NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR | NV097_SET_LIGHT_CONTROL_V_ALPHA_FROM_MATERIAL_SPECULAR);
      pb_end(p);
    }
    draw_quad(left, top);
    host_.PBKitBusyWait();
    left += inc_x;

    // Separate specular
    {
      auto p = pb_begin();
      p = pb_push1(p, NV097_SET_LIGHT_CONTROL, NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR);
      pb_end(p);
    }
    draw_quad(left, top);
    host_.PBKitBusyWait();
    left += inc_x;

    // Alpha from material
    {
      auto p = pb_begin();
      p = pb_push1(p, NV097_SET_LIGHT_CONTROL, NV097_SET_LIGHT_CONTROL_V_ALPHA_FROM_MATERIAL_SPECULAR);
      pb_end(p);
    }
    draw_quad(left, top);
    host_.PBKitBusyWait();
    left += inc_x;

    // None
    {
      auto p = pb_begin();
      p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0);
      pb_end(p);
    }
    draw_quad(left, top);
    host_.PBKitBusyWait();
  };

  auto top = quad_height + 15.f;
  const auto inc_y = quad_height + quad_spacing;

  // Specular disabled row
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, false);
    pb_end(p);
  }

  PixelShaderJustSpecular(host_);
  draw_row(top);
  top += inc_y;

  PixelShaderJustDiffuse(host_);
  draw_row(top);
  top += inc_y;

  // Specular enabled row
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);
    pb_end(p);
  }

  PixelShaderJustSpecular(host_);
  draw_row(top);
  top += inc_y;

  PixelShaderJustDiffuse(host_);
  draw_row(top);

  MakeFrontFaceFront();

  pb_print("%s\n", name.c_str());
  pb_printat(4, 0, "SPEC_EN=F");
  pb_printat(1, 10, "SEP_SPEC");
  pb_printat(2, 10, "ALPHA_MAT");
  pb_printat(1, 22, "SEP_SPEC");
  pb_printat(2, 34, "ALPHA_MAT");
  pb_printat(13, 0, "SPEC_EN=T");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void SpecularBackTests::CreateGeometry() {
  // SET_COLOR_MATERIAL below causes per-vertex diffuse color to be ignored entirely.
  vector_t diffuse{1.f, 0.f, 1.f, 0.75f};
  vector_t back_diffuse{0.f, 0.f, 0.f, 0.75f};

  vector_t specular{1.f, 0.f, 1.f, 0.15f};
  vector_t back_specular{0.f, 1.f, 1.f, 0.25f};

  auto construct_model = [this](ModelBuilder& model, std::shared_ptr<VertexBuffer>& vertex_buffer) {
    vertex_buffer = host_.AllocateVertexBuffer(model.GetVertexCount());
    model.PopulateVertexBuffer(vertex_buffer);

    // Reflect the normal to coincide with previously forward faces now being back faces.
    auto vertex = vertex_buffer->Lock();
    for (auto i = 0; i < vertex_buffer->GetNumVertices(); ++i, ++vertex) {
      vertex->normal[2] *= -1.f;
    }
    vertex_buffer->Unlock();
  };

  {
    auto model = LightControlTestMeshConeModel(diffuse, specular, back_diffuse, back_specular);
    construct_model(model, vertex_buffer_cone_);
  }
  {
    auto model = LightControlTestMeshCylinderModel(diffuse, specular, back_diffuse, back_specular);
    construct_model(model, vertex_buffer_cylinder_);
  }
  {
    auto model = LightControlTestMeshSphereModel(diffuse, specular, back_diffuse, back_specular);
    construct_model(model, vertex_buffer_sphere_);
  }
  {
    auto model = LightControlTestMeshSuzanneModel(diffuse, specular, back_diffuse, back_specular);
    construct_model(model, vertex_buffer_suzanne_);
  }
  {
    auto model = LightControlTestMeshTorusModel(diffuse, specular, back_diffuse, back_specular);
    construct_model(model, vertex_buffer_torus_);
  }
}

void SpecularBackTests::TestSpecularParams(const std::string& name, const float* specular_params) {
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  std::shared_ptr<PerspectiveVertexShader> shader;
  host_.SetVertexShaderProgram(nullptr);

  host_.PrepareDraw(0xFE313131);
  host_.DrawCheckerboardUnproject(kCheckerboardA, kCheckerboardB, 20);

  PixelShaderJustSpecular(host_);
  {
    uint32_t light_mode_bitvector = 0;
    {
      auto light = DirectionalLight(0, kDirectionalLightDir);
      light.SetAmbient(0.f, 0.f, 1.f);
      light.SetDiffuse(0.f, 0.f, 1.f);
      light.SetSpecular(0.f, 0.f, 1.f);
      light.SetBackAmbient(kDirectionalLightAmbientColor);
      light.SetBackDiffuse(kDirectionalLightDiffuseColor);
      light.SetBackSpecular(kDirectionalLightSpecularColor);

      vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
      vector_t at{0.0f, 0.0f, 0.0f, 1.0f};

      vector_t look_dir{0.f, 0.f, 0.f, 1.f};
      VectorSubtractVector(at, eye, look_dir);
      VectorNormalize(look_dir);
      light.Commit(host_, look_dir);

      light_mode_bitvector |= light.light_enable_mask();
    }

    {
      vector_t position{1.5f, 1.f, -2.5f, 1.f};
      static constexpr auto kRange = 4.f;
      static constexpr auto kAttenuationConstant = 0.025f;
      static constexpr auto kAttentuationLinear = 0.15f;
      static constexpr auto kAttenuationQuadratic = 0.02f;
      auto light = PointLight(1, position, kRange, kAttenuationConstant, kAttentuationLinear, kAttenuationQuadratic);

      light.SetAmbient(0.f, 0.f, 1.f);
      light.SetDiffuse(0.f, 0.f, 1.f);
      light.SetSpecular(0.f, 0.f, 1.f);
      light.SetBackAmbient(kPointLightAmbientColor);
      light.SetBackDiffuse(kPointLightDiffuseColor);
      light.SetBackSpecular(kPointLightSpecularColor);
      light.Commit(host_);

      light_mode_bitvector |= light.light_enable_mask();
    }

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);

    static constexpr vector_t kBrightAmbientColor{0.1f, 0.1f, 0.1f, 0.f};
    p = pb_push3fv(p, NV097_SET_SCENE_AMBIENT_COLOR, kBrightAmbientColor);

    p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
    p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);

    // These should both be irrelevant since only the specular color is rendered and
    // NV097_SET_LIGHT_CONTROL_V_ALPHA_FROM_MATERIAL_SPECULAR is not set.
    p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 0.15f);
    p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA_BACK, 0.75f);

    p = pb_push1(p, NV097_SET_LIGHT_CONTROL, NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR);

    // Set up the back specular params and set the front specular to 0 so that it will be very obvious if anything
    // uses it.
    for (uint32_t i = 0, offset = 0; i < 6; ++i, offset += 4) {
      p = pb_push1f(p, NV097_SET_SPECULAR_PARAMS + offset, 0);
      p = pb_push1f(p, NV097_SET_SPECULAR_PARAMS_BACK + offset, specular_params[i]);
    }

    p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, light_mode_bitvector);

    pb_end(p);
  }

  MakeBackFaceFront();

  for (auto& vb : {
           vertex_buffer_cone_,
           vertex_buffer_cylinder_,
           vertex_buffer_sphere_,
           vertex_buffer_suzanne_,
           vertex_buffer_torus_,
       }) {
    host_.SetVertexBuffer(vb);
    host_.DrawArrays(TestHost::POSITION | TestHost::NORMAL | TestHost::DIFFUSE | TestHost::SPECULAR |
                     TestHost::BACK_DIFFUSE | TestHost::BACK_SPECULAR);
  }

  MakeFrontFaceFront();

  pb_print("%s\n", name.c_str());
  for (auto i = 0; i < 6; ++i) {
    pb_print_with_floats("%f\n", specular_params[i]);
  }
  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
