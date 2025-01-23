/*
 * This sample provides a very basic demonstration of 3D rendering on the Xbox,
 * using pbkit. Based on the pbkit demo sources.
 */

#include <SDL_image.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#include <hal/debug.h>
#pragma clang diagnostic pop
#include <hal/fileio.h>
#include <hal/video.h>
#include <nxdk/mount.h>
#include <pbkit/pbkit.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#include <windows.h>
#pragma clang diagnostic pop

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "configure.h"
#include "debug_output.h"
#include "logger.h"
#include "runtime_config.h"
#include "test_driver.h"
#include "test_host.h"
#include "tests/antialiasing_tests.h"
#include "tests/attribute_carryover_tests.h"
#include "tests/attribute_explicit_setter_tests.h"
#include "tests/attribute_float_tests.h"
#include "tests/blend_tests.h"
#include "tests/clear_tests.h"
#include "tests/color_key_tests.h"
#include "tests/color_mask_blend_tests.h"
#include "tests/color_zeta_disable_tests.h"
#include "tests/color_zeta_overlap_tests.h"
#include "tests/combiner_tests.h"
#include "tests/depth_format_fixed_function_tests.h"
#include "tests/depth_format_tests.h"
#include "tests/dma_corruption_around_surface_tests.h"
#include "tests/edge_flag_tests.h"
#include "tests/fog_tests.h"
#include "tests/front_face_tests.h"
#include "tests/image_blit_tests.h"
#include "tests/inline_array_size_mismatch.h"
#include "tests/lighting_normal_tests.h"
#include "tests/lighting_two_sided_tests.h"
#include "tests/line_width_tests.h"
#include "tests/material_alpha_tests.h"
#include "tests/material_color_source_tests.h"
#include "tests/material_color_tests.h"
#include "tests/null_surface_tests.h"
#include "tests/overlapping_draw_modes_tests.h"
#include "tests/pvideo_tests.h"
#include "tests/set_vertex_data_tests.h"
#include "tests/shade_model_tests.h"
#include "tests/smoothing_tests.h"
#include "tests/stencil_tests.h"
#include "tests/stipple_tests.h"
#include "tests/surface_clip_tests.h"
#include "tests/surface_pitch_tests.h"
#include "tests/texgen_matrix_tests.h"
#include "tests/texgen_tests.h"
#include "tests/texture_border_tests.h"
#include "tests/texture_cpu_update_tests.h"
#include "tests/texture_cubemap_tests.h"
#include "tests/texture_format_dxt_tests.h"
#include "tests/texture_format_tests.h"
#include "tests/texture_framebuffer_blit_tests.h"
#include "tests/texture_matrix_tests.h"
#include "tests/texture_render_target_tests.h"
#include "tests/texture_render_update_in_place_tests.h"
#include "tests/texture_shadow_comparator_tests.h"
#include "tests/texture_signed_component_tests.h"
#include "tests/three_d_primitive_tests.h"
#include "tests/two_d_line_tests.h"
#include "tests/vertex_shader_independence_tests.h"
#include "tests/vertex_shader_rounding_tests.h"
#include "tests/vertex_shader_swizzle_tests.h"
#include "tests/viewport_tests.h"
#include "tests/volume_texture_tests.h"
#include "tests/w_param_tests.h"
#include "tests/window_clip_tests.h"
#include "tests/z_min_max_control_tests.h"
#include "tests/zero_stride_tests.h"

static constexpr int kDelayOnFailureMilliseconds = 4000;

static constexpr int kFramebufferWidth = 640;
static constexpr int kFramebufferHeight = 480;
static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 256;

static constexpr const char* kLogFileName = "pgraph_progress_log.txt";

const UCHAR kSMCSlaveAddress = 0x20;
const UCHAR kSMCRegisterPower = 0x02;
const UCHAR kSMCPowerShutdown = 0x80;

static bool EnsureDriveMounted(char drive_letter);
static bool LoadConfig(RuntimeConfig& config, std::vector<std::string>& errors);
#ifdef DUMP_CONFIG_FILE
static void DumpConfig(RuntimeConfig& config, std::vector<std::shared_ptr<TestSuite>>& test_suites);
#endif
static void RunTests(RuntimeConfig& config, TestHost& host, std::vector<std::shared_ptr<TestSuite>>& test_suites);
static void RegisterSuites(TestHost& host, RuntimeConfig& config, std::vector<std::shared_ptr<TestSuite>>& test_suites,
                           const std::string& output_directory);
static void Shutdown();
#ifdef ENABLE_INTERACTIVE_CRASH_AVOIDANCE
static bool DiscoverHistoricalCrashes(const std::string& log_file_path,
                                      std::map<std::string, std::set<std::string>>& crashes);
#endif

extern "C" __cdecl int automount_d_drive(void);

/* Main program function */
int main() {
  automount_d_drive();
  debugPrint("Set video mode");
  XVideoSetMode(kFramebufferWidth, kFramebufferHeight, 32, REFRESH_DEFAULT);

  // Reserve 4 times the size of the default framebuffers to allow for antialiasing.
  pb_set_fb_size_multiplier(4);
  int status = pb_init();
  if (status) {
    debugPrint("pb_init Error %d\n", status);
    pb_show_debug_screen();
    Sleep(kDelayOnFailureMilliseconds);
    return 1;
  }

  if (SDL_Init(SDL_INIT_GAMECONTROLLER)) {
    debugPrint("Failed to initialize SDL_GAMECONTROLLER.");
    debugPrint("%s", SDL_GetError());
    pb_show_debug_screen();
    Sleep(kDelayOnFailureMilliseconds);
    return 1;
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    debugPrint("Failed to initialize SDL_image PNG mode.");
    pb_show_debug_screen();
    Sleep(kDelayOnFailureMilliseconds);
    return 1;
  }

  RuntimeConfig config;
#ifndef DUMP_CONFIG_FILE
  {
    std::vector<std::string> errors;
    if (!LoadConfig(config, errors)) {
      debugPrint("Failed to load config, using default values.\n");
      for (auto& err : errors) {
        debugPrint("%s\n", err.c_str());
      }
      pb_show_debug_screen();
    }
  }
#endif  // DUMP_CONFIG_FILE

  if (!EnsureDriveMounted(config.output_directory_path().front())) {
    debugPrint("Failed to mount %s, please make sure output directory is on a writable drive.\n",
               config.output_directory_path().c_str());
    pb_show_debug_screen();
    Sleep(kDelayOnFailureMilliseconds);
    return 1;
  };

  TestHost::EnsureFolderExists(config.output_directory_path());
  TestHost host(kFramebufferWidth, kFramebufferHeight, kTextureWidth, kTextureHeight);

  std::vector<std::shared_ptr<TestSuite>> test_suites;
  RegisterSuites(host, config, test_suites, config.output_directory_path());

#ifdef DUMP_CONFIG_FILE
  debugPrint("Dumping config file and exiting due to DUMP_CONFIG_FILE option.\n");
  DumpConfig(config, test_suites);
#else   // DUMP_CONFIG_FILE
  {
    std::vector<std::string> errors;
    if (!config.ApplyConfig(test_suites, errors)) {
      debugClearScreen();
      debugPrint("Failed to apply runtime config:\n");
      for (auto& err : errors) {
        debugPrint("%s\n", err.c_str());
      }
      Sleep(kDelayOnFailureMilliseconds);
      return 1;
    }
  }
  pb_show_front_screen();
  RunTests(config, host, test_suites);
#endif  // DUMP_CONFIG_FILE

  pb_kill();
  return 0;
}

static bool EnsureDriveMounted(char drive_letter) {
  if (nxIsDriveMounted(drive_letter)) {
    return true;
  }

  char dos_path[4] = "x:\\";
  dos_path[0] = drive_letter;
  char device_path[256] = {0};
  if (XConvertDOSFilenameToXBOX(dos_path, device_path) != STATUS_SUCCESS) {
    return false;
  }

  if (!strstr(device_path, R"(\Device\Harddisk0\Partition)")) {
    return false;
  }
  device_path[28] = 0;

  return nxMountDrive(drive_letter, device_path);
}

static bool LoadConfig(RuntimeConfig& config, std::vector<std::string>& errors) {
#ifdef RUNTIME_CONFIG_PATH
  if (!EnsureDriveMounted(RUNTIME_CONFIG_PATH[0])) {
    debugPrint("Ignoring missing config at %s\n", RUNTIME_CONFIG_PATH);
  } else {
    if (config.LoadConfig(RUNTIME_CONFIG_PATH, errors)) {
      return true;
    } else {
      debugPrint("Failed to load config at %s\n", RUNTIME_CONFIG_PATH);
    }
  }
#endif

  return config.LoadConfig("d:\\nxdk_pgraph_tests_config.json", errors);
}

#ifdef DUMP_CONFIG_FILE
static void DumpConfig(RuntimeConfig& config, std::vector<std::shared_ptr<TestSuite>>& test_suites) {
  std::string output_path = config.output_directory_path() + "\\sample-config.json";
  std::vector<std::string> errors;
  if (!config.DumpConfig(output_path, test_suites, errors)) {
    debugPrint("Failed to dump config file at %s\n", output_path.c_str());
    for (auto& error : errors) {
      debugPrint("%s\n", error.c_str());
    }
  } else {
    debugPrint("Config written to %s\n", output_path.c_str());
  }

  pb_show_debug_screen();
  Sleep(4000);
  Shutdown();
}
#endif

static void RunTests(RuntimeConfig& config, TestHost& host, std::vector<std::shared_ptr<TestSuite>>& test_suites) {
  std::map<std::string, std::set<std::string>> historical_crashes;

  if (config.enable_progress_log()) {
    std::string log_file = config.output_directory_path() + "\\" + kLogFileName;
#ifdef ENABLE_INTERACTIVE_CRASH_AVOIDANCE
    DiscoverHistoricalCrashes(log_file, historical_crashes);
#endif
    DeleteFile(log_file.c_str());

    Logger::Initialize(log_file, true);

    if (!historical_crashes.empty()) {
      Logger::Log() << "<HistoricalCrashes>" << std::endl;
      for (auto& suite_tests : historical_crashes) {
        for (auto& test : suite_tests.second) {
          Logger::Log() << "Crash? " << suite_tests.first << "::" << test << std::endl;
        }
      }
      Logger::Log() << "</HistoricalCrashes>" << std::endl;
    }
  }

  if (!historical_crashes.empty()) {
    for (auto& suite : test_suites) {
      auto crash_info = historical_crashes.find(suite->Name());
      if (crash_info != historical_crashes.end()) {
        suite->SetSuspectedCrashes(crash_info->second);
      }
    }
  }

  TestDriver driver(host, test_suites, kFramebufferWidth, kFramebufferHeight, !historical_crashes.empty(),
                    config.disable_autorun(), config.enable_autorun_immediately());
  driver.Run();

  if (config.enable_progress_log() && Logger::Log().is_open()) {
    Logger::Log().close();
  }

  if (config.enable_shutdown_on_completion()) {
    debugPrint("Results written to %s\n\nShutting down in 4 seconds...\n", config.output_directory_path().c_str());
    pb_show_debug_screen();
    Sleep(4000);

    Shutdown();
  } else {
    debugPrint("Results written to %s\n\nRebooting in 4 seconds...\n", config.output_directory_path().c_str());
    pb_show_debug_screen();
    Sleep(4000);
  }
}

#ifdef ENABLE_INTERACTIVE_CRASH_AVOIDANCE
static bool DiscoverHistoricalCrashes(const std::string& log_file_path,
                                      std::map<std::string, std::set<std::string>>& crashes) {
  crashes.clear();
  if (!EnsureDriveMounted(log_file_path[0])) {
    return false;
  }

  std::string dos_style_path = log_file_path;
  std::replace(dos_style_path.begin(), dos_style_path.end(), '/', '\\');

  std::ifstream log_file(dos_style_path.c_str());
  if (!log_file) {
    return false;
  }

  std::string last_test_suite;
  std::string last_test_name;
  std::string line;

  auto add_crash = [&crashes](const std::string& suite, const std::string& test) {
    auto toskip = crashes.find(suite);
    if (toskip == crashes.end()) {
      crashes[suite] = {test};
    } else {
      toskip->second.emplace(test);
    }
  };

  while (std::getline(log_file, line)) {
    PrintMsg("'%s'\n", line.c_str());
    if (!line.compare(0, 7, "Crash? ")) {
      line = line.substr(7);
      auto delimiter = line.find("::");
      add_crash(line.substr(0, delimiter), line.substr(delimiter + 2));
      continue;
    }

    if (!line.compare(0, 9, "Starting ")) {
      if (!last_test_suite.empty()) {
        PrintMsg("Potential crash: '%s' '%s'\n", last_test_suite.c_str(), last_test_name.c_str());
        add_crash(last_test_suite, last_test_name);
      }

      line = line.substr(9);
      auto delimiter = line.find("::");
      last_test_suite = line.substr(0, delimiter);
      last_test_name = line.substr(delimiter + 2);
      continue;
    }

    if (!line.compare(0, 13, "  Completed '")) {
      std::string test_name = line.substr(13);
      auto delimiter = test_name.rfind("' in ");
      test_name = test_name.substr(0, delimiter);
      if (test_name == last_test_name) {
        auto crash_suite = crashes.find(last_test_suite);
        if (crash_suite != crashes.end()) {
          crash_suite->second.erase(last_test_name);
        }
        last_test_suite.clear();
        last_test_name.clear();
        continue;
      }
      PrintMsg("Completed line mismatch: '%s' '%s' but completed: '%s'\n", last_test_suite.c_str(),
               last_test_name.c_str(), test_name.c_str());
    }

    PrintMsg("Unprocessed log line '%s'\n", line.c_str());
  }

  if (!last_test_suite.empty()) {
    PrintMsg("Potential crash: '%s' '%s'\n", last_test_suite.c_str(), last_test_name.c_str());
    add_crash(last_test_suite, last_test_name);
  }

  return true;
}
#endif  // ENABLE_INTERACTIVE_CRASH_AVOIDANCE

static void Shutdown() {
  // TODO: HalInitiateShutdown doesn't seem to cause Xemu to actually close.
  // This never sends the SMC command indicating that a shutdown should occur (at least, it never makes it to
  // `smc_write_data` to be processed).
  // HalInitiateShutdown();

  HalWriteSMBusValue(kSMCSlaveAddress, kSMCRegisterPower, FALSE, kSMCPowerShutdown);
  while (true) {
    Sleep(30000);
  }
}

static void RegisterSuites(TestHost& host, RuntimeConfig& runtime_config,
                           std::vector<std::shared_ptr<TestSuite>>& test_suites, const std::string& output_directory) {
  auto config = TestSuite::Config{runtime_config.enable_progress_log(), runtime_config.enable_pgraph_region_diff(),
                                  runtime_config.delay_milliseconds_between_tests()};

#define REG_TEST(CLASS_NAME)                                                   \
  {                                                                            \
    auto suite = std::make_shared<CLASS_NAME>(host, output_directory, config); \
    test_suites.push_back(suite);                                              \
  }

  // LightingNormalTests must be the first suite run for valid results. The first test in the suite depends on having a
  // clean initial state.
  REG_TEST(LightingNormalTests)

  // Remaining tests should be alphabetized.
  // -- Begin REG_TEST --
  REG_TEST(AntialiasingTests)
  REG_TEST(AttributeCarryoverTests)
  REG_TEST(AttributeExplicitSetterTests)
  REG_TEST(AttributeFloatTests)
  REG_TEST(BlendTests)
  REG_TEST(ClearTests)
  REG_TEST(ColorKeyTests)
  REG_TEST(ColorMaskBlendTests)
  REG_TEST(ColorZetaDisableTests)
  REG_TEST(ColorZetaOverlapTests)
  REG_TEST(CombinerTests)
  REG_TEST(DepthFormatTests)
  REG_TEST(DepthFormatFixedFunctionTests)
  REG_TEST(DMACorruptionAroundSurfaceTests)
  REG_TEST(EdgeFlagTests)
  REG_TEST(FogTests)
  REG_TEST(FogCustomShaderTests)
  REG_TEST(FogInfiniteFogCoordinateTests)
  REG_TEST(FogVec4CoordTests)
  REG_TEST(FrontFaceTests)
  REG_TEST(ImageBlitTests)
  REG_TEST(InlineArraySizeMismatchTests)
  REG_TEST(LightingTwoSidedTests)
  REG_TEST(LineWidthTests)
  REG_TEST(MaterialAlphaTests)
  REG_TEST(MaterialColorTests)
  REG_TEST(MaterialColorSourceTests)
  REG_TEST(NullSurfaceTests)
  REG_TEST(OverlappingDrawModesTests)
  REG_TEST(PvideoTests)
  REG_TEST(SetVertexDataTests)
  REG_TEST(ShadeModelTests)
  REG_TEST(SmoothingTests)
  REG_TEST(SurfaceClipTests)
  REG_TEST(SurfacePitchTests)
  REG_TEST(StencilTests)
  REG_TEST(StippleTests)
  REG_TEST(TextureBorderTests)
  REG_TEST(TextureCPUUpdateTests)
  REG_TEST(TextureCubemapTests)
  REG_TEST(TexgenMatrixTests)
  REG_TEST(TexgenTests)
  REG_TEST(TextureFormatDXTTests)
  REG_TEST(TextureFormatTests)
  REG_TEST(TextureFramebufferBlitTests)
  REG_TEST(TextureMatrixTests)
  REG_TEST(TextureRenderTargetTests)
  REG_TEST(TextureRenderUpdateInPlaceTests)
  REG_TEST(TextureShadowComparatorTests)
  REG_TEST(TextureSignedComponentTests)
  REG_TEST(ThreeDPrimitiveTests)
  REG_TEST(TwoDLineTests)
  REG_TEST(VertexShaderIndependenceTests)
  REG_TEST(VertexShaderRoundingTests)
  REG_TEST(VertexShaderSwizzleTests)
  REG_TEST(ViewportTests)
  REG_TEST(VolumeTextureTests)
  REG_TEST(WParamTests)
  REG_TEST(WindowClipTests)
  REG_TEST(ZMinMaxControlTests)
  REG_TEST(ZeroStrideTests)

  // -- End REG_TEST --

#undef REG_TEST
}
