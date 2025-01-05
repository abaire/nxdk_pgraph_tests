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
#include <lwip/inet.h>
#include <lwip/netif.h>
#include <nxdk/format.h>
#include <nxdk/net.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "configure.h"
#include "debug_output.h"
#include "logger.h"
#include "pushbuffer.h"
#include "runtime_config.h"
#include "test_driver.h"
#include "test_host.h"
#include "tests/alpha_func_tests.h"
#include "tests/antialiasing_tests.h"
#include "tests/attribute_carryover_tests.h"
#include "tests/attribute_explicit_setter_tests.h"
#include "tests/attribute_float_tests.h"
#include "tests/blend_surface_tests.h"
#include "tests/blend_tests.h"
#include "tests/bump_env_lum_tests.h"
#include "tests/bump_map_tests.h"
#include "tests/clear_tests.h"
#include "tests/color_key_tests.h"
#include "tests/color_mask_blend_tests.h"
#include "tests/color_zeta_disable_tests.h"
#include "tests/color_zeta_overlap_tests.h"
#include "tests/combiner_tests.h"
#include "tests/context_switch_tests.h"
#include "tests/degenerate_begin_end_tests.h"
#include "tests/depth_clamp_tests.h"
#include "tests/depth_format_fixed_function_tests.h"
#include "tests/depth_format_tests.h"
#include "tests/depth_function_tests.h"
#include "tests/dma_corruption_around_surface_tests.h"
#include "tests/edge_flag_tests.h"
#include "tests/fog_carryover_tests.h"
#include "tests/fog_exceptional_value_tests.h"
#include "tests/fog_gen_tests.h"
#include "tests/fog_param_tests.h"
#include "tests/fog_tests.h"
#include "tests/front_face_tests.h"
#include "tests/high_vertex_count_tests.h"
#include "tests/image_blit_tests.h"
#include "tests/inline_array_size_mismatch.h"
#include "tests/lighting_accumulation_tests.h"
#include "tests/lighting_control_tests.h"
#include "tests/lighting_normal_tests.h"
#include "tests/lighting_range_tests.h"
#include "tests/lighting_spotlight_tests.h"
#include "tests/lighting_two_sided_tests.h"
#include "tests/line_width_tests.h"
#include "tests/material_alpha_tests.h"
#include "tests/material_color_source_tests.h"
#include "tests/material_color_tests.h"
#include "tests/null_surface_tests.h"
#include "tests/overlapping_draw_modes_tests.h"
#include "tests/pixel_shader_tests.h"
#include "tests/point_params_tests.h"
#include "tests/point_size_tests.h"
#include "tests/point_sprite_tests.h"
#include "tests/pvideo_tests.h"
#include "tests/set_vertex_data_tests.h"
#include "tests/shade_model_tests.h"
#include "tests/smoothing_tests.h"
#include "tests/specular_back_tests.h"
#include "tests/specular_tests.h"
#include "tests/stencil_func_tests.h"
#include "tests/stencil_tests.h"
#include "tests/stipple_tests.h"
#include "tests/surface_clip_tests.h"
#include "tests/surface_format_tests.h"
#include "tests/surface_pitch_tests.h"
#include "tests/swath_width_tests.h"
#include "tests/texgen_matrix_tests.h"
#include "tests/texgen_tests.h"
#include "tests/texture_2d_as_cubemap_tests.h"
#include "tests/texture_3d_as_2d_tests.h"
#include "tests/texture_anisotropy_tests.h"
#include "tests/texture_border_color_tests.h"
#include "tests/texture_border_tests.h"
#include "tests/texture_brdf_tests.h"
#include "tests/texture_cpu_update_tests.h"
#include "tests/texture_cubemap_tests.h"
#include "tests/texture_format_dxt_tests.h"
#include "tests/texture_format_tests.h"
#include "tests/texture_framebuffer_blit_tests.h"
#include "tests/texture_lod_bias_tests.h"
#include "tests/texture_matrix_tests.h"
#include "tests/texture_palette_tests.h"
#include "tests/texture_perspective_enable_tests.h"
#include "tests/texture_perspective_tests.h"
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
#include "tests/weight_setter_tests.h"
#include "tests/window_clip_tests.h"
#include "tests/z_min_max_control_tests.h"
#include "tests/zero_stride_tests.h"
#include "tests/zpass_pixel_count_tests.h"

static constexpr int kDelayOnFailureMilliseconds = 4000;

static constexpr int kFramebufferWidth = 640;
static constexpr int kFramebufferHeight = 480;
static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 256;

#ifndef DUMP_CONFIG_FILE
static constexpr const char* kLogFileName = "pgraph_progress_log.txt";
#endif

const UCHAR kSMCSlaveAddress = 0x20;
const UCHAR kSMCRegisterPower = 0x02;
const UCHAR kSMCPowerShutdown = 0x80;

static bool EnsureDriveMounted(char drive_letter, bool format = false);
#ifdef DUMP_CONFIG_FILE
static void DumpConfig(RuntimeConfig& config, std::vector<std::shared_ptr<TestSuite>>& test_suites);
#else
static bool LoadConfig(RuntimeConfig& config, std::vector<std::string>& errors);
static void RunTests(RuntimeConfig& config, TestHost& host, std::vector<std::shared_ptr<TestSuite>>& test_suites);
#endif
static void RegisterSuites(TestHost& host, RuntimeConfig& config, std::vector<std::shared_ptr<TestSuite>>& test_suites,
                           const std::string& output_directory, std::shared_ptr<FTPLogger> ftp_logger);
static void Shutdown();

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
    pb_kill();
    return 1;
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    debugPrint("Failed to initialize SDL_image PNG mode.");
    pb_show_debug_screen();
    Sleep(kDelayOnFailureMilliseconds);
    pb_kill();
    return 1;
  }

  Pushbuffer::Initialize();

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
    pb_kill();
    return 1;
  };

  if (!EnsureDriveMounted('Z', true)) {
    debugPrint("Failed to mount cache dir.\n");
    pb_show_debug_screen();
    Sleep(kDelayOnFailureMilliseconds);
    pb_kill();
    return 1;
  }

  TestHost::EnsureFolderExists(config.output_directory_path());

  std::vector<std::shared_ptr<TestSuite>> test_suites;
  std::shared_ptr<FTPLogger> ftp_logger;
#ifndef DUMP_CONFIG_FILE
  auto network_config_mode = config.network_config_mode();
  if (network_config_mode != RuntimeConfig::NetworkConfigMode::OFF) {
    debugPrint("Initializing network...\n");
    pb_show_debug_screen();

    nx_net_mode_t network_mode = NX_NET_AUTO;
    if (network_config_mode == RuntimeConfig::NetworkConfigMode::DHCP) {
      network_mode = NX_NET_DHCP;
    } else if (network_config_mode == RuntimeConfig::NetworkConfigMode::STATIC) {
      network_mode = NX_NET_STATIC;
    }

    nx_net_parameters_t network_config{network_mode,
                                       network_mode,
                                       config.static_ip(),
                                       config.static_gateway(),
                                       config.static_netmask(),
                                       config.static_dns_1(),
                                       config.static_dns_2()};
    int net_init_result = nxNetInit(&network_config);
    if (net_init_result) {
      debugPrint("nxNetInit failed: %d\n", net_init_result);
      pb_show_debug_screen();
      Sleep(kDelayOnFailureMilliseconds);
      pb_kill();
      return 1;
    }

    debugPrint("Network initialized: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));
    pb_show_debug_screen();

    if (config.ftp_server_ip()) {
      ftp_logger = std::make_shared<FTPLogger>(config.ftp_server_ip(), config.ftp_server_port(), config.ftp_user(),
                                               config.ftp_password(), config.ftp_timeout_milliseconds());
    }
  }
#endif  // #ifndef DUMP_CONFIG_FILE

  TestHost host(ftp_logger, kFramebufferWidth, kFramebufferHeight, kTextureWidth, kTextureHeight);
  RegisterSuites(host, config, test_suites, config.output_directory_path(), ftp_logger);

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
      pb_kill();
      return 1;
    }
  }

  pb_show_front_screen();
  RunTests(config, host, test_suites);
#endif  // DUMP_CONFIG_FILE

  pb_kill();
  return 0;
}

static bool EnsureDriveMounted(char drive_letter, bool format) {
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

  if (format) {
    // Safety check - only allow formatting X,Y,Z
    char last_char = device_path[27];
    ASSERT((last_char == '3' || last_char == '4' || last_char == '5') && "Only X, Y, and Z drives may be formatted.");
    if (!nxFormatVolume(device_path, 0)) {
      return false;
    }
  }

  return nxMountDrive(drive_letter, device_path);
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
#else  // #ifdef DUMP_CONFIG_FILE

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

static void RunTests(RuntimeConfig& config, TestHost& host, std::vector<std::shared_ptr<TestSuite>>& test_suites) {
  if (config.enable_progress_log()) {
    std::string log_file = config.output_directory_path() + "\\" + kLogFileName;

    DeleteFile(log_file.c_str());

    Logger::Initialize(log_file, true);
  }

  TestDriver driver(host, test_suites, kFramebufferWidth, kFramebufferHeight, false, config.disable_autorun(),
                    config.enable_autorun_immediately());
  driver.Run();

  PrintMsg("Test loop completed normally\n");
  if (config.enable_progress_log() && Logger::Log().is_open()) {
    Logger::Log() << "Testing completed normally, closing log." << std::endl;
    Logger::Log().close();
  }

  auto exit_wait = config.delay_milliseconds_before_exit();
  auto exit_wait_seconds = exit_wait / 1000;
  auto exit_wait_remainder = exit_wait - (exit_wait_seconds * 1000);

  if (config.enable_shutdown_on_completion()) {
    if (exit_wait) {
      debugPrint("Results written to %s\n\nShutting down in %d.%03d seconds...\n",
                 config.output_directory_path().c_str(), exit_wait_seconds, exit_wait_remainder);
      pb_show_debug_screen();
      Sleep(exit_wait);
    }

    Shutdown();
  } else if (exit_wait) {
    debugPrint("Results written to %s\n\nRebooting in %d.%03d seconds...\n", config.output_directory_path().c_str(),
               exit_wait_seconds, exit_wait_remainder);
    pb_show_debug_screen();
    Sleep(exit_wait);
  }
}
#endif  // #ifdef DUMP_CONFIG_FILE

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
                           std::vector<std::shared_ptr<TestSuite>>& test_suites, const std::string& output_directory,
                           std::shared_ptr<FTPLogger> ftp_logger) {
  auto config = TestSuite::Config{runtime_config.enable_progress_log(), runtime_config.enable_pgraph_region_diff(),
                                  runtime_config.delay_milliseconds_between_tests(), std::move(ftp_logger)};

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

  REG_TEST(AlphaFuncTests)
  REG_TEST(AntialiasingTests)
  REG_TEST(AttributeCarryoverTests)
  REG_TEST(AttributeExplicitSetterTests)
  REG_TEST(AttributeFloatTests)
  REG_TEST(BlendSurfaceTests)
  REG_TEST(BlendTests)
  REG_TEST(BumpMapTests)
  REG_TEST(BumpEnvLumTests)
  REG_TEST(ClearTests)
  REG_TEST(ColorKeyTests)
  REG_TEST(ColorMaskBlendTests)
  REG_TEST(ColorZetaDisableTests)
  REG_TEST(ColorZetaOverlapTests)
  REG_TEST(CombinerTests)
  REG_TEST(ContextSwitchTests)
  REG_TEST(DegenerateBeginEndTests)
  REG_TEST(DepthClampTests)
  REG_TEST(DepthFormatFixedFunctionTests)
  REG_TEST(DepthFormatTests)
  REG_TEST(DepthFunctionTests)
  REG_TEST(DMACorruptionAroundSurfaceTests)
  REG_TEST(EdgeFlagTests)
  REG_TEST(FogCarryoverTests)
  REG_TEST(FogCustomShaderTests)
  REG_TEST(FogExceptionalValueTests)
  REG_TEST(FogGenTests)
  REG_TEST(FogInfiniteFogCoordinateTests)
  REG_TEST(FogParamTests)
  REG_TEST(FogTests)
  REG_TEST(FogVec4CoordTests)
  REG_TEST(FrontFaceTests)
  REG_TEST(HighVertexCountTests)
  REG_TEST(ImageBlitTests)
  REG_TEST(InlineArraySizeMismatchTests)
  REG_TEST(LightingAccumulationTests)
  REG_TEST(LightingControlTests)
  REG_TEST(LightingRangeTests)
  REG_TEST(LightingSpotlightTests)
  REG_TEST(LightingTwoSidedTests)
  REG_TEST(LineWidthTests)
  REG_TEST(MaterialAlphaTests)
  REG_TEST(MaterialColorSourceTests)
  REG_TEST(MaterialColorTests)
  REG_TEST(NullSurfaceTests)
  REG_TEST(OverlappingDrawModesTests)
  REG_TEST(PixelShaderTests)
  REG_TEST(PointParamsTests)
  REG_TEST(PointSizeTests)
  REG_TEST(PointSpriteTests)
  REG_TEST(PvideoTests)
  REG_TEST(SetVertexDataTests)
  REG_TEST(ShadeModelTests)
  REG_TEST(SmoothingTests)
  REG_TEST(SpecularBackTests)
  REG_TEST(SpecularTests)
  REG_TEST(StencilFuncTests)
  REG_TEST(StencilTests)
  REG_TEST(StippleTests)
  REG_TEST(SurfaceClipTests)
  REG_TEST(SurfaceFormatTests)
  REG_TEST(SurfacePitchTests)
  REG_TEST(SwathWidthTests)
  REG_TEST(TexgenMatrixTests)
  REG_TEST(TexgenTests)
  REG_TEST(Texture2DAsCubemapTests)
  REG_TEST(Texture3DAs2DTests)
  REG_TEST(TextureAnisotropyTests)
  REG_TEST(TextureBRDFTests)
  REG_TEST(TextureBorderColorTests)
  REG_TEST(TextureBorderTests)
  REG_TEST(TextureCPUUpdateTests)
  REG_TEST(TextureCubemapTests)
  REG_TEST(TextureFormatDXTTests)
  REG_TEST(TextureFormatTests)
  REG_TEST(TextureFramebufferBlitTests)
  REG_TEST(TextureLodBiasTests)
  REG_TEST(TextureMatrixTests)
  REG_TEST(TexturePaletteTests)
  REG_TEST(TexturePerspectiveEnableTests)
  REG_TEST(TexturePerspectiveTests)
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
  REG_TEST(WeightSetterTests)
  REG_TEST(WindowClipTests)
  REG_TEST(WParamTests)
  REG_TEST(ZeroStrideTests)
  REG_TEST(ZMinMaxControlTests)
  REG_TEST(ZPassPixelCountTests)
  // -- End REG_TEST --

#undef REG_TEST
}
