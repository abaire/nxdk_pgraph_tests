#include "lighting_control_tests.h"

#include <models/flat_mesh_grid_model.h>
#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "models/light_control_test_mesh_cone_model.h"
#include "models/light_control_test_mesh_cylinder_model.h"
#include "models/light_control_test_mesh_sphere_model.h"
#include "models/light_control_test_mesh_suzanne_model.h"
#include "models/light_control_test_mesh_torus_model.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"
#include "vertex_buffer.h"
#include "xbox_math_matrix.h"

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF000000;

static std::string MakeTestName(uint32_t light_control, bool is_fixed_function, bool specular_enabled) {
  char buf[32] = {0};
  snprintf(buf, 31, "%s_0x%06X%s", is_fixed_function ? "FF" : "VS", light_control, specular_enabled ? "" : "NoSpec");
  return buf;
}

/**
 * @tc FF_0x000000
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED on.
 *  Separate specular is off, causing the green of the mesh specular to be retained.
 *  Localeye is off, using a simplified model for specular highlighting, causing artifacts for the point light on the
 *  cylinder mesh.
 *  Light control alpha is forced to opaque.
 *
 * @tc FF_0x000000NoSpec
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED off.
 *  Separate specular is ignored because SPECULAR_ENABLED is off, so the green of the mesh specular is not seen.
 *  Localeye is off, using a simplified model for specular highlighting, causing artifacts for the point light on the
 *  cylinder mesh.
 *  Light control alpha is ignored and the mesh is forced to opaque because SPECULAR_ENABLED is off.
 *  Visually equivalent to FF_0x000001.
 *
 * @tc FF_0x000001
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED on.
 *  Separate specular is on, so the green of the mesh specular is not seen.
 *  Localeye is off, using a simplified model for specular highlighting, causing artifacts for the point light on the
 *  cylinder mesh.
 *  Light control alpha is forced to opaque.
 *
 * @tc FF_0x000001NoSpec
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED off.
 *  Separate specular is ignored because SPECULAR_ENABLED is off, so the green of the mesh specular is not seen.
 *  Localeye is off, using a simplified model for specular highlighting, causing artifacts for the point light on the
 *  cylinder mesh.
 *  Light control alpha is ignored and the mesh is forced to opaque because SPECULAR_ENABLED is off.
 *
 * @tc FF_0x010000
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED on.
 *  Separate specular is off, causing the green of the mesh specular to be retained.
 *  Localeye is on, using a slower model for specular highlighting that results in more realistic behavior for the point
 *  light on the cylinder mesh.
 *  Light control alpha is forced to opaque.
 *
 * @tc FF_0x010000NoSpec
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED off.
 *  Separate specular is ignored because SPECULAR_ENABLED is off, so the green of the mesh specular is not seen.
 *  Localeye is on, using a slower model for specular highlighting that results in more realistic behavior for the point
 *  light on the cylinder mesh.
 *  Light control alpha is ignored and the mesh is forced to opaque because SPECULAR_ENABLED is off.
 *
 * @tc FF_0x010001
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED on.
 *  Separate specular is on, so the green of the mesh specular is not seen.
 *  Localeye is on, using a slower model for specular highlighting that results in more realistic behavior for the point
 *  light on the cylinder mesh.
 *  Light control alpha is forced to opaque.
 *
 * @tc FF_0x010001NoSpec
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED off.
 *  Separate specular is ignored because SPECULAR_ENABLED is off, so the green of the mesh specular is not seen.
 *  Localeye is on, using a slower model for specular highlighting that results in more realistic behavior for the point
 *  light on the cylinder mesh.
 *  Light control alpha is ignored and the mesh is forced to opaque because SPECULAR_ENABLED is off.
 *
 * @tc FF_0x020000
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED on.
 *  Separate specular is off, causing the green of the mesh specular to be retained.
 *  Localeye is off, using a simplified model for specular highlighting, causing artifacts for the point light on the
 *  cylinder mesh.
 *  Light control alpha is set to compute the alpha from the specular and light effects, so the model is transluscent.
 *
 * @tc FF_0x020000NoSpec
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED off.
 *  Separate specular is ignored because SPECULAR_ENABLED is off, so the green of the mesh specular is not seen.
 *  Localeye is off, using a simplified model for specular highlighting, causing artifacts for the point light on the
 *  cylinder mesh.
 *  Light control alpha is ignored and the mesh is forced to opaque because SPECULAR_ENABLED is off.
 *
 * @tc FF_0x020001
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED on.
 *  Separate specular is on, so the green of the mesh specular is not seen.
 *  Localeye is off, using a simplified model for specular highlighting, causing artifacts for the point light on the
 *  cylinder mesh.
 *  Light control alpha is set to compute the alpha from the specular and light effects, so the model is transluscent.
 *
 * @tc FF_0x020001NoSpec
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED off.
 *  Separate specular is ignored because SPECULAR_ENABLED is off, so the green of the mesh specular is not seen.
 *  Localeye is off, using a simplified model for specular highlighting, causing artifacts for the point light on the
 *  cylinder mesh.
 *  Light control alpha is ignored and the mesh is forced to opaque because SPECULAR_ENABLED is off.
 *
 * @tc FF_0x030000
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED on.
 *  Separate specular is off, causing the green of the mesh specular to be retained.
 *  Localeye is on, using a slower model for specular highlighting that results in more realistic behavior for the point
 *  light on the cylinder mesh.
 *  Light control alpha is set to compute the alpha from the specular and light effects, so the model is transluscent.
 *
 * @tc FF_0x030000NoSpec
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED off.
 *  Separate specular is ignored because SPECULAR_ENABLED is off, so the green of the mesh specular is not seen.
 *  Localeye is on, using a slower model for specular highlighting that results in more realistic behavior for the point
 *  light on the cylinder mesh.
 *  Light control alpha is ignored and the mesh is forced to opaque because SPECULAR_ENABLED is off.
 *
 * @tc FF_0x030001
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED on.
 *  Separate specular is on, so the green of the mesh specular is not seen.
 *  Localeye is on, using a slower model for specular highlighting that results in more realistic behavior for the point
 *  light on the cylinder mesh.
 *  Light control alpha is set to compute the alpha from the specular and light effects, so the model is transluscent.
 *
 * @tc FF_0x030001NoSpec
 *  Tests behavior using the fixed function pipeline with SET_SPECULAR_ENABLED off.
 *  Separate specular is ignored because SPECULAR_ENABLED is off, so the green of the mesh specular is not seen.
 *  Localeye is on, using a slower model for specular highlighting that results in more realistic behavior for the point
 *  light on the cylinder mesh.
 *  Light control alpha is ignored and the mesh is forced to opaque because SPECULAR_ENABLED is off.
 */
LightingControlTests::LightingControlTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Lighting control", config) {
  for (auto local_eye : {0, 1}) {
    for (auto separate_specular : {0, 1}) {
      for (auto sout : {0, 1}) {
        for (auto specular_enabled : {false, true}) {
          uint32_t light_control = (sout * NV097_SET_LIGHT_CONTROL_V_ALPHA_FROM_MATERIAL) +
                                   (local_eye * NV097_SET_LIGHT_CONTROL_V_LOCALEYE) +
                                   (separate_specular * NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR);
          // TODO: Implement vertex shader tests.
          std::string test_name = MakeTestName(light_control, true, specular_enabled);
          tests_[test_name] = [this, test_name, light_control, specular_enabled]() {
            this->TestFixed(test_name, light_control, specular_enabled);
          };
        }
      }
    }
  }
}

void LightingControlTests::Deinitialize() {
  vertex_buffer_cone_.reset();
  vertex_buffer_cylinder_.reset();
  vertex_buffer_sphere_.reset();
  vertex_buffer_suzanne_.reset();
  vertex_buffer_torus_.reset();
}

void LightingControlTests::CreateGeometry() {
  // SET_COLOR_MATERIAL below causes per-vertex diffuse color to be ignored entirely.
  vector_t diffuse{0.f, 0.f, 0.0f, 0.75f};

  // However:
  // 1) the alpha from the specular value is added to the material alpha.
  // 2) the color is added to the computed vertex color if SEPARATE_SPECULAR is OFF
  vector_t specular{0.f, 0.4, 0.f, 0.25f};

  auto construct_model = [this](ModelBuilder& model, std::shared_ptr<VertexBuffer>& vertex_buffer) {
    vertex_buffer = host_.AllocateVertexBuffer(model.GetVertexCount());
    model.PopulateVertexBuffer(vertex_buffer);
  };

  {
    auto model = LightControlTestMeshConeModel(diffuse, specular);
    construct_model(model, vertex_buffer_cone_);
  }
  {
    auto model = LightControlTestMeshCylinderModel(diffuse, specular);
    construct_model(model, vertex_buffer_cylinder_);
  }
  {
    auto model = LightControlTestMeshSphereModel(diffuse, specular);
    construct_model(model, vertex_buffer_sphere_);
  }
  {
    auto model = LightControlTestMeshSuzanneModel(diffuse, specular);
    construct_model(model, vertex_buffer_suzanne_);
  }
  {
    auto model = LightControlTestMeshTorusModel(diffuse, specular);
    construct_model(model, vertex_buffer_torus_);
  }
}

void LightingControlTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  CreateGeometry();
}

static void SetupLights(TestHost& host, bool specular_enabled) {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, specular_enabled);

  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_SPECULAR), 0);
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_DIFFUSE), 0xFFFFFFFF);
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_SPECULAR), 0);

  p = pb_push3f(p, NV097_SET_SCENE_AMBIENT_COLOR, 0.031373, 0.031373, 0.031373);

  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 0.40f);

  // Values taken from MechAssault
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x00, 0xBF56C33A);  // -0.838916
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x04, 0xC038C729);  // -2.887156
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x08, 0x4043165A);  // 3.048239
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x0c, 0xBF34DCE5);  // -0.706496
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x10, 0xC020743F);  // -2.507095
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x14, 0x40333D06);  // 2.800600

  // // Values from Ninja Gaiden Black
  // p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x00,0xBF6E9EE5); //  -0.932112)
  // p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x04,0xC0463F88); //  -3.097628)
  // p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x08,0x404A97CF); //  3.165516)
  // p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x0c,0xBF5E8491); //  -0.869210)
  // p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x10,0xC03BDEAB); //  -2.935466)
  // p = pb_push1(p, NV097_SET_SPECULAR_PARAMS + 0x14,0x40443D87); //  3.066255)

  // Collects the modes of each hardware light.
  uint32_t light_mode_bitvector = 0;

  // Directional light from the left pointing towards the right and into the screen.
  {
    constexpr uint32_t kLightNum = 0;

    // From the left, pointing right and into the screen.
    vector_t light_dir{1.f, 0.f, 1.f, 1.f};
    VectorNormalize(light_dir);

    // Calculate the Blinn half vector.
    // https://paroj.github.io/gltut/Illumination/Tut11%20BlinnPhong%20Model.html

    // The look at vector from the default XDK initialization in TestHost::BuildDefaultXDKModelViewMatrix
    vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
    vector_t at{0.0f, 0.0f, 0.0f, 1.0f};

    vector_t look_dir{0.f, 0.f, 0.f, 1.f};
    VectorSubtractVector(at, eye, look_dir);
    VectorNormalize(look_dir);

    vector_t half_angle_vector{0.f, 0.f, 0.f, 1.f};
    VectorAddVector(look_dir, light_dir, half_angle_vector);
    VectorNormalize(half_angle_vector);

    // Infinite direction is the direction towards the light source.
    ScalarMultVector(light_dir, -1.f);
    ScalarMultVector(half_angle_vector, -1.f);

    p = pb_push3f(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_AMBIENT_COLOR), 0.f, 0.f, 0.05f);
    p = pb_push3f(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_DIFFUSE_COLOR), 0.25f, 0.f, 0.f);
    p = pb_push3f(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_SPECULAR_COLOR), 0.f, 0.2f, 0.4f);
    p = pb_push1f(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_LOCAL_RANGE), 1e30f);
    p = pb_push3fv(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_INFINITE_HALF_VECTOR), half_angle_vector);
    p = pb_push3fv(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_INFINITE_DIRECTION), light_dir);

    light_mode_bitvector |= LIGHT_MODE(kLightNum, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);
  }

  // Point light on the right between the geometry and camera.
  {
    constexpr uint32_t kLightNum = 1;
    vector_t position{1.5f, 1.f, -2.5f, 1.f};
    const matrix4_t& view_matrix = host.GetFixedFunctionModelViewMatrix();
    VectorMultMatrix(position, view_matrix);
    position[3] = 1.f;

    p = pb_push3f(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_AMBIENT_COLOR), 0.f, 0.f, 0.05f);
    p = pb_push3f(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_DIFFUSE_COLOR), 0.25f, 0.f, 0.f);
    p = pb_push3f(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_SPECULAR_COLOR), 0.f, 0.2f, 0.4f);
    p = pb_push1f(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_LOCAL_RANGE), 4.f);
    p = pb_push3fv(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_LOCAL_POSITION), position);
    // Attenuation = param1 + (param2 * d) + (param3 * d^2) where d = distance from light to surface.
    p = pb_push3f(p, SET_LIGHT(kLightNum, NV097_SET_LIGHT_LOCAL_ATTENUATION), 0.025f, 0.15f, 0.02f);

    light_mode_bitvector |= LIGHT_MODE(kLightNum, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_LOCAL);
  }

  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, light_mode_bitvector);
  pb_end(p);

  host.SetCombinerControl(1);
  host.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                             false, TestHost::MAP_UNSIGNED_INVERT, TestHost::SRC_SPECULAR, false,
                             TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false, TestHost::MAP_UNSIGNED_INVERT);
  host.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false,
                             TestHost::MAP_UNSIGNED_INVERT, TestHost::SRC_SPECULAR, true,
                             TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO, false, TestHost::MAP_UNSIGNED_INVERT);

  host.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);
  host.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);

  host.SetFinalCombiner0Just(TestHost::SRC_R0);
  host.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                         false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                         /* specular_clamp */ true);
}

static void DrawCheckerboardBackground(TestHost& host) {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, false);
  pb_end(p);

  host.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  host.SetTextureStageEnabled(0, true);
  host.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  static constexpr uint32_t kTextureSize = 256;
  static constexpr uint32_t kCheckerSize = 24;

  auto& texture_stage = host.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host.SetupTextureStages();

  auto texture_memory = host.GetTextureMemoryForStage(0);
  GenerateSwizzledRGBACheckerboard(texture_memory, 0, 0, kTextureSize, kTextureSize, kTextureSize * 4, kCheckerboardA,
                                   kCheckerboardB, kCheckerSize);

  host.Begin(TestHost::PRIMITIVE_QUADS);
  host.SetTexCoord0(0.0f, 0.0f);
  vector_t world_point;
  vector_t screen_point = {0.f, 0.f, 0.0f, 1.0f};
  host.UnprojectPoint(world_point, screen_point, 1.f);
  host.SetVertex(world_point);

  host.SetTexCoord0(1.0f, 0.0f);
  screen_point[0] = host.GetFramebufferWidthF();
  host.UnprojectPoint(world_point, screen_point, 1.f);
  host.SetVertex(world_point);

  host.SetTexCoord0(1.0f, 1.0f);
  screen_point[1] = host.GetFramebufferHeightF();
  host.UnprojectPoint(world_point, screen_point, 1.f);
  host.SetVertex(world_point);

  host.SetTexCoord0(0.0f, 1.0f);
  screen_point[0] = 0.f;
  host.UnprojectPoint(world_point, screen_point, 1.f);
  host.SetVertex(world_point);

  host.End();

  host.SetTextureStageEnabled(0, false);
  host.SetShaderStageProgram(TestHost::STAGE_NONE);
}

void LightingControlTests::TestFixed(const std::string& name, uint32_t light_control, bool specular_enabled) {
  static constexpr uint32_t kBackgroundColor = 0xFF232623;
  host_.PrepareDraw(kBackgroundColor);

  DrawCheckerboardBackground(host_);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, light_control);
  pb_end(p);

  SetupLights(host_, specular_enabled);

  for (auto& vb : {
           vertex_buffer_cone_,
           vertex_buffer_cylinder_,
           vertex_buffer_sphere_,
           vertex_buffer_suzanne_,
           vertex_buffer_torus_,
       }) {
    host_.SetVertexBuffer(vb);
    host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);
  }

  pb_print("0x%X\n", light_control);
  pb_print("SEPARATE_SPECULAR %s\n", (light_control & NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR) ? "ON" : "off");
  pb_print("LOCALEYE %s\n", (light_control & NV097_SET_LIGHT_CONTROL_V_LOCALEYE) ? "ON" : "off");
  pb_print("ALPHA %s\n", (light_control & NV097_SET_LIGHT_CONTROL_V_ALPHA_FROM_MATERIAL) ? "FROM MATERIAL" : "OPAQUE");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
