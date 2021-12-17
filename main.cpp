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

#include <cstdint>
#include <memory>
#include <vector>

#include "test_driver.h"
#include "test_host.h"
#include "tests/depth_format_tests.h"
#include "tests/front_face_tests.h"
#include "tests/image_blit_tests.h"
#include "tests/material_tests.h"
#include "tests/texture_format_tests.h"

#ifdef DEVKIT
static constexpr const char *kOutputDirectory = "e:\\DEVKIT\\nxdk_pgraph_tests";
#else
static constexpr const char *kOutputDirectory = "e:\\nxdk_pgraph_tests";
#endif

static constexpr int kFramebufferWidth = 640;
static constexpr int kFramebufferHeight = 480;
static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 256;

static void register_suites(TestHost &host, std::vector<std::shared_ptr<TestSuite>> &test_suites);

/* Main program function */
int main() {
  XVideoSetMode(kFramebufferWidth, kFramebufferHeight, 32, REFRESH_DEFAULT);

  if (!nxMountDrive('E', R"(\Device\Harddisk0\Partition1)")) {
    debugPrint("Failed to mount E:");
    pb_show_debug_screen();
    Sleep(2000);
    return 1;
  }

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

  pb_show_front_screen();

  TestHost host(kFramebufferWidth, kFramebufferHeight, kTextureWidth, kTextureHeight);

  std::vector<std::shared_ptr<TestSuite>> test_suites;
  register_suites(host, test_suites);

  TestDriver driver(test_suites, kFramebufferWidth, kFramebufferHeight);
  driver.Run();

  debugPrint("Results written to %s\n\nRebooting in 4 seconds...", kOutputDirectory);
  pb_show_debug_screen();
  Sleep(4000);

  pb_kill();
  return 0;
}

static void register_suites(TestHost &host, std::vector<std::shared_ptr<TestSuite>> &test_suites) {
  {
    auto suite = std::make_shared<MaterialTests>(host, kOutputDirectory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
  {
    auto suite = std::make_shared<ImageBlitTests>(host, kOutputDirectory);
    test_suites.push_back(std::dynamic_pointer_cast<ImageBlitTests>(suite));
  }
  {
    auto suite = std::make_shared<TextureFormatTests>(host, kOutputDirectory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
  {
    auto suite = std::make_shared<FrontFaceTests>(host, kOutputDirectory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
  {
    auto suite = std::make_shared<DepthFormatTests>(host, kOutputDirectory);
    test_suites.push_back(std::dynamic_pointer_cast<TestSuite>(suite));
  }
}
