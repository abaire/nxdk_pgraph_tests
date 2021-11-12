/*
 * This sample provides a very basic demonstration of 3D rendering on the Xbox,
 * using pbkit. Based on the pbkit demo sources.
 */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

#include <SDL_image.h>
#include <hal/debug.h>
#include <hal/video.h>
#include <nxdk/mount.h>
#include <pbkit/pbkit.h>
#include <windows.h>

#include <cstdint>

#include "test_host.h"
#include "tests/depth_format_tests.h"
#include "tests/texture_format_tests.h"

#ifdef DEVKIT
static constexpr const char *kOutputDirectory = "e:\\DEVKIT\\nxdk_texture_tests";
#else
static constexpr const char *kOutputDirectory = "e:\\nxdk_texture_tests";
#endif

static constexpr int kFramebufferWidth = 640;
static constexpr int kFramebufferHeight = 480;
static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 256;

/* Main program function */
int main() {
  XVideoSetMode(kFramebufferWidth, kFramebufferHeight, 32, REFRESH_DEFAULT);

  if (!nxMountDrive('E', R"(\Device\Harddisk0\Partition1)")) {
    debugPrint("Failed to mount E:");
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

  int status = pb_init();
  if (status) {
    debugPrint("pb_init Error %d\n", status);
    pb_show_debug_screen();
    Sleep(2000);
    return 1;
  }

  pb_show_front_screen();

  TestHost host(kFramebufferWidth, kFramebufferHeight, kTextureWidth, kTextureHeight);

  {
    TextureFormatTests tests(host, kOutputDirectory, kFramebufferWidth, kFramebufferHeight);
    tests.Run();
  }

  {
    DepthFormatTests tests(host, kOutputDirectory);
    tests.Run();
  }

  debugPrint("Results written to %s", kOutputDirectory);
  pb_show_debug_screen();
  Sleep(15000);

  pb_kill();
  return 0;
}

#pragma clang diagnostic pop
