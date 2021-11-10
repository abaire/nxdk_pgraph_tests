/*
 * This sample provides a very basic demonstration of 3D rendering on the Xbox,
 * using pbkit. Based on the pbkit demo sources.
 */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

// clang format off
#define _USE_MATH_DEFINES
#include <cmath>
// clang format on

#include <SDL.h>
#include <SDL_image.h>
#include <hal/debug.h>
#include <hal/video.h>
#include <nxdk/mount.h>
#include <pbkit/pbkit.h>
#include <strings.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>

#include <cstdint>
#include <cstring>

#include "math3d.h"
#include "nxdk_missing_defines.h"
#include "third_party/swizzle.h"

#ifdef DEVKIT
static constexpr const char *kOutputDirectory = "e:\\DEVKIT\\nxdk_texture_tests";
#else
static constexpr const char *kOutputDirectory = "e:\\nxdk_texture_tests";
#endif

typedef struct TextureFormatInfo {
  SDL_PixelFormatEnum SdlFormat;
  uint32_t XboxFormat;
  uint16_t XboxBpp;  // bytes per pixel
  bool XboxSwizzled;
  bool RequireConversion;
  const char *Name;
} TextureFormatInfo;

#pragma pack(1)
typedef struct Vertex {
  float pos[3];
  float texcoord[2];
  float normal[3];
} Vertex;

#pragma pack()

static Vertex *alloc_vertices;           // texcoords 0 to kFramebufferWidth/kFramebufferHeight
static Vertex *alloc_vertices_swizzled;  // texcoords normalized 0 to 1

static constexpr int kFramebufferWidth = 640;
static constexpr int kFramebufferHeight = 480;
static constexpr int kTextureWidth = 256;
static constexpr int kTextureHeight = 256;

MATRIX m_model, m_view, m_proj;

VECTOR v_cam_pos = {0, 0.05, 1.07, 1};
VECTOR v_cam_rot = {0, 0, 0, 1};
VECTOR v_light_dir = {0, 0, 1, 1};

#include "resources/texture.h"
#include "resources/verts.h"

#define MASK(mask, val) (((val) << (ffs(mask) - 1)) & (mask))
#define MAXRAM 0x03FFAFFF

static void init_vertices();
static void init_matrices();
static void matrix_viewport(float out[4][4], float x, float y, float width, float height, float z_min, float z_max);
static void init_shader();
static void set_attrib_pointer(unsigned int index, unsigned int format, unsigned int size, unsigned int stride,
                               const void *data);
static void draw_arrays(unsigned int mode, int start, int count);
static int update_texture_memory(uint8_t *texture_memory, TextureFormatInfo format, int width, int height);
static void save_framebuffer(const uint8_t *framebuffer, const TextureFormatInfo &format_info, const char *prefix);
static void setup_texture_stages(const uint8_t *texture_memory, const TextureFormatInfo &format_info);
static void send_shader_constants();

static constexpr TextureFormatInfo format_map[] = {

    // swizzled
    {SDL_PIXELFORMAT_ABGR8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8, 4, true, false, "A8B8G8R8"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8G8B8A8, 4, true, false, "R8G8B8A8"},
    {SDL_PIXELFORMAT_ARGB8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8, 4, true, false, "A8R8G8B8"},
    {SDL_PIXELFORMAT_ARGB8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8, 4, true, false, "X8R8G8B8"},
    {SDL_PIXELFORMAT_BGRA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_B8G8R8A8, 4, true, false, "B8G8R8A8"},
    {SDL_PIXELFORMAT_RGB565, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R5G6B5, 2, true, false, "R5G6B5"},
    {SDL_PIXELFORMAT_ARGB1555, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A1R5G5B5, 2, true, false, "A1R5G5B5"},
    {SDL_PIXELFORMAT_ARGB1555, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X1R5G5B5, 2, true, false, "X1R5G5B5"},
    {SDL_PIXELFORMAT_ARGB4444, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A4R4G4B4, 2, true, false, "A4R4G4B4"},

    // linear unsigned
    {SDL_PIXELFORMAT_ABGR8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8B8G8R8, 4, false, false, "A8B8G8R8"},
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R8G8B8A8, 4, false, false, "R8G8B8A8"},
    {SDL_PIXELFORMAT_ARGB8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8, 4, false, false, "A8R8G8B8"},
    {SDL_PIXELFORMAT_ARGB8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8, 4, false, false, "X8R8G8B8"},
    {SDL_PIXELFORMAT_BGRA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_B8G8R8A8, 4, false, false, "B8G8R8A8"},
    {SDL_PIXELFORMAT_RGB565, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_R5G6B5, 2, false, false, "R5G6B5"},
    {SDL_PIXELFORMAT_ARGB1555, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A1R5G5B5, 2, false, false, "A1R5G5B5"},
    {SDL_PIXELFORMAT_ARGB1555, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X1R5G5B5, 2, false, false, "X1R5G5B5"},
    {SDL_PIXELFORMAT_ARGB4444, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A4R4G4B4, 2, false, false, "A4R4G4B4"},

    // yuv color space
    // Each 4 bytes represent the color for 2 neighboring pixels:
    // [ U0 | Y0 | V0 | Y1 ]
    // Y0 is the brightness of pixel 0, Y1 the brightness of pixel 1.
    // U0 and V0 is the color of both pixels. (second pixel is the one sampled?
    // or averaged? doesn't really matter here)
    // https://docs.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering#converting-8-bit-yuv-to-rgb888
    {SDL_PIXELFORMAT_BGRA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_CR8YB8CB8YA8, 2, false, true, "YUY2"},
    {SDL_PIXELFORMAT_BGRA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_YB8CR8YA8CB8, 2, false, true, "UYVY"},

    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_Y16, false, true, "Y16" },
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_Y8, true, true, "SZ_Y8" },
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_Y8, false, true, "Y8" },
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_AY8, true, true, "SZ_AY8" },
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_AY8, false, true, "AY8" },
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8, true, true, "SZ_A8" },
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8Y8, true, true, "SZ_A8Y8" },

    // misc formats
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT1_A1R5G5B5, false, true, "DXT1" },
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT23_A8R8G8B8, false, true, "DXT3" },
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT45_A8R8G8B8, false, true, "DXT5" },
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_G8B8, true, true, "SZ_G8B8" },
    {SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_G8B8, 2, false, true, "G8B8"},
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_D16, false, true, "D16" },    // TODO: implement in
    // xemu
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_LIN_F16, false, true, "LIN_F16" },    // TODO:
    // implement in xemu
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R8B8, true, true, "SZ_R8B8" },
    //{ SDL_PIXELFORMAT_RGBA8888, NV097_SET_TEXTURE_FORMAT_COLOR_SZ_R6G5B5, true, true, "R6G5B5" }

    // TODO: define others here
};
static constexpr int kNumFormats = sizeof(format_map) / sizeof(format_map[0]);

// bitscan forward
int bsf(int val) { __asm bsf eax, val }

/* Main program function */
int main() {
  uint32_t *p;

  XVideoSetMode(kFramebufferWidth, kFramebufferHeight, 32, REFRESH_DEFAULT);

  if (!nxMountDrive('E', R"(\Device\Harddisk0\Partition1)")) {
    debugPrint("Failed to mount E:");
    Sleep(2000);
    return 1;
  }

  // initialize input for the first gamepad
  SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
  SDL_GameController *gameController = SDL_GameControllerOpen(0);
  if (!gameController) {
    debugPrint("Failed to initialize input for gamepad 0.");
    Sleep(2000);
    return 1;
  }

  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    debugPrint("Failed to initialize SDL_image PNG mode.");
    Sleep(2000);
    return 1;
  }

  int status = pb_init();
  if (status) {
    debugPrint("pb_init Error %d\n", status);
    Sleep(2000);
    return 1;
  }

  pb_show_front_screen();

  /* Load constant rendering things (shaders, geometry) */
  init_shader();

  init_vertices();
  init_matrices();

  // allocate texture memory buffer large enough for all types
  auto texture_memory = static_cast<uint8_t *>(MmAllocateContiguousMemoryEx(
      kTextureWidth * kTextureHeight * 4, 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE));
  int format_map_index = 0;
  int update_texture_result =
      update_texture_memory(texture_memory, format_map[format_map_index], kTextureWidth, kTextureHeight);

  uint8_t *framebuffer = XVideoGetFB();
  bool toggle_format_allowed = true;
  bool render_changed = true;
  bool automatic_forward_mode = false;
  bool automatic_backward_mode = false;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
  while (true) {
    SDL_GameControllerUpdate();
    bool a_pressed = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_A);
    bool b_pressed = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_B);
    bool start_pressed = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_START);
    bool back_pressed = SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_BACK);
    if (a_pressed || b_pressed || start_pressed || back_pressed) {
      if (toggle_format_allowed && !automatic_forward_mode && !automatic_backward_mode) {
        if (start_pressed) {
          automatic_forward_mode = !automatic_forward_mode;
          if (automatic_forward_mode) {
            format_map_index = 0;
            update_texture_result =
                update_texture_memory(texture_memory, format_map[format_map_index], kTextureWidth, kTextureHeight);
            render_changed = true;
          }
        } else if (back_pressed) {
          automatic_backward_mode = !automatic_backward_mode;
          if (automatic_backward_mode) {
            format_map_index = kNumFormats - 1;
            update_texture_result =
                update_texture_memory(texture_memory, format_map[format_map_index], kTextureWidth, kTextureHeight);
            render_changed = true;
          }
        } else {
          if (a_pressed) {
            format_map_index = (format_map_index + 1) % kNumFormats;
          } else {
            if (--format_map_index < 0) {
              format_map_index = kNumFormats - 1;
            }
          }
          update_texture_result =
              update_texture_memory(texture_memory, format_map[format_map_index], kTextureWidth, kTextureHeight);
          render_changed = true;
        }
      }
      toggle_format_allowed = false;
    } else {
      toggle_format_allowed = true;
    }

    pb_wait_for_vbl();
    pb_reset();
    pb_target_back_buffer();

    /* Clear depth & stencil buffers */
    pb_erase_depth_stencil_buffer(0, 0, kFramebufferWidth, kFramebufferHeight);
    pb_fill(0, 0, kFramebufferWidth, kFramebufferHeight, 0xff000000);
    pb_erase_text_screen();

    while (pb_busy()) {
      /* Wait for completion... */
    }

    setup_texture_stages(texture_memory, format_map[format_map_index]);
    send_shader_constants();

    /*
     * Setup vertex attributes
     */

    Vertex *vptr = format_map[format_map_index].XboxSwizzled ? alloc_vertices_swizzled : alloc_vertices;

    /* Set vertex position attribute */
    set_attrib_pointer(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, sizeof(Vertex),
                       &vptr[0].pos);

    /* Set texture coordinate attribute */
    set_attrib_pointer(NV2A_VERTEX_ATTR_TEXTURE0, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 2, sizeof(Vertex),
                       &vptr[0].texcoord);

    /* Set vertex normal attribute */
    set_attrib_pointer(NV2A_VERTEX_ATTR_NORMAL, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, sizeof(Vertex),
                       &vptr[0].normal);

    /* Begin drawing triangles */
    draw_arrays(NV097_SET_BEGIN_END_OP_TRIANGLES, 0, kNumVertices);

    /* Draw some text on the screen */
    pb_print("N: %s\n", format_map[format_map_index].Name);
    pb_print("F: 0x%x\n", format_map[format_map_index].XboxFormat);
    pb_print("SZ: %d\n", format_map[format_map_index].XboxSwizzled);
    pb_print("C: %d\n", format_map[format_map_index].RequireConversion);
    pb_print("W: %d\n", kTextureWidth);
    pb_print("H: %d\n", kTextureHeight);
    pb_print("P: %d\n", format_map[format_map_index].XboxBpp * kTextureWidth);
    pb_print("ERR: %d\n", update_texture_result);
    pb_draw_text_screen();

    while (pb_busy()) {
      /* Wait for completion... */
    }

    /* Swap buffers (if we can) */
    while (pb_finished()) {
      /* Not ready to swap yet */
    }

    if (render_changed) {
      render_changed = false;
      const char *prefix = "";
      if (automatic_forward_mode) {
        prefix = "auto_fwd_";
      } else if (automatic_backward_mode) {
        prefix = "auto_rev_";
      }
      save_framebuffer(framebuffer, format_map[format_map_index], prefix);
    }

    if (automatic_forward_mode) {
      if (++format_map_index == kNumFormats) {
        format_map_index = kNumFormats - 1;
        automatic_forward_mode = false;
      } else {
        update_texture_result =
            update_texture_memory(texture_memory, format_map[format_map_index], kTextureWidth, kTextureHeight);
        render_changed = true;
      }
    } else if (automatic_backward_mode) {
      if (--format_map_index < 0) {
        format_map_index = 0;
        automatic_backward_mode = false;
      } else {
        update_texture_result =
            update_texture_memory(texture_memory, format_map[format_map_index], kTextureWidth, kTextureHeight);
        render_changed = true;
      }
    }
  }
#pragma clang diagnostic pop

  /* Unreachable cleanup code */
  SDL_GameControllerClose(gameController);
  SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
  MmFreeContiguousMemory(alloc_vertices);
  MmFreeContiguousMemory(alloc_vertices_swizzled);
  MmFreeContiguousMemory(texture_memory);
  pb_show_debug_screen();
  pb_kill();
  return 0;
}

static void save_framebuffer(const uint8_t *framebuffer, const TextureFormatInfo &format_info, const char *prefix) {
  char target_file[256] = {0};
  snprintf(target_file, 255, "%s\\%s%s.bmp", kOutputDirectory, prefix, format_info.Name);
  CreateDirectoryA(kOutputDirectory, nullptr);

  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom((void *)framebuffer, 0, kFramebufferWidth,
                                                            kFramebufferHeight, 32, SDL_PIXELFORMAT_RGBA8888);
  int ret = SDL_SaveBMP(surface, target_file);
  SDL_FreeSurface(surface);
}

static void setup_texture_stages(const uint8_t *texture_memory, const TextureFormatInfo &format_info) {
  /* Enable texture stage 0 */
  /* FIXME: Use constants instead of the hardcoded values below */
  auto p = pb_begin();

  // first one seems to be needed
  p = pb_push1(p, NV097_SET_FRONT_FACE, NV097_SET_FRONT_FACE_V_CCW);
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);

  // Enable alpha blending functionality
  p = pb_push1(p, NV097_SET_BLEND_ENABLE, true);

  // Set the alpha blend source (s) and destination (d) factors
  p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
  p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_SRC_ALPHA);

  // yuv requires color space conversion
  if (format_info.XboxFormat == NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_CR8YB8CB8YA8 ||
      format_info.XboxFormat == NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_YB8CR8YA8CB8) {
    p = pb_push1(p, NV097_SET_CONTROL0,
                 MASK(NV097_SET_CONTROL0_COLOR_SPACE_CONVERT, NV097_SET_CONTROL0_COLOR_SPACE_CONVERT_CRYCB_TO_RGB));
  }

  DWORD format_mask = MASK(NV097_SET_TEXTURE_FORMAT_CONTEXT_DMA, 1) | MASK(NV097_SET_TEXTURE_FORMAT_CUBEMAP_ENABLE, 0) |
                      MASK(NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE, NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE_COLOR) |
                      MASK(NV097_SET_TEXTURE_FORMAT_DIMENSIONALITY, 2) |
                      MASK(NV097_SET_TEXTURE_FORMAT_COLOR, format_info.XboxFormat) |
                      MASK(NV097_SET_TEXTURE_FORMAT_MIPMAP_LEVELS, 1) |
                      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_U, format_info.XboxSwizzled ? bsf(kTextureWidth) : 0) |
                      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_V, format_info.XboxSwizzled ? bsf(kTextureHeight) : 0) |
                      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_P, 0);

  // set stage 0 texture address & format
  p = pb_push2(p, NV20_TCL_PRIMITIVE_3D_TX_OFFSET(0), (DWORD)texture_memory & 0x03ffffff, format_mask);

  if (!format_info.XboxSwizzled) {
    // set stage 0 texture pitch (pitch<<16)
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(0), (format_info.XboxBpp * kTextureWidth) << 16);

    // set stage 0 texture kFramebufferWidth & kFramebufferHeight
    // ((width<<16)|kFramebufferHeight)
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(0), (kTextureWidth << 16) | kTextureHeight);
  }

  // set stage 0 texture modes
  // (0x0W0V0U wrapping: 1=wrap 2=mirror 3=clamp 4=border 5=clamp to edge)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(0), 0x00030303);

  // set stage 0 texture enable flags
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(0), 0x4003ffc0);

  // set stage 0 texture filters (AA!)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(0), 0x04074000);

  pb_end(p);

  /* Disable other texture stages */
  p = pb_begin();

  // set stage 1 texture enable flags (bit30 disabled)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(1), 0x0003ffc0);

  // set stage 2 texture enable flags (bit30 disabled)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(2), 0x0003ffc0);

  // set stage 3 texture enable flags (bit30 disabled)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(3), 0x0003ffc0);

  // set stage 1 texture modes (0x0W0V0U wrapping: 1=wrap 2=mirror 3=clamp
  // 4=border 5=clamp to edge)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(1), 0x00030303);

  // set stage 2 texture modes (0x0W0V0U wrapping: 1=wrap 2=mirror 3=clamp
  // 4=border 5=clamp to edge)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(2), 0x00030303);

  // set stage 3 texture modes (0x0W0V0U wrapping: 1=wrap 2=mirror 3=clamp
  // 4=border 5=clamp to edge)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(3), 0x00030303);

  // set stage 1 texture filters (no AA, stage not even used)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(1), 0x02022000);

  // set stage 2 texture filters (no AA, stage not even used)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(2), 0x02022000);

  // set stage 3 texture filters (no AA, stage not even used)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(3), 0x02022000);

  pb_end(p);
}

static void send_shader_constants() {
  /* Send shader constants
   *
   * WARNING: Changing shader source code may impact constant locations!
   * Check the intermediate file (*.inl) for the expected locations after
   * changing the code.
   */
  auto p = pb_begin();

  /* Set shader constants cursor at C0 */
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_ID, 96);

  /* Send the model matrix */
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 16);
  memcpy(p, m_model, 16 * 4);
  p += 16;

  /* Send the view matrix */
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 16);
  memcpy(p, m_view, 16 * 4);
  p += 16;

  /* Send the projection matrix */
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 16);
  memcpy(p, m_proj, 16 * 4);
  p += 16;

  /* Send camera position */
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 4);
  memcpy(p, v_cam_pos, 4 * 4);
  p += 4;

  /* Send light direction */
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 4);
  memcpy(p, v_light_dir, 4 * 4);
  p += 4;

  /* Send shader constants */
  float constants_0[4] = {0, 0, 0, 0};
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 4);
  memcpy(p, constants_0, 4 * 4);
  p += 4;

  /* Clear all attributes */
  pb_push(p++, NV097_SET_VERTEX_DATA_ARRAY_FORMAT, 16);
  for (auto i = 0; i < 16; i++) {
    *(p++) = 2;
  }
  pb_end(p);
}

static int update_texture_memory(uint8_t *texture_memory, TextureFormatInfo format, int width, int height) {
  // create source surface
  SDL_Surface *gradient_surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  if (gradient_surface == nullptr) {
    return 1;
  }

  if (SDL_LockSurface(gradient_surface)) {
    return 2;
  }

  // TODO: have different color patterns controlled by alternate gamepad
  // button(s) generate basic gradient pattern
  auto pixels = static_cast<uint32_t *>(gradient_surface->pixels);
  for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++) {
      int x_normal = static_cast<int>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      int y_normal = static_cast<int>(static_cast<float>(y) * 255.0f / static_cast<float>(height));
      pixels[y * width + x] =
          SDL_MapRGBA(gradient_surface->format, y_normal, x_normal, 255 - y_normal, x_normal + y_normal);
    }

  SDL_UnlockSurface(gradient_surface);

  // if conversion required, do so, otherwise use SDL to convert
  if (format.RequireConversion) {
    uint8_t *dstP = texture_memory;

    // TODO: potential reference material -
    // https://github.com/scalablecory/colors/blob/master/color.c
    switch (format.XboxFormat) {
      case NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_CR8YB8CB8YA8:  // YUY2 aka
                                                                  // YUYV
        for (int y = 0; y < height; y++)
          for (int x = 0; x < width; x += 2) {
            uint8_t R0, G0, B0, R1, G1, B1;
            SDL_GetRGB(pixels[y * width + x], gradient_surface->format, &R0, &G0, &B0);
            SDL_GetRGB(pixels[y * width + x + 1], gradient_surface->format, &R1, &G1, &B1);
            dstP[0] = (0.257f * R0) + (0.504f * G0) + (0.098f * B0) + 16;    // Y0
            dstP[1] = -(0.148f * R1) - (0.291f * G1) + (0.439f * B1) + 128;  // U
            dstP[2] = (0.257f * R1) + (0.504f * G1) + (0.098f * B1) + 16;    // Y1
            dstP[3] = (0.439f * R1) - (0.368f * G1) - (0.071f * B1) + 128;   // V
            dstP += 4;
          }
        break;

      case NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_YB8CR8YA8CB8:  // UYVY
        for (int y = 0; y < height; y++)
          for (int x = 0; x < width; x += 2) {
            uint8_t R0, G0, B0, R1, G1, B1;
            SDL_GetRGB(pixels[y * width + x], gradient_surface->format, &R0, &G0, &B0);
            SDL_GetRGB(pixels[y * width + x + 1], gradient_surface->format, &R1, &G1, &B1);
            dstP[0] = -(0.148f * R1) - (0.291f * G1) + (0.439f * B1) + 128;  // U
            dstP[1] = (0.257f * R0) + (0.504f * G0) + (0.098f * B0) + 16;    // Y0
            dstP[2] = (0.439f * R1) - (0.368f * G1) - (0.071f * B1) + 128;   // V
            dstP[3] = (0.257f * R1) + (0.504f * G1) + (0.098f * B1) + 16;    // Y1
            dstP += 4;
          }
        break;

      case NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_G8B8:
        // TODO: for now, just let default gradient happen
        break;

      default:
        SDL_FreeSurface(gradient_surface);
        return 3;
        break;
    }

    // TODO: swizzling

    SDL_FreeSurface(gradient_surface);
  } else {
    // standard SDL conversion to destination format
    SDL_Surface *new_surf = SDL_ConvertSurfaceFormat(gradient_surface, format.SdlFormat, 0);
    SDL_FreeSurface(gradient_surface);
    if (!new_surf) {
      return 4;
    }

    // copy pixels over to texture memory, swizzling if desired
    if (format.XboxSwizzled) {
      swizzle_rect((uint8_t *)new_surf->pixels, new_surf->w, new_surf->h, texture_memory, new_surf->pitch,
                   new_surf->format->BytesPerPixel);
    } else {
      memcpy(texture_memory, new_surf->pixels, new_surf->pitch * new_surf->h);
    }

    SDL_FreeSurface(new_surf);
  }

  return 0;
}

static void init_vertices() {
  // real nv2a hardware seems to cache this and not honor updates? have separate
  // vertex buffers for swizzled and linear for now...
  alloc_vertices = static_cast<Vertex *>(
      MmAllocateContiguousMemoryEx(sizeof(vertices), 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE));
  alloc_vertices_swizzled = static_cast<Vertex *>(
      MmAllocateContiguousMemoryEx(sizeof(vertices), 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE));
  memcpy(alloc_vertices, vertices, sizeof(vertices));
  memcpy(alloc_vertices_swizzled, vertices, sizeof(vertices));
  for (int i = 0; i < kNumVertices; i++) {
    if (alloc_vertices[i].texcoord[0] != 0.0f) {
      alloc_vertices[i].texcoord[0] = static_cast<float>(kTextureWidth);
    }

    if (alloc_vertices[i].texcoord[1] != 0.0f) {
      alloc_vertices[i].texcoord[1] = static_cast<float>(kTextureHeight);
    }
  }
}

static void init_matrices() {
  /* Create view matrix (our camera is static) */
  matrix_unit(m_view);
  create_world_view(m_view, v_cam_pos, v_cam_rot);

  /* Create projection matrix */
  matrix_unit(m_proj);
  create_view_screen(m_proj, (float)kFramebufferWidth / (float)kFramebufferHeight, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                     10000.0f);

  /* Create viewport matrix, combine with projection */
  {
    float m_viewport[4][4];
    matrix_viewport(m_viewport, 0, 0, (float)kFramebufferWidth, (float)kFramebufferHeight, 0, 65536.0f);
    matrix_multiply(m_proj, m_proj, (float *)m_viewport);
  }

  /* Create local->world matrix given our updated object */
  matrix_unit(m_model);
}

/* Construct a viewport transformation matrix */
static void matrix_viewport(float out[4][4], float x, float y, float width, float height, float z_min, float z_max) {
  memset(out, 0, 4 * 4 * sizeof(float));
  out[0][0] = width / 2.0f;
  out[1][1] = height / -2.0f;
  out[2][2] = (z_max - z_min) / 2.0f;
  out[3][3] = 1.0f;
  out[3][0] = x + width / 2.0f;
  out[3][1] = y + height / 2.0f;
  out[3][2] = (z_min + z_max) / 2.0f;
}

/* Load the shader we will render with */
static void init_shader() {
  uint32_t *p;
  int i;

  /* Setup vertex shader */
  uint32_t vs_program[] = {
// clang format off
#include "vs.inl"
      // clang format on
  };

  p = pb_begin();

  /* Set run address of shader */
  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);

  /* Set execution mode */
  p = pb_push1(
      p, NV097_SET_TRANSFORM_EXECUTION_MODE,
      MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
          MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));

  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);
  pb_end(p);

  /* Set cursor and begin copying program */
  p = pb_begin();
  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
  pb_end(p);

  /* Copy program instructions (16-bytes each) */
  for (i = 0; i < sizeof(vs_program) / 16; i++) {
    p = pb_begin();
    pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
    memcpy(p, &vs_program[i * 4], 4 * 4);
    p += 4;
    pb_end(p);
  }

  /* Setup fragment shader */
  p = pb_begin();

// clang format off
#include "ps.inl"
  // clang format on
  pb_end(p);
}

/* Set an attribute pointer */
static void set_attrib_pointer(uint32_t index, uint32_t format, unsigned int size, uint32_t stride, const void *data) {
  uint32_t *p = pb_begin();
  p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + index * 4,
               MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, format) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, size) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, stride));
  p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET + index * 4, (uint32_t)data & 0x03ffffff);
  pb_end(p);
}

/* Send draw commands for the triangles */
static void draw_arrays(unsigned int mode, int start, int count) {
  uint32_t *p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, mode);

  // bit 30 means all params go to same register 0x1810
  p = pb_push1(p, 0x40000000 | NV097_DRAW_ARRAYS,
               MASK(NV097_DRAW_ARRAYS_COUNT, (count - 1)) | MASK(NV097_DRAW_ARRAYS_START_INDEX, start));

  p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);

  pb_end(p);
}

#pragma clang diagnostic pop