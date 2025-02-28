#include "test_suite.h"

#include <chrono>
#include <fstream>
#include <sstream>

#include "configure.h"
#include "debug_output.h"
#include "logger.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "shaders/pixel_shader_program.h"
#include "test_host.h"
#include "texture_format.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"

using namespace XboxMath;

static constexpr char kFTPLogProgressFilename[] = "nxdk_pgraph_tests_progress.log";

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

TestSuite::TestSuite(TestHost& host, std::string output_dir, std::string suite_name, const Config& config)
    : host_(host),
      output_dir_(std::move(output_dir)),
      suite_name_(std::move(suite_name)),
      pgraph_diff_(false, config.enable_progress_log),
      enable_progress_log_{config.enable_progress_log},
      enable_pgraph_region_diff_{config.enable_pgraph_region_diff},
      delay_milliseconds_between_tests_{config.delay_milliseconds_between_tests},
      ftp_logger_{config.ftp_logger} {
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

void TestSuite::DisableTests(const std::set<std::string>& tests_to_skip) {
  for (auto& name : tests_to_skip) {
    tests_.erase(name);
  }
}

void TestSuite::Run(const std::string& test_name) {
  auto it = tests_.find(test_name);
  if (it == tests_.end()) {
    ASSERT(!"Invalid test name");
  }

  SetupTest();
  auto start_time = LogTestStart(test_name);
  it->second();
  auto duration = LogTestEnd(test_name, start_time);
  TearDownTest();

  if (ftp_logger_) {
    if (!ftp_logger_->Connect()) {
      PrintMsg("FTP connect failed, aborting\n");
    } else {
      PrintMsg("Saving progress to FTP server...\n");

      if (allow_saving_) {
        for (auto& put_operation : ftp_logger_->send_file_queue()) {
          std::stringstream message;
          if (!ftp_logger_->PutFile(put_operation.first, put_operation.second)) {
            message << "- MISSING: \"" << put_operation.second << "\"\n";
          } else {
            message << "- OUTPUT: \"" << put_operation.second << "\"\n";
          }

          if (!ftp_logger_->AppendFile(kFTPLogProgressFilename, message.str())) {
            PrintMsg("Failed to store progress log to FTP server with artifact info!\n");
          }
          message.clear();
        }
      }
      ftp_logger_->ClearSendQueue();

      std::stringstream message;
      message << "END: \"" << suite_name_ << "::" << test_name << "\" IN " << duration << " MS\n";
      if (!ftp_logger_->AppendFile(kFTPLogProgressFilename, message.str())) {
        PrintMsg("Failed to store progress log to FTP server!\n");
      }
    }
  }
}

void TestSuite::RunAll() {
  auto names = TestNames();
  for (const auto& test_name : names) {
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

  {
    auto p = pb_begin();
    p = pb_push4f(p, NV097_SET_EYE_POSITION, 0.0f, 0.0f, 0.0f, 1.0f);
    p = pb_push1(p, NV097_SET_ZMIN_MAX_CONTROL,
                 NV097_SET_ZMIN_MAX_CONTROL_CULL_NEAR_FAR_EN_TRUE | NV097_SET_ZMIN_MAX_CONTROL_ZCLAMP_EN_CULL |
                     NV097_SET_ZMIN_MAX_CONTROL_CULL_IGNORE_W_FALSE);
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
    p = pb_push3(p, NV097_SET_SCENE_AMBIENT_COLOR, 0x0, 0x0, 0x0);
    p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
    p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.0f);
    p = pb_push1f(p, NV097_SET_BACK_MATERIAL_ALPHA, 0.f);

    p = pb_push1(p, NV097_SET_LIGHT_TWO_SIDE_ENABLE, false);
    p = pb_push1(p, NV097_SET_FRONT_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);
    p = pb_push1(p, NV097_SET_BACK_POLYGON_MODE, NV097_SET_FRONT_POLYGON_MODE_V_FILL);

    p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_SPECULAR), 0);
    p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_DIFFUSE), 0xFFFFFFFF);
    p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_SPECULAR), 0);

    p = pb_push1(p, NV097_SET_POINT_PARAMS_ENABLE, false);
    p = pb_push1(p, NV097_SET_POINT_SMOOTH_ENABLE, false);
    p = pb_push1(p, NV097_SET_POINT_SIZE, 8);

    p = pb_push1(p, NV097_SET_DOT_RGBMAPPING, 0);

    p = pb_push1(p, NV097_SET_SHADE_MODEL, NV097_SET_SHADE_MODEL_SMOOTH);
    p = pb_push1(p, NV097_SET_FLAT_SHADE_OP, NV097_SET_FLAT_SHADE_OP_VERTEX_LAST);

    p = pb_push1(p, NV097_SET_OCCLUDE_ZSTENCIL_EN, 3);
    pb_end(p);
  }

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
                          false, /*specular_add_invert_r0*/ false, /* specular_add_invert_v1*/ false,
                          /* specular_clamp */ true);

  host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE, TestHost::STAGE_NONE);

  while (pb_busy()) {
    /* Wait for completion... */
  }

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
    stage.SetColorKeyMode(TextureStage::CKM_DISABLE);
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
    auto p = pb_begin();
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
    pb_end(p);
  }

  {
    auto p = pb_begin();
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

    // Prevent bleedover between tests by setting default values.
    p = pb_push4f(p, NV097_SET_WEIGHT4F, 0.f, 0.f, 0.f, 0.f);
    p = pb_push3f(p, NV097_SET_NORMAL3F, 0.f, 0.f, 0.f);
    p = pb_push1(p, NV097_SET_DIFFUSE_COLOR4I, 0x00000000);
    p = pb_push1(p, NV097_SET_SPECULAR_COLOR4I, 0x00000000);
    p = pb_push1f(p, NV097_SET_SPECULAR_COLOR4I, 0.f);
    p = pb_push1(p, NV097_SET_POINT_SIZE, 0x8);
    p = pb_push4f(p, NV097_SET_TEXCOORD0_4F, 0.f, 0.f, 0.f, 0.f);
    p = pb_push4f(p, NV097_SET_TEXCOORD1_4F, 0.f, 0.f, 0.f, 0.f);
    p = pb_push4f(p, NV097_SET_TEXCOORD2_4F, 0.f, 0.f, 0.f, 0.f);
    p = pb_push4f(p, NV097_SET_TEXCOORD3_4F, 0.f, 0.f, 0.f, 0.f);
    pb_end(p);
  }

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

  if (enable_pgraph_region_diff_) {
    pgraph_diff_.Capture();
  }

  TagNV2ATrace(2);
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_FOG_ENABLE, true);
    p = pb_push1(p, NV097_SET_FOG_ENABLE, false);
    pb_end(p);
  }
  TagNV2ATrace(4);
}

void TestSuite::TagNV2ATrace(uint32_t num_nops) {
  ASSERT(num_nops < 32 && "Too many nops in TagNV2ATrace");
  auto p = pb_begin();
  for (auto i = 0; i < num_nops; ++i) p = pb_push1(p, NV097_NO_OPERATION, 0x00);
  pb_end(p);
}

void TestSuite::Deinitialize() {
  if (enable_pgraph_region_diff_) {
    pgraph_diff_.DumpDiff();
  }
}

void TestSuite::SetupTest() {}

void TestSuite::TearDownTest() {}

std::chrono::steady_clock::time_point TestSuite::LogTestStart(const std::string& test_name) {
  PrintMsg("Starting %s::%s\n", suite_name_.c_str(), test_name.c_str());

  if (allow_saving_) {
    if (enable_progress_log_) {
      Logger::Log() << "Starting " << suite_name_ << "::" << test_name << std::endl;
    }
  }

  if (ftp_logger_) {
    if (!ftp_logger_->Connect()) {
      PrintMsg("FTP connect failed, aborting\n");
    } else {
      PrintMsg("Saving start message to FTP server...\n");
      std::stringstream message;
      message << "START: \"" << suite_name_ << "::" << test_name << "\"\n";
      if (!ftp_logger_->AppendFile(kFTPLogProgressFilename, message.str())) {
        PrintMsg("Failed to store progress log to FTP server!\n");
      }
    }
  }

  if (delay_milliseconds_between_tests_) {
    Sleep(delay_milliseconds_between_tests_);
  }

  return std::chrono::high_resolution_clock::now();
}

long TestSuite::LogTestEnd(const std::string& test_name,
                           const std::chrono::steady_clock::time_point& start_time) const {
  auto now = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
  auto elapsed = static_cast<long>((duration.count() & 0xFFFFFFFF));

  PrintMsg("  Completed '%s' in %lums\n", test_name.c_str(), elapsed);

  if (enable_progress_log_ && allow_saving_) {
    Logger::Log() << "  Completed '" << test_name << "' in " << elapsed << "ms" << std::endl;
  }

  return elapsed;
}
