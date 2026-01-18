#include "lighting_spotlight_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "models/flat_mesh_grid_model.h"
#include "models/light_control_test_mesh_cylinder_model.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "texture_generator.h"
#include "vertex_buffer.h"
#include "xbox_math_matrix.h"

static constexpr uint32_t kCheckerboardA = 0xFF111111;
static constexpr uint32_t kCheckerboardB = 0xFF000000;

static constexpr float PI_OVER_180 = (float)M_PI / 180.0f;
#define DEG2RAD(c) ((float)(c) * PI_OVER_180)

// static constexpr char kFalloffName[] = "Fo";
static constexpr char kFalloffFixedName[] = "FoFixed";
// static constexpr char kPhiThetaName[] = "PT";
static constexpr char kPhiThetaFixedName[] = "PTFixed";
// static constexpr char kAttenuationName[] = "At";
static constexpr char kAttenuationFixedName[] = "AtFixed";

static constexpr float kLightRange = 15.f;
static constexpr XboxMath::vector_t kSpotlightPosition{0.f, 0.f, -7.f, 1.f};
static constexpr XboxMath::vector_t kSpotlightDirection{0.f, 0.f, 1.f, 1.f};
static constexpr float kFalloffPenumbraDegrees = 30.f;
static constexpr float kFalloffUmbraDegrees = 10.f;
static constexpr float kAttenuation[] = {1.f, 0.f, 0.f};

#define FALLOFF_ENTRY(a, b, c)                                                                                         \
  LightingSpotlightTests::Spotlight(kSpotlightPosition, kSpotlightDirection, kFalloffPenumbraDegrees,                  \
                                    kFalloffUmbraDegrees, kAttenuation[0], kAttenuation[1], kAttenuation[2], (a), (b), \
                                    (c))

// Values observed by modifying falloff factor in D3DLIGHT8 struct.
static const LightingSpotlightTests::Spotlight kFalloffTests[] = {
    FALLOFF_ENTRY(0.f, -0.494592f, 1.494592f),         // falloff = 1.f (default)
    FALLOFF_ENTRY(-0.f, 1.f, 0.f),                     // falloff = 0.f
    FALLOFF_ENTRY(-0.000244f, 0.500122f, 0.499634f),   // falloff=0.5
    FALLOFF_ENTRY(-0.170208f, -0.855843f, 1.685635f),  // falloff=2.0
    FALLOFF_ENTRY(-0.706496f, -2.507095f, 2.800600f),  // falloff=10.0
    FALLOFF_ENTRY(-0.932112f, -3.097628f, 3.165516f),  // falloff=50.0
    FALLOFF_ENTRY(-0.986137f, -3.165117f, 3.178980f),  // falloff=250.0
    FALLOFF_ENTRY(-0.993286f, -2.953324f, 2.960038f),  // falloff=500.0
    FALLOFF_ENTRY(-0.996561f, -3.084043f, 3.087482f),  // falloff=1000.0
    FALLOFF_ENTRY(0.f, 0.f, 0.f)};

static constexpr float kFalloff[] = {-0.170208f, -0.855843f, 1.685635f};  // 2.0
#define PHI_THETA(phi, theta)                                                                             \
  LightingSpotlightTests::Spotlight(kSpotlightPosition, kSpotlightDirection, phi, theta, kAttenuation[0], \
                                    kAttenuation[1], kAttenuation[2], kFalloff[0], kFalloff[1], kFalloff[2])

static const LightingSpotlightTests::Spotlight kPhiThetaTests[] = {
    PHI_THETA(25.f, 5.f),     // Typical values
    PHI_THETA(25.f, 0.f),     // 0 theta
    PHI_THETA(10.f, 25.f),    // Inverted phi and theta
    PHI_THETA(10.f, 10.f),    // phi == theta
    PHI_THETA(0.f, 0.f),      // No application at all.
    PHI_THETA(-10.f, -10.f),  // negative values
};

#define ATTENUATION_ENTRY(a, b, c)                                                                    \
  LightingSpotlightTests::Spotlight(kSpotlightPosition, kSpotlightDirection, kFalloffPenumbraDegrees, \
                                    kFalloffUmbraDegrees, (a), (b), (c), kFalloff[0], kFalloff[1], kFalloff[2])

static const LightingSpotlightTests::Spotlight kAttenuationTests[] = {
    ATTENUATION_ENTRY(0.f, 0.f, 0.f),     ATTENUATION_ENTRY(0.5f, 0.f, 0.f),      ATTENUATION_ENTRY(1.f, 0.f, 0.f),
    ATTENUATION_ENTRY(2.f, 0.f, 0.f),     ATTENUATION_ENTRY(10.f, 0.f, 0.f),      ATTENUATION_ENTRY(0.05f, 0.25f, 0.f),
    ATTENUATION_ENTRY(0.05f, 0.f, 0.25f), ATTENUATION_ENTRY(0.05f, 0.05f, 0.05f),
};

static std::string MakeFalloffTestName(const char* prefix, const LightingSpotlightTests::Spotlight& light) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%s_%f_%f_%f", prefix, light.falloff[0], light.falloff[1], light.falloff[2]);
  return buf;
}

static std::string MakePhiThetaTestName(const char* prefix, const LightingSpotlightTests::Spotlight& light) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%s_%f_%f", prefix, light.phi, light.theta);
  return buf;
}

static std::string MakeAttenuationTestName(const char* prefix, const LightingSpotlightTests::Spotlight& light) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%s_%f_%f_%f", prefix, light.attenuation[0], light.attenuation[1], light.attenuation[2]);
  return buf;
}

LightingSpotlightTests::LightingSpotlightTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Lighting spotlight", config) {
  for (auto& light : kFalloffTests) {
    //    {
    //      auto name = MakeFalloffTestName(kFalloffName, light);
    //      tests_[name] = [this, name, light]() { Test(name, light); };
    //    }
    {
      auto name = MakeFalloffTestName(kFalloffFixedName, light);
      tests_[name] = [this, name, light]() { TestFixed(name, light); };
    }
  }

  for (auto& light : kPhiThetaTests) {
    //    {
    //      auto name = MakePhiThetaTestName(kPhiThetaName, light);
    //      tests_[name] = [this, name, light]() { Test(name, light); };
    //    }
    {
      auto name = MakePhiThetaTestName(kPhiThetaFixedName, light);
      tests_[name] = [this, name, light]() { TestFixed(name, light); };
    }
  }

  for (auto& light : kAttenuationTests) {
    //    {
    //      auto name = MakeAttenuationTestName(kAttenuationFixedName, light);
    //      tests_[name] = [this, name, light]() { Test(name, light); };
    //    }
    {
      auto name = MakeAttenuationTestName(kAttenuationFixedName, light);
      tests_[name] = [this, name, light]() { TestFixed(name, light); };
    }
  }
}

void LightingSpotlightTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_LIGHT_CONTROL, NV097_SET_LIGHT_CONTROL_V_ALPHA_FROM_MATERIAL_SPECULAR |
                                                  NV097_SET_LIGHT_CONTROL_V_LOCALEYE |
                                                  NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR);

    Pushbuffer::Push(NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_SPECULAR), 0);
    Pushbuffer::Push(NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_DIFFUSE), 0);
    Pushbuffer::Push(NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_SPECULAR), 0);
    Pushbuffer::End();
  }

  CreateGeometry();

  Pushbuffer::Begin();

  // Values taken from MechAssault
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x00, 0xBF56C33A);  // -0.838916
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x04, 0xC038C729);  // -2.887156
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x08, 0x4043165A);  // 3.048239
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x0c, 0xBF34DCE5);  // -0.706496
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x10, 0xC020743F);  // -2.507095
  Pushbuffer::Push(NV097_SET_SPECULAR_PARAMS + 0x14, 0x40333D06);  // 2.800600

  Pushbuffer::Push(NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  Pushbuffer::PushF(NV097_SET_SCENE_AMBIENT_COLOR, 0.01f, 0.01f, 0.01f);
  Pushbuffer::Push(NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
  Pushbuffer::PushF(NV097_SET_MATERIAL_ALPHA, 0.40f);
  Pushbuffer::End();
}

void LightingSpotlightTests::Deinitialize() {
  vertex_buffer_plane_.reset();
  vertex_buffer_cylinder_.reset();
  TestSuite::Deinitialize();
}

void LightingSpotlightTests::CreateGeometry() {
  // SET_COLOR_MATERIAL below causes per-vertex diffuse color to be ignored entirely.
  vector_t diffuse{1.f, 0.f, 1.f, 1.f};

  // However:
  // 1) the alpha from the specular value is added to the material alpha.
  // 2) the color is added to the computed vertex color if SEPARATE_SPECULAR is OFF
  vector_t specular{0.f, 0.4, 0.f, 0.25f};

  auto construct_model = [this](ModelBuilder& model, std::shared_ptr<VertexBuffer>& vertex_buffer) {
    vertex_buffer = host_.AllocateVertexBuffer(model.GetVertexCount());
    model.PopulateVertexBuffer(vertex_buffer);
  };

  {
    auto model = FlatMeshGridModel(diffuse, specular);
    construct_model(model, vertex_buffer_plane_);
  }
  {
    auto model = LightControlTestMeshCylinderModel(diffuse, specular);
    construct_model(model, vertex_buffer_cylinder_);
  }
}

LightingSpotlightTests::Spotlight::Spotlight(const XboxMath::vector_t& position, const XboxMath::vector_t& direction,
                                             float phi, float theta, float attenuation_1, float attenuation_2,
                                             float attenuation_3, float falloff_1, float falloff_2, float falloff_3)
    : phi(phi), theta(theta) {
  memcpy(this->position, position, sizeof(position));
  memcpy(this->direction, direction, sizeof(direction));
  attenuation[0] = attenuation_1;
  attenuation[1] = attenuation_2;
  attenuation[2] = attenuation_3;
  falloff[0] = falloff_1;
  falloff[1] = falloff_2;
  falloff[2] = falloff_3;
}

void LightingSpotlightTests::Spotlight::Commit(uint32_t light_index, const XboxMath::matrix4_t& view_matrix) const {
  vector_t transformed_position;
  VectorMultMatrix(position, view_matrix, transformed_position);
  transformed_position[3] = 1.f;

  vector_t normalized_direction{0.f, 0.f, 0.f, 1.f};
  VectorNormalize(direction, normalized_direction);

  // See spotlight handling code here:
  // https://github.com/xemu-project/xemu/blob/15bf68594f186c3df876fa7856b0d7a13e80f0e1/hw/xbox/nv2a/shaders.c#L653
  // and directx description here:
  // https://learn.microsoft.com/en-us/windows/uwp/graphics-concepts/attenuation-and-spotlight-factor
  float cos_half_theta = cosf(0.5f * DEG2RAD(theta));
  float cos_half_phi = cosf(0.5f * DEG2RAD(phi));

  float inv_scale = -1.f / (cos_half_theta - cos_half_phi);
  ScalarMultVector(normalized_direction, inv_scale);
  normalized_direction[3] = cos_half_phi * inv_scale;

  Pushbuffer::Begin();

  Pushbuffer::Push(SET_LIGHT(light_index, NV097_SET_LIGHT_AMBIENT_COLOR), 0, 0, 0);
  Pushbuffer::PushF(SET_LIGHT(light_index, NV097_SET_LIGHT_DIFFUSE_COLOR), 1.f, 0.f, 0.f);
  Pushbuffer::PushF(SET_LIGHT(light_index, NV097_SET_LIGHT_SPECULAR_COLOR), 0.f, 0.f, 0.5f);
  Pushbuffer::PushF(SET_LIGHT(light_index, NV097_SET_LIGHT_LOCAL_RANGE), kLightRange);
  Pushbuffer::Push3F(SET_LIGHT(light_index, NV097_SET_LIGHT_LOCAL_POSITION), transformed_position);
  Pushbuffer::Push3F(SET_LIGHT(light_index, NV097_SET_LIGHT_LOCAL_ATTENUATION), attenuation);
  Pushbuffer::Push3F(SET_LIGHT(light_index, NV097_SET_LIGHT_SPOT_FALLOFF), falloff);
  Pushbuffer::Push4F(SET_LIGHT(light_index, NV097_SET_LIGHT_SPOT_DIRECTION), normalized_direction);

  Pushbuffer::End();
}

static void DrawCheckerboardBackground(TestHost& host) {
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_LIGHTING_ENABLE, false);
  Pushbuffer::Push(NV097_SET_SPECULAR_ENABLE, false);
  Pushbuffer::End();

  host.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  host.SetTextureStageEnabled(0, true);
  host.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  static constexpr uint32_t kTextureSize = 256;
  static constexpr uint32_t kCheckerSize = 16;

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

static void SetupLighting(TestHost& host, uint32_t light_mode_bitvector) {
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

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_LIGHTING_ENABLE, true);
  Pushbuffer::Push(NV097_SET_SPECULAR_ENABLE, true);
  Pushbuffer::Push(NV097_SET_LIGHT_ENABLE_MASK, light_mode_bitvector);
  Pushbuffer::End();
}

void LightingSpotlightTests::Test(const std::string& name, const Spotlight& light) {}

void LightingSpotlightTests::TestFixed(const std::string& name, const Spotlight& light) {
  static constexpr uint32_t kBackgroundColor = 0xFF232623;
  host_.PrepareDraw(kBackgroundColor);

  DrawCheckerboardBackground(host_);

  uint32_t light_mode_bitvector = 0;
  light.Commit(0, host_.GetFixedFunctionModelViewMatrix());
  light_mode_bitvector |= LIGHT_MODE(0, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_SPOT);

  SetupLighting(host_, light_mode_bitvector);

  for (auto& vb : {
           vertex_buffer_plane_,
           vertex_buffer_cylinder_,
       }) {
    host_.SetVertexBuffer(vb);
    host_.DrawArrays(host_.POSITION | host_.NORMAL | host_.DIFFUSE | host_.SPECULAR);
  }

  pb_print("%s", name.c_str());
  pb_draw_text_screen();

  FinishDraw(name);
}
