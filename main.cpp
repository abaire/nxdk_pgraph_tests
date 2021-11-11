/*
 * This sample provides a very basic demonstration of 3D rendering on the Xbox,
 * using pbkit. Based on the pbkit demo sources.
 */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

#include <SDL.h>
#include <SDL_image.h>
#include <hal/debug.h>
#include <hal/video.h>
#include <nxdk/mount.h>
#include <pbkit/pbkit.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>

#include <cstdint>

#include "test_host.h"
#include "texture_format_tests.h"

#ifdef DEVKIT
static constexpr const char *kOutputDirectory = "e:\\DEVKIT\\nxdk_texture_tests";
#else
static constexpr const char *kOutputDirectory = "e:\\nxdk_texture_tests";
#endif

static Vertex *alloc_vertices;           // texcoords 0 to kFramebufferWidth/kFramebufferHeight
static Vertex *alloc_vertices_swizzled;  // texcoords normalized 0 to 1
static uint8_t *texture_memory;

static constexpr int kFramebufferWidth = 640;
static constexpr int kFramebufferHeight = 480;
static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 256;

static constexpr uint32_t kDepthFormats[] = {
    NV097_SET_SURFACE_FORMAT_ZETA_Z24S8,
    NV097_SET_SURFACE_FORMAT_ZETA_Z16,
};
static constexpr uint32_t kNumDepthFormats = sizeof(kDepthFormats) / sizeof(kDepthFormats[0]);

/* Main program function */
int main() {
  uint32_t *p;

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
    TextureFormatTests tests(host, kOutputDirectory);
    tests.Run();
  }

  //
  //  for (auto depth_format : kDepthFormats) {
  //    for (auto texture_format_idx : kTextureFormats) {
  //      char prefix[64] = {0};
  //      snprintf(prefix, 63, "TexFmt_Depth%d_", depth_format);
  //      test_texture_format(prefix, depth_format, texture_format_idx, texture_memory);
  //    }
  //  }
  //
  //  perform_depth_buffer_tests(texture_memory);

  debugPrint("Results written to %s", kOutputDirectory);
  pb_show_debug_screen();
  Sleep(2000);

  MmFreeContiguousMemory(alloc_vertices);
  MmFreeContiguousMemory(alloc_vertices_swizzled);
  MmFreeContiguousMemory(texture_memory);
  pb_show_debug_screen();
  pb_kill();
  return 0;
}

// static void perform_depth_buffer_tests(uint8_t *texture_memory) {
//  // NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8
//  const TextureFormatInfo &depth_test_texture_format = kTextureFormats[3];
//  init_depth_test_texture_memory(texture_memory, depth_test_texture_format);
//  for (auto depth_format : kDepthFormats) {
//    char prefix[64] = {0};
//    snprintf(prefix, 63, "DepthFmt_Depth%d_", depth_format);
//
//    test_depth_buffer(prefix, depth_format, depth_test_texture_format, texture_memory);
//  }
//
//}
//
// static int init_depth_test_texture_memory(uint8_t *texture_memory, const TextureFormatInfo &texture_format) {
//
//  SDL_Surface *gradient_surface = SDL_CreateRGBSurfaceWithFormat(0, kTextureWidth, kTextureHeight, 32,
//  SDL_PIXELFORMAT_RGBA8888); if (!gradient_surface) {
//    return 1;
//  }
//
//  if (SDL_LockSurface(gradient_surface)) {
//    return 2;
//  }
//
//  auto pixels = static_cast<uint32_t *>(gradient_surface->pixels);
//  for (int y = 0; y < kTextureHeight; ++y)
//    for (int x = 0; x < kTextureWidth; ++x, ++pixels) {
//      int normalized_x = static_cast<int>(static_cast<float>(x) * 255.0f / static_cast<float>(kTextureWidth));
//      int normalized_y = static_cast<int>(static_cast<float>(y) * 255.0f / static_cast<float>(kTextureHeight));
//      *pixels = SDL_MapRGB(gradient_surface->format, normalized_y, normalized_x, 0);
//    }
//
//  SDL_UnlockSurface(gradient_surface);
//
//  int ret = assign_texture_memory(texture_memory, texture_format, gradient_surface);
//  SDL_FreeSurface(gradient_surface);
//
//  return ret;
//}
//

//
// static void test_depth_buffer(const char *prefix, uint32_t depth_buffer_format, const TextureFormatInfo
// &texture_format, uint8_t *texture_memory) {
//  pb_wait_for_vbl();
//  pb_reset();
//  pb_target_back_buffer();
//
//  set_depth_buffer_format(depth_buffer_format);
//  clear_depth_buffer();
//
//  while (pb_busy()) {
//    /* Wait for completion... */
//  }
//
//  setup_texture_stages(texture_memory, texture_format);
//  send_shader_constants();
//
//  /*
//   * Setup vertex attributes
//   */
//  Vertex *vptr = texture_format.XboxSwizzled ? alloc_vertices_swizzled : alloc_vertices;
//
//  /* Set vertex position attribute */
//  set_attrib_pointer(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, sizeof(Vertex),
//                     &vptr[0].pos);
//
//  /* Set texture coordinate attribute */
//  set_attrib_pointer(NV2A_VERTEX_ATTR_TEXTURE0, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 2, sizeof(Vertex),
//                     &vptr[0].texcoord);
//
//  /* Set vertex normal attribute */
//  set_attrib_pointer(NV2A_VERTEX_ATTR_NORMAL, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, sizeof(Vertex),
//                     &vptr[0].normal);
//
//  /* Begin drawing triangles */
//  draw_arrays(NV097_SET_BEGIN_END_OP_TRIANGLES, 0, kNumVertices);
//
//
//  while (pb_busy()) {
//    /* Wait for completion... */
//  }
//
//  save_back_buffer(prefix, "depthtest");
//
//  /* Swap buffers (if we can) */
//  while (pb_finished()) {
//    /* Not ready to swap yet */
//  }
//}

#pragma clang diagnostic pop