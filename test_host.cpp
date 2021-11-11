#include "test_host.h"

// clang format off
#define _USE_MATH_DEFINES
#include <cmath>
// clang format on

#include <SDL.h>
#include <SDL_image.h>
#include <hal/video.h>
#include <strings.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>

#include <cassert>

#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "resources/verts.h"
#include "third_party/swizzle.h"

#define MASK(mask, val) (((val) << (ffs(mask) - 1)) & (mask))
#define MAXRAM 0x03FFAFFF

static void matrix_viewport(float out[4][4], float x, float y, float width, float height, float z_min, float z_max);
static void set_attrib_pointer(uint32_t index, uint32_t format, unsigned int size, uint32_t stride, const void *data);
static void draw_arrays(unsigned int mode, int start, int count);

// bitscan forward
static int bsf(int val){__asm bsf eax, val}

TestHost::TestHost(uint32_t framebuffer_width, uint32_t framebuffer_height, uint32_t texture_width,
                   uint32_t texture_height)
    : framebuffer_width(framebuffer_width),
      framebuffer_height(framebuffer_height),
      texture_width(texture_width),
      texture_height(texture_height) {
  init_vertices();
  init_matrices();
  init_shader();

  // allocate texture memory buffer large enough for all types
  texture_memory_ = static_cast<uint8_t *>(MmAllocateContiguousMemoryEx(texture_width * texture_height * 4, 0, MAXRAM,
                                                                        0, PAGE_WRITECOMBINE | PAGE_READWRITE));
  assert(texture_memory_ && "Failed to allocate texture memory.");
}

void TestHost::init_vertices() {
  // real nv2a hardware seems to cache this and not honor updates? have separate
  // vertex buffers for swizzled and linear for now...
  uint32_t buffer_size = sizeof(Vertex) * kNumVertices;

  alloc_vertices_ = static_cast<Vertex *>(
      MmAllocateContiguousMemoryEx(buffer_size, 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE));
  alloc_vertices_swizzled_ = static_cast<Vertex *>(
      MmAllocateContiguousMemoryEx(buffer_size, 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE));

  memcpy(alloc_vertices_, kBillboardQuad, buffer_size);
  memcpy(alloc_vertices_swizzled_, kBillboardQuad, buffer_size);

  for (int i = 0; i < kNumVertices; i++) {
    alloc_vertices_[i].texcoord[0] *= static_cast<float>(texture_width);
    alloc_vertices_[i].texcoord[1] *= static_cast<float>(texture_height);
  }
}

void TestHost::init_matrices() {
  /* Create view matrix (our camera is static) */
  matrix_unit(m_view);
  create_world_view(m_view, v_cam_pos, v_cam_rot);

  /* Create projection matrix */
  matrix_unit(m_proj);
  create_view_screen(m_proj, (float)framebuffer_width / (float)framebuffer_height, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                     10000.0f);

  /* Create viewport matrix, combine with projection */
  {
    float m_viewport[4][4];
    matrix_viewport(m_viewport, 0, 0, (float)framebuffer_width, (float)framebuffer_height, 0, 65536.0f);
    matrix_multiply(m_proj, m_proj, (float *)m_viewport);
  }

  /* Create local->world matrix given our updated object */
  matrix_unit(m_model);
}

void TestHost::Clear() {
  set_depth_stencil_buffer_region(0xFFFFFFFF, 0, 0, 0, framebuffer_width, framebuffer_height);
  pb_fill(0, 0, framebuffer_width, framebuffer_height, 0xff000000);
  pb_erase_text_screen();
}

void TestHost::StartDraw() {
  pb_wait_for_vbl();
  pb_reset();
  pb_target_back_buffer();

  set_depth_buffer_format(depth_buffer_format);
  Clear();

  while (pb_busy()) {
    /* Wait for completion... */
  }

  SetupTextureStages();
  SendShaderConstants();

  /*
   * Setup vertex attributes
   */
  Vertex *vptr = texture_format.XboxSwizzled ? alloc_vertices_swizzled_ : alloc_vertices_;

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
}

void TestHost::SaveBackbuffer(const char *output_directory, const char *name) {
  char target_file[256] = {0};
  snprintf(target_file, 255, "%s\\%s.png", output_directory, name);
  CreateDirectory(output_directory, nullptr);

  auto buffer = pb_back_buffer();
  auto width = static_cast<int>(pb_back_buffer_width());
  auto height = static_cast<int>(pb_back_buffer_height());
  auto pitch = static_cast<int>(pb_back_buffer_pitch());

  SDL_Surface *surface =
      SDL_CreateRGBSurfaceWithFormatFrom((void *)buffer, width, height, 32, pitch, SDL_PIXELFORMAT_ARGB8888);
  if (IMG_SavePNG(surface, target_file)) {
    assert(!"Failed to save PNG file.");
  }

  SDL_FreeSurface(surface);
}

void TestHost::SetupTextureStages() {
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
  if (texture_format.XboxFormat == NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_CR8YB8CB8YA8 ||
      texture_format.XboxFormat == NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_YB8CR8YA8CB8) {
    p = pb_push1(p, NV097_SET_CONTROL0,
                 MASK(NV097_SET_CONTROL0_COLOR_SPACE_CONVERT, NV097_SET_CONTROL0_COLOR_SPACE_CONVERT_CRYCB_TO_RGB));
  }

  DWORD format_mask =
      MASK(NV097_SET_TEXTURE_FORMAT_CONTEXT_DMA, 1) | MASK(NV097_SET_TEXTURE_FORMAT_CUBEMAP_ENABLE, 0) |
      MASK(NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE, NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE_COLOR) |
      MASK(NV097_SET_TEXTURE_FORMAT_DIMENSIONALITY, 2) |
      MASK(NV097_SET_TEXTURE_FORMAT_COLOR, texture_format.XboxFormat) |
      MASK(NV097_SET_TEXTURE_FORMAT_MIPMAP_LEVELS, 1) |
      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_U, texture_format.XboxSwizzled ? bsf(texture_width) : 0) |
      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_V, texture_format.XboxSwizzled ? bsf(texture_height) : 0) |
      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_P, 0);

  // set stage 0 texture address & format
  p = pb_push2(p, NV20_TCL_PRIMITIVE_3D_TX_OFFSET(0), (DWORD)texture_memory_ & 0x03ffffff, format_mask);

  if (!texture_format.XboxSwizzled) {
    // set stage 0 texture pitch (pitch<<16)
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(0), (texture_format.XboxBpp * texture_width) << 16);

    // set stage 0 texture framebuffer_width & framebuffer_height
    // ((width<<16)|framebuffer_height)
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(0), (texture_width << 16) | texture_height);
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

void TestHost::SendShaderConstants() {
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

void TestHost::SetTextureFormat(const TextureFormatInfo &fmt) { texture_format = fmt; }

void TestHost::SetDepthBufferFormat(uint32_t fmt) { depth_buffer_format = fmt; }

int TestHost::SetTexture(SDL_Surface *gradient_surface) {
  auto pixels = static_cast<uint32_t *>(gradient_surface->pixels);

  // if conversion required, do so, otherwise use SDL to convert
  if (texture_format.RequireConversion) {
    uint8_t *dest = texture_memory_;

    // TODO: potential reference material -
    // https://github.com/scalablecory/colors/blob/master/color.c
    switch (texture_format.XboxFormat) {
      case NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_CR8YB8CB8YA8:  // YUY2 aka
                                                                  // YUYV
      {
        uint32_t *source = pixels;
        for (int y = 0; y < gradient_surface->h; ++y)
          for (int x = 0; x < gradient_surface->w; x += 2, source += 2) {
            uint8_t R0, G0, B0, R1, G1, B1;
            SDL_GetRGB(source[0], gradient_surface->format, &R0, &G0, &B0);
            SDL_GetRGB(source[1], gradient_surface->format, &R1, &G1, &B1);
            dest[0] = (0.257f * R0) + (0.504f * G0) + (0.098f * B0) + 16;    // Y0
            dest[1] = -(0.148f * R1) - (0.291f * G1) + (0.439f * B1) + 128;  // U
            dest[2] = (0.257f * R1) + (0.504f * G1) + (0.098f * B1) + 16;    // Y1
            dest[3] = (0.439f * R1) - (0.368f * G1) - (0.071f * B1) + 128;   // V
            dest += 4;
          }
      } break;

      case NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_YB8CR8YA8CB8:  // UYVY
      {
        uint32_t *source = pixels;
        for (int y = 0; y < gradient_surface->h; ++y)
          for (int x = 0; x < gradient_surface->w; x += 2, source += 2) {
            uint8_t R0, G0, B0, R1, G1, B1;
            SDL_GetRGB(source[0], gradient_surface->format, &R0, &G0, &B0);
            SDL_GetRGB(source[1], gradient_surface->format, &R1, &G1, &B1);
            dest[0] = -(0.148f * R1) - (0.291f * G1) + (0.439f * B1) + 128;  // U
            dest[1] = (0.257f * R0) + (0.504f * G0) + (0.098f * B0) + 16;    // Y0
            dest[2] = (0.439f * R1) - (0.368f * G1) - (0.071f * B1) + 128;   // V
            dest[3] = (0.257f * R1) + (0.504f * G1) + (0.098f * B1) + 16;    // Y1
            dest += 4;
          }
      } break;

      case NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_G8B8:
        // TODO: for now, just let default gradient happen
        break;

      default:
        return 3;
        break;
    }

    // TODO: swizzling
  } else {
    // standard SDL conversion to destination format
    SDL_Surface *new_surf = SDL_ConvertSurfaceFormat(gradient_surface, texture_format.SdlFormat, 0);
    if (!new_surf) {
      return 4;
    }

    // copy pixels over to texture memory, swizzling if desired
    if (texture_format.XboxSwizzled) {
      swizzle_rect((uint8_t *)new_surf->pixels, new_surf->w, new_surf->h, texture_memory_, new_surf->pitch,
                   new_surf->format->BytesPerPixel);
    } else {
      memcpy(texture_memory_, new_surf->pixels, new_surf->pitch * new_surf->h);
    }

    SDL_FreeSurface(new_surf);
  }

  return 0;
}

void TestHost::FinishDrawAndSave(const char *output_directory, const char *name) {
  while (pb_busy()) {
    /* Wait for completion... */
  }

  SaveBackbuffer(output_directory, name);

  /* Swap buffers (if we can) */
  while (pb_finished()) {
    /* Not ready to swap yet */
  }
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

void TestHost::init_shader() {
  /* Load the shader we will render with */
  uint32_t *p;
  int i;

  /* Setup vertex shader */
  // clang format off
  uint32_t vs_program[] = {
#include "vs.inl"
  };
  // clang format on

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
