/*
 * This sample provides a very basic demonstration of 3D rendering on the Xbox,
 * using pbkit. Based on the pbkit demo sources.
 */

#include <SDL_image.h>
#include <hal/debug.h>
#include <hal/video.h>
#include <nxdk/mount.h>
#include <pbkit/pbkit.h>
#include <windows.h>

#include <memory>
#include <vector>

#include "test_driver.h"
#include "test_host.h"
#include "tests/depth_format_tests.h"
#include "tests/front_face_tests.h"
#include "tests/image_blit_tests.h"
#include "tests/lighting_normal_tests.h"
#include "tests/material_alpha_tests.h"
#include "tests/material_color_source_tests.h"
#include "tests/material_color_tests.h"
#include "tests/set_vertex_data_tests.h"
#include "tests/texture_format_tests.h"
#include "tests/three_d_primitive_tests.h"
#include "tests/two_d_line_tests.h"

#define FALLBACK_XBE_DIRECTORY "f:\\";
static constexpr int kFramebufferWidth = 640;
static constexpr int kFramebufferHeight = 480;
static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 256;

static void register_suites(TestHost& host, std::vector<std::shared_ptr<TestSuite>>& test_suites,
                            const std::string& output_directory);
static bool get_xbe_directory(std::string& xbe_root_directory);
static bool get_test_output_path(std::string& test_output_directory);

/* Main program function */
int main() {
  XVideoSetMode(kFramebufferWidth, kFramebufferHeight, 32, REFRESH_DEFAULT);

  int status = pb_init();
  if (status) {
    debugPrint("pb_init Error %d\n", status);
    pb_show_debug_screen();
    Sleep(2000);
    return 1;
  }

  if (SDL_Init(SDL_INIT_GAMECONTROLLER)) {
    debugPrint("Failed to initialize SDL_GAMECONTROLLER.");
    debugPrint("%s", SDL_GetError());
    pb_show_debug_screen();
    Sleep(2000);
    return 1;
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    debugPrint("Failed to initialize SDL_image PNG mode.");
    pb_show_debug_screen();
    Sleep(2000);
    return 1;
  }

  std::string test_output_directory;
  if (!get_test_output_path(test_output_directory)) {
    debugPrint("Failed to mount F:");
    pb_show_debug_screen();
    Sleep(2000);
    return 1;
  };

  pb_show_front_screen();

  TestHost host(kFramebufferWidth, kFramebufferHeight, kTextureWidth, kTextureHeight);

  std::vector<std::shared_ptr<TestSuite>> test_suites;
  register_suites(host, test_suites, test_output_directory);

  TestDriver driver(test_suites, kFramebufferWidth, kFramebufferHeight);
  driver.Run();

  debugPrint("Results written to %s\n\nRebooting in 4 seconds...\n", test_output_directory.c_str());
  pb_show_debug_screen();
  Sleep(4000);

  pb_kill();
  return 0;
}

bool get_xbe_directory(std::string& xbe_root_directory) {
  std::string xbe_directory = XeImageFileName->Buffer;
  if (xbe_directory.find("\\Device\\CdRom") == 0) {
    debugPrint("Running from readonly media, using default path for test output.\n");
    xbe_root_directory = FALLBACK_XBE_DIRECTORY;

    if (!nxMountDrive('F', R"(\Device\Harddisk0\Partition6)")) {
      return false;
    }
    return true;
  }

  xbe_root_directory = "D:";
  return true;
}

static bool get_test_output_path(std::string& test_output_directory) {
  if (!get_xbe_directory(test_output_directory)) {
    return false;
  }
  if (test_output_directory.back() == '\\') {
    test_output_directory.pop_back();
  }
  test_output_directory += "\\nxdk_pgraph_tests";
  return true;
}

static void register_suites(TestHost& host, std::vector<std::shared_ptr<TestSuite>>& test_suites,
                            const std::string& output_directory) {
  // Must be the first suite run for valid results. The first test depends on having a cleared initial state.
  {
    auto suite = std::make_shared<LightingNormalTests>(host, output_directory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }

  {
    auto suite = std::make_shared<FrontFaceTests>(host, output_directory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
  {
    auto suite = std::make_shared<DepthFormatTests>(host, output_directory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
  {
    auto suite = std::make_shared<ImageBlitTests>(host, output_directory);
    test_suites.push_back(std::dynamic_pointer_cast<ImageBlitTests>(suite));
  }
  {
    auto suite = std::make_shared<MaterialAlphaTests>(host, output_directory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
  {
    auto suite = std::make_shared<MaterialColorTests>(host, output_directory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
  {
    auto suite = std::make_shared<MaterialColorSourceTests>(host, output_directory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
  {
    auto suite = std::make_shared<SetVertexDataTests>(host, output_directory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
  {
    auto suite = std::make_shared<TextureFormatTests>(host, output_directory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
  {
    auto suite = std::make_shared<ThreeDPrimitiveTests>(host, output_directory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
  {
    auto suite = std::make_shared<TwoDLineTests>(host, output_directory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
}
