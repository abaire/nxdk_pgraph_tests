#include "test_suite.h"

#include <fstream>

#include "configure.h"
#include "debug_output.h"
#include "logger.h"
#include "pbkit_ext.h"
#include "shaders/pixel_shader_program.h"
#include "test_host.h"
#include "texture_format.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"

using namespace XboxMath;

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

TestSuite::TestSuite(TestHost& host, std::string output_dir, std::string suite_name)
    : host_(host), output_dir_(std::move(output_dir)), suite_name_(std::move(suite_name)), pgraph_diff_(false) {
  output_dir_ += "\\";
  output_dir_ += suite_name_;
  std::replace(output_dir_.begin(), output_dir_.end(), ' ', '_');
}

std::vector<std::string> TestSuite::TestNames() const {
  std::vector<std::string> ret;
  ret.reserve(tests_.size());

  for (auto& kv : tests_) {
    ret.push_back(kv.first);
  }
  return std::move(ret);
}

void TestSuite::DisableTests(const std::vector<std::string>& tests_to_skip) {
  for (auto& name : tests_to_skip) {
    tests_.erase(name);
  }
}

void TestSuite::Run(const std::string& test_name) {
  auto it = tests_.find(test_name);
  if (it == tests_.end()) {
    ASSERT(!"Invalid test name");
  }

  auto start_time = LogTestStart(test_name);
  it->second();
  LogTestEnd(test_name, start_time);
}

void TestSuite::RunAll() {
  auto names = TestNames();
  for (const auto& test_name : names) {
    if (suspected_crashes_.find(test_name) != suspected_crashes_.end()) {
      switch (suspected_crash_handling_mode_) {
        case SuspectedCrashHandling::RUN_ALL:
          break;

        case SuspectedCrashHandling::SKIP_ALL:
          continue;

        case SuspectedCrashHandling::ASK:
          // TODO: IMPLEMENT ASK MODE FOR CRASH HANDLING
          ASSERT(!"TODO: IMPLEMENT ME");
          break;
      }
    }

    Run(test_name);
  }
}

void TestSuite::SetDefaultTextureFormat() const {
  const TextureFormatInfo& texture_format = GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8);
  host_.SetTextureFormat(texture_format, 0);
  host_.SetDefaultTextureParams(0);
  host_.SetTextureFormat(texture_format, 1);
  host_.SetDefaultTextureParams(1);
  host_.SetTextureFormat(texture_format, 2);
  host_.SetDefaultTextureParams(2);
  host_.SetTextureFormat(texture_format, 3);
  host_.SetDefaultTextureParams(3);
}

void TestSuite::Initialize() {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SURFACE_PITCH,
               SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                   SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  p = pb_push1(p, NV097_SET_SURFACE_CLIP_HORIZONTAL, host_.GetFramebufferWidth() << 16);
  p = pb_push1(p, NV097_SET_SURFACE_CLIP_VERTICAL, host_.GetFramebufferHeight() << 16);

  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, false);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, false);
  p = pb_push1(p, NV097_SET_LIGHT_CONTROL, 0x20001);
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_OFF);
  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.0f);

  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_LIGHT_MODEL_TWO_SIDE_ENABLE, 0);
  p = pb_push1(p, NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
  p = pb_push1(p, NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);

  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x10, 0);           // Specular
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x1C, 0xFFFFFFFF);  // Back diffuse
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + 0x20, 0);           // Back specular

  p = pb_push1(p, NV097_SET_POINT_PARAMS_ENABLE, false);
  p = pb_push1(p, NV097_SET_POINT_SMOOTH_ENABLE, false);
  p = pb_push1(p, NV097_SET_POINT_SIZE, 8);

  p = pb_push1(p, NV097_SET_DOT_RGBMAPPING, 0);

  p = pb_push1(p, NV097_SET_SHADE_MODEL, NV097_SET_SHADE_MODEL_SMOOTH);
  pb_end(p);

  host_.SetWindowClipExclusive(false);
  // Note, setting the first clip region will cause the hardware to also set all subsequent regions.
  host_.SetWindowClip(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  host_.SetBlend();

  host_.ClearInputColorCombiners();
  host_.ClearInputAlphaCombiners();
  host_.ClearOutputColorCombiners();
  host_.ClearOutputAlphaCombiners();

  host_.SetCombinerControl(1);
  host_.SetInputColorCombiner(0, TestHost::SRC_DIFFUSE, false, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);
  host_.SetInputAlphaCombiner(0, TestHost::SRC_DIFFUSE, true, TestHost::MAP_UNSIGNED_IDENTITY, TestHost::SRC_ZERO,
                              false, TestHost::MAP_UNSIGNED_INVERT);

  host_.SetOutputColorCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);
  host_.SetOutputAlphaCombiner(0, TestHost::DST_DISCARD, TestHost::DST_DISCARD, TestHost::DST_R0);

  host_.SetFinalCombiner0(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false,
                          false, TestHost::SRC_R0);
  host_.SetFinalCombiner1(TestHost::SRC_ZERO, false, false, TestHost::SRC_ZERO, false, false, TestHost::SRC_R0, true,
                          false, false, false, true);

  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE);

  while (pb_busy()) {
    /* Wait for completion... */
  }

  p = pb_begin();

  matrix4_t identity_matrix;
  MatrixSetIdentity(identity_matrix);
  for (auto i = 0; i < 4; ++i) {
    auto& stage = host_.GetTextureStage(i);
    stage.SetUWrap(TextureStage::WRAP_CLAMP_TO_EDGE, false);
    stage.SetVWrap(TextureStage::WRAP_CLAMP_TO_EDGE, false);
    stage.SetPWrap(TextureStage::WRAP_CLAMP_TO_EDGE, false);
    stage.SetQWrap(false);

    stage.SetEnabled(false);
    stage.SetCubemapEnable(false);
    stage.SetFilter();
    stage.SetAlphaKillEnable(false);
    stage.SetLODClamp(0, 4095);

    stage.SetTextureMatrixEnable(false);
    stage.SetTextureMatrix(identity_matrix);

    stage.SetTexgenS(TextureStage::TG_DISABLE);
    stage.SetTexgenT(TextureStage::TG_DISABLE);
    stage.SetTexgenR(TextureStage::TG_DISABLE);
    stage.SetTexgenQ(TextureStage::TG_DISABLE);
  }

  // TODO: Set up with TextureStage instances in host_.
  {
    uint32_t address = NV097_SET_TEXTURE_ADDRESS;
    uint32_t control = NV097_SET_TEXTURE_CONTROL0;
    uint32_t filter = NV097_SET_TEXTURE_FILTER;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);

    address += 0x40;
    control += 0x40;
    filter += 0x40;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);

    address += 0x40;
    control += 0x40;
    filter += 0x40;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);

    address += 0x40;
    control += 0x40;
    filter += 0x40;
    p = pb_push1(p, address, 0x10101);
    p = pb_push1(p, control, 0x3ffc0);
    p = pb_push1(p, filter, 0x1012000);
  }

  p = pb_push1(p, NV097_SET_FOG_ENABLE, false);
  p = pb_push4(p, NV097_SET_TEXTURE_MATRIX_ENABLE, 0, 0, 0, 0);

  p = pb_push1(p, NV097_SET_FRONT_FACE, NV097_SET_FRONT_FACE_V_CW);
  p = pb_push1(p, NV097_SET_CULL_FACE, NV097_SET_CULL_FACE_V_BACK);
  p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, true);

  p = pb_push1(p, NV097_SET_COLOR_MASK,
               NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE | NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
                   NV097_SET_COLOR_MASK_RED_WRITE_ENABLE | NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE);

  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, true);
  p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, true);

  p = pb_push1(p, NV097_SET_NORMALIZATION_ENABLE, false);
  pb_end(p);

  host_.SetDefaultViewportAndFixedFunctionMatrices();
  host_.SetDepthBufferFloatMode(false);

  host_.SetVertexShaderProgram(nullptr);
  SetDefaultTextureFormat();
  host_.SetTextureStageEnabled(0, false);
  host_.SetTextureStageEnabled(1, false);
  host_.SetTextureStageEnabled(2, false);
  host_.SetTextureStageEnabled(3, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetShaderStageInput(0, 0);

  PixelShaderProgram::LoadUntexturedPixelShader();
  PixelShaderProgram::DisablePixelShader();

  host_.ClearAllVertexAttributeStrideOverrides();

#ifdef ENABLE_PGRAPH_REGION_DIFF
  pgraph_diff_.Capture();
#endif
}

void TestSuite::Deinitialize() {
#ifdef ENABLE_PGRAPH_REGION_DIFF
  pgraph_diff_.DumpDiff();
#endif
}

std::chrono::steady_clock::time_point TestSuite::LogTestStart(const std::string& test_name) {
  PrintMsg("Starting %s::%s\n", suite_name_.c_str(), test_name.c_str());

#ifdef ENABLE_PROGRESS_LOG
  if (allow_saving_) {
    Logger::Log() << "Starting " << suite_name_ << "::" << test_name << std::endl;
  }
#endif

  return std::chrono::high_resolution_clock::now();
}

void TestSuite::LogTestEnd(const std::string& test_name, std::chrono::steady_clock::time_point start_time) {
  auto now = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();

  PrintMsg("  Completed '%s' in %lums\n", test_name.c_str(), elapsed);

#ifdef ENABLE_PROGRESS_LOG
  if (allow_saving_) {
    Logger::Log() << "  Completed '" << test_name << "' in " << elapsed << "ms" << std::endl;
  }
#endif
}
