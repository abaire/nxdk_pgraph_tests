#include "fog_gen_tests.h"

#include <light.h>
#include <models/flat_mesh_grid_model.h>
#include <pbkit/pbkit.h>
#include <shaders/perspective_vertex_shader.h>

#include "debug_output.h"
#include "test_host.h"

// clang-format off
static constexpr uint32_t kVertexShader[] = {
#include "projection_vertex_shader_no_lighting_explicit_fog.vshinc"



};
// clang-format on

static constexpr float kFogStart = 0.0f;
static constexpr float kFogEnd = 200.0f;

static constexpr uint32_t kFogColor = 0xDD0000FF;

static constexpr vector_t kDiffuse{0.f, 0.f, 1.f, 1.f};

// Specular alpha is used for NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA
static constexpr vector_t kSpecularUL{0.f, 0.f, 0.f, 0.f};
static constexpr vector_t kSpecularUR{0.f, 0.f, 0.f, 1.f};
static constexpr vector_t kSpecularLR{0.f, 0.f, 0.f, 0.75f};
static constexpr vector_t kSpecularLL{0.f, 0.f, 0.f, 0.25f};

static constexpr uint32_t kFogModes[] = {
    NV097_SET_FOG_MODE_V_LINEAR,  NV097_SET_FOG_MODE_V_EXP,      NV097_SET_FOG_MODE_V_EXP2,
    NV097_SET_FOG_MODE_V_EXP_ABS, NV097_SET_FOG_MODE_V_EXP2_ABS, NV097_SET_FOG_MODE_V_LINEAR_ABS,
};

static constexpr uint32_t kFogGenModes[] = {
    NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA, NV097_SET_FOG_GEN_MODE_V_RADIAL, NV097_SET_FOG_GEN_MODE_V_PLANAR,
    NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR, NV097_SET_FOG_GEN_MODE_V_FOG_X,
};

static std::string MakeTestName(uint32_t fog_mode, uint32_t gen_mode, bool use_fixed_function) {
  std::string ret = "FogGen";
  ret += use_fixed_function ? "_FF" : "_VS";

  switch (fog_mode) {
    case NV097_SET_FOG_MODE_V_LINEAR:
      ret += "-linear";
      break;
    case NV097_SET_FOG_MODE_V_EXP:
      ret += "-exp";
      break;
    case NV097_SET_FOG_MODE_V_EXP2:
      ret += "-exp2";
      break;
    case NV097_SET_FOG_MODE_V_EXP_ABS:
      ret += "-exp_abs";
      break;
    case NV097_SET_FOG_MODE_V_EXP2_ABS:
      ret += "-exp2_abs";
      break;
    case NV097_SET_FOG_MODE_V_LINEAR_ABS:
      ret += "-linear_abs";
      break;
    default:
      ASSERT(!"Unhandled fog mode");
  }

  switch (gen_mode) {
    case NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA:
      ret += "-spec_alpha";
      break;
    case NV097_SET_FOG_GEN_MODE_V_RADIAL:
      ret += "-radial";
      break;
    case NV097_SET_FOG_GEN_MODE_V_PLANAR:
      ret += "-planar";
      break;
    case NV097_SET_FOG_GEN_MODE_V_ABS_PLANAR:
      ret += "-abs_planar";
      break;
    case NV097_SET_FOG_GEN_MODE_V_FOG_X:
      ret += "-fog_x";
      break;
    default:
      ASSERT(!"Unhandled fog gen mode");
  }

  return std::move(ret);
}

FogGenTests::FogGenTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Fog gen", config) {
  for (auto fixed_function : {false, true}) {
    for (auto fog_gen_mode : kFogGenModes) {
      for (auto fog_mode : kFogModes) {
        std::string name = MakeTestName(fog_mode, fog_gen_mode, fixed_function);
        tests_[name] = [this, name, fog_mode, fog_gen_mode, fixed_function]() {
          Test(name, fog_mode, fog_gen_mode, fixed_function);
        };
      }
    }
  }
}

void FogGenTests::Initialize() {
  TestSuite::Initialize();

  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_FOG_ENABLE, true);
    p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);
    p = pb_push1(p, NV097_SET_LIGHT_CONTROL, NV097_SET_LIGHT_CONTROL_V_SEPARATE_SPECULAR);
    pb_end(p);
  }

  // Just pass through the diffuse color.
  host_.SetCombinerControl(1);
  host_.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);
  host_.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);

  // Mix between the output pixel color and the fog color using the fog alpha value.
  host_.SetFinalCombiner0(TestHost::SRC_FOG, true, false, TestHost::SRC_DIFFUSE, false, false, TestHost::SRC_FOG, false,
                          false);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_DIFFUSE,
                          true, false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);
}

static std::shared_ptr<PerspectiveVertexShader> SetupVertexShader(TestHost& host) {
  // Note: This could just as easily use the PassthroughVertexShader and specify all values via vertex vals, but a
  // transforming shader is used to better reproduce observed issues with Otogi:
  // https://github.com/xemu-project/xemu/issues/365
  float depth_buffer_max_value = host.GetMaxDepthBufferValue();
  auto shader = std::make_shared<PerspectiveVertexShader>(host.GetFramebufferWidth(), host.GetFramebufferHeight(), 0.0f,
                                                          depth_buffer_max_value, M_PI * 0.25f, 1.0f, 200.0f);
  shader->SetShaderOverride(kVertexShader, sizeof(kVertexShader));
  shader->SetLightingEnabled(false);
  shader->SetTransposeOnUpload();
  vector_t camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
  vector_t camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
  shader->LookAt(camera_position, camera_look_at);

  host.SetVertexShaderProgram(shader);

  return shader;
}

static void SetupFogParams(uint32_t fog_mode, uint32_t fog_gen_mode) {
  auto p = pb_begin();
  // Note: Fog color is ABGR and not ARGB
  p = pb_push1(p, NV097_SET_FOG_COLOR, kFogColor);

  p = pb_push1(p, NV097_SET_FOG_GEN_MODE, fog_gen_mode);
  p = pb_push1(p, NV097_SET_FOG_MODE, fog_mode);

  p = pb_push4f(p, NV097_SET_FOG_PLANE, 0.f, 0.f, 2.f, 0.f);

  // Exponential parameters.
  const float fog_density = 0.025f;

  static constexpr float LN_256 = 5.5452f;
  static constexpr float SQRT_LN_256 = 2.3548f;

  float bias_param = 0.0f;
  float multiplier_param = 1.0f;
  float quadratic_param = 0.f;

  // NOTE: Specular alpha only has a range of 0..1 so the fog params need to be adjusted for there to be any visible
  // effect.

  switch (fog_mode) {
    case NV097_SET_FOG_MODE_V_LINEAR:
    case NV097_SET_FOG_MODE_V_LINEAR_ABS:
      if (fog_gen_mode == NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA) {
        // bias_param + alpha * multiplier_param - 1.0
        // Map to the range -1 to 1
        bias_param = 0.f;
        multiplier_param = 2.f;
      } else {
        multiplier_param = -1.0f / (kFogEnd - kFogStart);
        bias_param = 1.0f + -kFogEnd * multiplier_param;
      }
      break;

    case NV097_SET_FOG_MODE_V_EXP:
    case NV097_SET_FOG_MODE_V_EXP_ABS:
      if (fog_gen_mode == NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA) {
        // (bias_param + 2^ (alpha * multiplier_param * 16)) - 1.5
        // Map to the range 0..1 using xemu's (apparently incorrect) calculation
        bias_param = 0.5f;
        multiplier_param = 1.f / 16.f;
      } else {
        bias_param = 1.5f;
        multiplier_param = -fog_density / (2.0f * LN_256);
      }
      break;

    case NV097_SET_FOG_MODE_V_EXP2:
    case NV097_SET_FOG_MODE_V_EXP2_ABS:
      if (fog_gen_mode == NV097_SET_FOG_GEN_MODE_V_SPEC_ALPHA) {
        // (bias_param + 2^ (-alpha * alpha * multiplier_param^2 * 32)) - 1.5
        // Map to the range 0..1 using xemu's (apparently incorrect) calculation
        bias_param = 0.5f;
        multiplier_param = sqrt(1.f / 32.f);
      } else {
        bias_param = 1.5f;
        multiplier_param = -fog_density / (2.0f * SQRT_LN_256);
      }
      break;

    default:
      break;
  }

  p = pb_push3f(p, NV097_SET_FOG_PARAMS, bias_param, multiplier_param, quadratic_param);

  pb_end(p);
}

void FogGenTests::Test(const std::string& name, uint32_t fog_mode, uint32_t fog_gen_mode, bool use_fixed_function) {
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  std::shared_ptr<PerspectiveVertexShader> shader;
  if (use_fixed_function) {
    host_.SetVertexShaderProgram(nullptr);
  } else {
    shader = SetupVertexShader(host_);
  }
  static constexpr auto kFogValueIndex = 120 - PerspectiveVertexShader::kShaderUserConstantOffset;

  SetupFogParams(fog_mode, fog_gen_mode);

  static constexpr uint32_t kBackgroundColor = 0xFF232623;
  host_.PrepareDraw(kBackgroundColor);

  static constexpr float kQuadWidth = 22.f;
  static constexpr float kQuadHeight = 22.f;
  static constexpr float kQuadZRightInc = 1.f;

  auto unproject = [this, shader, use_fixed_function](vector_t& world_point, float x, float y, float z) {
    vector_t screen_point{x, y, z, 1.f};
    if (use_fixed_function) {
      host_.UnprojectPoint(world_point, screen_point, z);
    } else {
      shader->UnprojectPoint(world_point, screen_point, z);
    }
  };

  auto draw_quad = [this, unproject](float left, float top, float z) {
    const auto right = left + kQuadWidth;
    const auto bottom = top + kQuadHeight;

    host_.Begin(TestHost::PRIMITIVE_QUADS);

    vector_t world_point{0.f, 0.f, 0.f, 1.f};
    host_.SetNormal(0.f, 0.f, -1.f);
    host_.SetDiffuse(kDiffuse);

    host_.SetSpecular(kSpecularUL);
    unproject(world_point, left, top, z);
    host_.SetVertex(world_point);

    host_.SetSpecular(kSpecularUR);
    unproject(world_point, right, top, z + kQuadZRightInc);
    host_.SetVertex(world_point);

    host_.SetSpecular(kSpecularLR);
    unproject(world_point, right, bottom, z + kQuadZRightInc);
    host_.SetVertex(world_point);

    host_.SetSpecular(kSpecularLL);
    unproject(world_point, left, bottom, z);
    host_.SetVertex(world_point);

    host_.End();
  };

  static constexpr float kTopPadding = 70.f;
  static constexpr float kSidePadding = 48.f;
  static constexpr float kQuadSpacingH = 2.f;
  static constexpr float kQuadSpacingV = 2.f;

  const float right = host_.GetFramebufferWidthF() - (kSidePadding + kQuadWidth);
  const float bottom = host_.GetFramebufferHeightF() - kQuadHeight;
  float left = kSidePadding;
  float top = kTopPadding;

  static constexpr float kQuadZ = -6.f;
  static constexpr float kZInc = 0.5f;

  float fog_x = 50.f;
  static constexpr float kFogXDec = 0.2f;

  float z = kQuadZ;

  while (top < bottom) {
    if (use_fixed_function) {
      host_.SetFogCoord(fog_x);
    } else {
      // Set #fog_value
      shader->SetUniformF(kFogValueIndex, fog_x, 0.f, 0.f, 0.f);
      shader->PrepareDraw();
    }

    draw_quad(left, top, z);

    left += kQuadWidth + kQuadSpacingH;
    if (left >= right) {
      left = kSidePadding;
      top += kQuadHeight + kQuadSpacingV;
    }

    z += kZInc;
    fog_x -= kFogXDec;
  }

  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}
