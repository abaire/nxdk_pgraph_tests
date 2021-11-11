#include "test_host.h"

// clang format off
#define _USE_MATH_DEFINES
#include <cmath>
// clang format on

#include <SDL.h>
#include <SDL_image.h>
#include <strings.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>

#include <cassert>
#include <utility>

#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "shaders/shader_program.h"
#include "third_party/swizzle.h"
#include "vertex_buffer.h"

#define MAXRAM 0x03FFAFFF

static void set_attrib_pointer(uint32_t index, uint32_t format, unsigned int size, uint32_t stride, const void *data);
static void draw_arrays(unsigned int mode, int start, int count);

// bitscan forward
static int bsf(int val){__asm bsf eax, val}

TestHost::TestHost(uint32_t framebuffer_width, uint32_t framebuffer_height, uint32_t texture_width,
                   uint32_t texture_height)
    : framebuffer_width_(framebuffer_width),
      framebuffer_height_(framebuffer_height),
      texture_width_(texture_width),
      texture_height_(texture_height) {
  // allocate texture memory buffer large enough for all types
  texture_memory_ = static_cast<uint8_t *>(MmAllocateContiguousMemoryEx(texture_width * texture_height * 4, 0, MAXRAM,
                                                                        0, PAGE_WRITECOMBINE | PAGE_READWRITE));
  assert(texture_memory_ && "Failed to allocate texture memory.");
}

TestHost::~TestHost() {
  vertex_buffer_.reset();
  if (texture_memory_) {
    MmFreeContiguousMemory(texture_memory_);
  }
}

void TestHost::SetDepthStencilRegion(uint32_t depth_value, uint8_t stencil_value, uint32_t left, uint32_t top,
                                     uint32_t width, uint32_t height) const {
  if (!width || width > framebuffer_width_) {
    width = framebuffer_width_;
  }
  if (!height || height > framebuffer_height_) {
    height = framebuffer_height_;
  }

  set_depth_stencil_buffer_region(depth_value, stencil_value, left, top, width, height);
}

void TestHost::SetFillColorRegion(uint32_t argb, uint32_t left, uint32_t top, uint32_t width, uint32_t height) const {
  if (!width || width > framebuffer_width_) {
    width = framebuffer_width_;
  }
  if (!height || height > framebuffer_height_) {
    height = framebuffer_height_;
  }
  pb_fill(static_cast<int>(left), static_cast<int>(top), static_cast<int>(width), static_cast<int>(height), argb);
}

void TestHost::EraseText() { pb_erase_text_screen(); }

void TestHost::Clear(uint32_t argb, uint32_t depth_value, uint8_t stencil_value) const {
  SetDepthStencilRegion(depth_value, stencil_value);
  SetFillColorRegion(argb);
  EraseText();
}

void TestHost::PrepareDraw(uint32_t argb, uint32_t depth_value, uint8_t stencil_value) {
  pb_wait_for_vbl();
  pb_reset();
  pb_target_back_buffer();

  set_depth_buffer_format(depth_buffer_format_);
  Clear(argb, depth_value, stencil_value);

  while (pb_busy()) {
    /* Wait for completion... */
  }

  SetupTextureStages();
  assert(shader_program_ && "SetShaderProgram must be called before PrepareDraw");
  shader_program_->PrepareDraw();
}

void TestHost::DrawVertices() {
  assert(vertex_buffer_ && "Vertex buffer must be set before calling DrawVertices.");

  Vertex *vptr = texture_format_.XboxSwizzled ? vertex_buffer_->normalized_vertex_buffer_ : vertex_buffer_->linear_vertex_buffer_;

  set_attrib_pointer(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, sizeof(Vertex),
                     &vptr[0].pos);

  set_attrib_pointer(NV2A_VERTEX_ATTR_TEXTURE0, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 2, sizeof(Vertex),
                     &vptr[0].texcoord);

  set_attrib_pointer(NV2A_VERTEX_ATTR_NORMAL, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, sizeof(Vertex),
                     &vptr[0].normal);

  set_attrib_pointer(NV2A_VERTEX_ATTR_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, sizeof(Vertex),
                     &vptr[0].diffuse);

  int num_vertices = static_cast<int>(vertex_buffer_->num_vertices_);

  // Looks like the count field is a single byte, so large vertex arrays need to be batched.
  if (num_vertices < 252) {
    draw_arrays(NV097_SET_BEGIN_END_OP_TRIANGLES, 0, num_vertices);
  } else {
    int start = 0;
    while (start < num_vertices) {
      int count = (num_vertices > 252 ? 252 : num_vertices);
      draw_arrays(NV097_SET_BEGIN_END_OP_TRIANGLES, start, count);
      start += count;
    }
  }
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

  // Enable alpha blending functionality
  p = pb_push1(p, NV097_SET_BLEND_ENABLE, true);

  // Set the alpha blend source (s) and destination (d) factors
  p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
  p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_SRC_ALPHA);

  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);

  // yuv requires color space conversion
  if (texture_format_.XboxFormat == NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_CR8YB8CB8YA8 ||
      texture_format_.XboxFormat == NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_YB8CR8YA8CB8) {
    p = pb_push1(p, NV097_SET_CONTROL0,
                 MASK(NV097_SET_CONTROL0_COLOR_SPACE_CONVERT, NV097_SET_CONTROL0_COLOR_SPACE_CONVERT_CRYCB_TO_RGB));
  }

  DWORD format_mask =
      MASK(NV097_SET_TEXTURE_FORMAT_CONTEXT_DMA, 1) | MASK(NV097_SET_TEXTURE_FORMAT_CUBEMAP_ENABLE, 0) |
      MASK(NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE, NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE_COLOR) |
      MASK(NV097_SET_TEXTURE_FORMAT_DIMENSIONALITY, 2) |
      MASK(NV097_SET_TEXTURE_FORMAT_COLOR, texture_format_.XboxFormat) |
      MASK(NV097_SET_TEXTURE_FORMAT_MIPMAP_LEVELS, 1) |
      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_U, texture_format_.XboxSwizzled ? bsf(texture_width_) : 0) |
      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_V, texture_format_.XboxSwizzled ? bsf(texture_height_) : 0) |
      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_P, 0);

  // set stage 0 texture address & format
  p = pb_push2(p, NV20_TCL_PRIMITIVE_3D_TX_OFFSET(0), (DWORD)texture_memory_ & 0x03ffffff, format_mask);

  if (!texture_format_.XboxSwizzled) {
    // set stage 0 texture pitch (pitch<<16)
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(0), (texture_format_.XboxBpp * texture_width_) << 16);

    // set stage 0 texture framebuffer_width_ & framebuffer_height_
    // ((width<<16)|framebuffer_height_)
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(0), (texture_width_ << 16) | texture_height_);
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

void TestHost::SetTextureFormat(const TextureFormatInfo &fmt) { texture_format_ = fmt; }

void TestHost::SetDepthBufferFormat(uint32_t fmt) { depth_buffer_format_ = fmt; }

int TestHost::SetTexture(SDL_Surface *gradient_surface) {
  auto pixels = static_cast<uint32_t *>(gradient_surface->pixels);

  // if conversion required, do so, otherwise use SDL to convert
  if (texture_format_.RequireConversion) {
    uint8_t *dest = texture_memory_;

    // TODO: potential reference material -
    // https://github.com/scalablecory/colors/blob/master/color.c
    switch (texture_format_.XboxFormat) {
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
    SDL_Surface *new_surf = SDL_ConvertSurfaceFormat(gradient_surface, texture_format_.SdlFormat, 0);
    if (!new_surf) {
      return 4;
    }

    // copy pixels over to texture memory, swizzling if desired
    if (texture_format_.XboxSwizzled) {
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

void TestHost::SetShaderProgram(std::shared_ptr<ShaderProgram> program) {
  shader_program_ = std::move(program);
  shader_program_->Activate();
}

std::shared_ptr<VertexBuffer> TestHost::AllocateVertexBuffer(uint32_t num_vertices) {
  vertex_buffer_ = std::make_shared<VertexBuffer>(num_vertices);
  return vertex_buffer_;
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
