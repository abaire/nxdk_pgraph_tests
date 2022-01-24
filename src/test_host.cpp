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

#include <algorithm>
#include <utility>

#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "shaders/shader_program.h"
#include "swizzle.h"
#include "vertex_buffer.h"

#define MAXRAM 0x03FFAFFF
#define MAX_FILE_PATH_SIZE 248
static void set_attrib_pointer(uint32_t index, uint32_t format, uint32_t size, uint32_t stride, const void *data);
static void clear_attrib(uint32_t index);

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
  ASSERT(texture_memory_ && "Failed to allocate texture memory.");

  matrix_unit(fixed_function_model_view_matrix_);
  matrix_unit(fixed_function_projection_matrix_);
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

  set_depth_stencil_buffer_region(depth_buffer_format_, depth_value, stencil_value, left, top, width, height);
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
  SetupControl0();
  SetFillColorRegion(argb);
  SetDepthStencilRegion(depth_value, stencil_value);
  EraseText();
}

void TestHost::PrepareDraw(uint32_t argb, uint32_t depth_value, uint8_t stencil_value) {
  pb_wait_for_vbl();
  pb_reset();

  SetupTextureStages();

  // Override the values set in pb_init. Unfortunately the default is not exposed and must be recreated here.
  auto p = pb_begin();
  uint32_t color_format = NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8;
  uint32_t frame_buffer_format =
      (NV097_SET_SURFACE_FORMAT_TYPE_PITCH << 8) | (depth_buffer_format_ << 4) | color_format;
  uint32_t fbv_flag = 0;
  p = pb_push1(p, NV097_SET_SURFACE_FORMAT, frame_buffer_format | fbv_flag);

  uint32_t max_depth;
  if (depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    if (depth_buffer_mode_float_) {
      max_depth = 0x43FFF800;  // z16_max as 32-bit float.
    } else {
      *((float *)&max_depth) = static_cast<float>(0xFFFF);
    }
  } else {
    if (depth_buffer_mode_float_) {
      // max_depth = 0x7F7FFF80;  // z24_max as 32-bit float.
      max_depth = 0x7149F2CA;  // Observed value, 1e+30 (also used for directional lighting as "infinity").
    } else {
      *((float *)&max_depth) = static_cast<float>(0x00FFFFFF);
    }
  }

  p = pb_push1(p, NV097_SET_CLIP_MIN, 0);
  p = pb_push1(p, NV097_SET_CLIP_MAX, max_depth);

  pb_end(p);

  Clear(argb, depth_value, stencil_value);

  if (shader_program_) {
    shader_program_->PrepareDraw();
  }

  while (pb_busy()) {
    /* Wait for completion... */
  }
}

void TestHost::SetVertexBufferAttributes(uint32_t enabled_fields) {
  if (!vertex_buffer_->IsCacheValid()) {
    auto p = pb_begin();
    p = pb_push1(p, NV097_BREAK_VERTEX_BUFFER_CACHE, 0);
    pb_end(p);
    vertex_buffer_->SetCacheValid();
  }

  Vertex *vptr =
      texture_format_.xbox_swizzled ? vertex_buffer_->normalized_vertex_buffer_ : vertex_buffer_->linear_vertex_buffer_;

  if (enabled_fields & POSITION) {
    set_attrib_pointer(NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
                       vertex_buffer_->position_count_, sizeof(Vertex), &vptr[0].pos);
  } else {
    clear_attrib(NV2A_VERTEX_ATTR_POSITION);
  }

  if (enabled_fields & WEIGHT) {
    set_attrib_pointer(NV2A_VERTEX_ATTR_WEIGHT, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, sizeof(Vertex),
                       &vptr[0].weight);
  } else {
    clear_attrib(NV2A_VERTEX_ATTR_WEIGHT);
  }

  if (enabled_fields & NORMAL) {
    set_attrib_pointer(NV2A_VERTEX_ATTR_NORMAL, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, sizeof(Vertex),
                       &vptr[0].normal);
  } else {
    clear_attrib(NV2A_VERTEX_ATTR_NORMAL);
  }

  if (enabled_fields & DIFFUSE) {
    set_attrib_pointer(NV2A_VERTEX_ATTR_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, sizeof(Vertex),
                       &vptr[0].diffuse);
  } else {
    clear_attrib(NV2A_VERTEX_ATTR_DIFFUSE);
  }

  if (enabled_fields & SPECULAR) {
    set_attrib_pointer(NV2A_VERTEX_ATTR_SPECULAR, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, sizeof(Vertex),
                       &vptr[0].specular);
  } else {
    clear_attrib(NV2A_VERTEX_ATTR_SPECULAR);
  }

  if (enabled_fields & FOG_COORD) {
    set_attrib_pointer(NV2A_VERTEX_ATTR_FOG_COORD, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 1, sizeof(Vertex),
                       &vptr[0].fog_coord);
  } else {
    clear_attrib(NV2A_VERTEX_ATTR_FOG_COORD);
  }

  if (enabled_fields & POINT_SIZE) {
    set_attrib_pointer(NV2A_VERTEX_ATTR_POINT_SIZE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 1, sizeof(Vertex),
                       &vptr[0].point_size);
  } else {
    clear_attrib(NV2A_VERTEX_ATTR_POINT_SIZE);
  }

  //  if (enabled_fields & BACK_DIFFUSE) {
  //    set_attrib_pointer(NV2A_VERTEX_ATTR_BACK_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, sizeof(Vertex),
  //                       &vptr[0].back_diffuse);
  //  } else {
  clear_attrib(NV2A_VERTEX_ATTR_BACK_DIFFUSE);
  //  }

  //  if (enabled_fields & BACK_SPECULAR) {
  //    set_attrib_pointer(NV2A_VERTEX_ATTR_BACK_SPECULAR, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, sizeof(Vertex),
  //                       &vptr[0].back_specular);
  //  } else {
  clear_attrib(NV2A_VERTEX_ATTR_BACK_SPECULAR);
  //  }

  if (enabled_fields & TEXCOORD0) {
    set_attrib_pointer(NV2A_VERTEX_ATTR_TEXTURE0, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 2, sizeof(Vertex),
                       &vptr[0].texcoord0);
  } else {
    clear_attrib(NV2A_VERTEX_ATTR_TEXTURE0);
  }

  if (enabled_fields & TEXCOORD1) {
    set_attrib_pointer(NV2A_VERTEX_ATTR_TEXTURE1, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 2, sizeof(Vertex),
                       &vptr[0].texcoord1);
  } else {
    clear_attrib(NV2A_VERTEX_ATTR_TEXTURE1);
  }

  if (enabled_fields & TEXCOORD2) {
    set_attrib_pointer(NV2A_VERTEX_ATTR_TEXTURE2, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 2, sizeof(Vertex),
                       &vptr[0].texcoord2);
  } else {
    clear_attrib(NV2A_VERTEX_ATTR_TEXTURE2);
  }

  if (enabled_fields & TEXCOORD3) {
    set_attrib_pointer(NV2A_VERTEX_ATTR_TEXTURE3, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 2, sizeof(Vertex),
                       &vptr[0].texcoord3);
  } else {
    clear_attrib(NV2A_VERTEX_ATTR_TEXTURE3);
  }

  // Matching observed behavior. This is probably unnecessary as these are never set by pbkit.
  clear_attrib(13);
  clear_attrib(14);
  clear_attrib(15);
}

void TestHost::DrawArrays(uint32_t enabled_vertex_fields, DrawPrimitive primitive) {
  if (shader_program_) {
    shader_program_->PrepareDraw();
  }

  ASSERT(vertex_buffer_ && "Vertex buffer must be set before calling DrawArrays.");
  static constexpr int kVerticesPerPush = 120;

  SetVertexBufferAttributes(enabled_vertex_fields);
  int num_vertices = static_cast<int>(vertex_buffer_->num_vertices_);

  int start = 0;
  while (start < num_vertices) {
    int count = std::min(num_vertices - start, kVerticesPerPush);

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_BEGIN_END, primitive);

    p = pb_push1(p, NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_DRAW_ARRAYS),
                 MASK(NV097_DRAW_ARRAYS_COUNT, count - 1) | MASK(NV097_DRAW_ARRAYS_START_INDEX, start));

    p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);

    pb_end(p);

    start += count;
  }
}

void TestHost::DrawInlineBuffer(uint32_t enabled_vertex_fields, DrawPrimitive primitive) {
  if (shader_program_) {
    shader_program_->PrepareDraw();
  }

  ASSERT(vertex_buffer_ && "Vertex buffer must be set before calling DrawInlineBuffer.");
  SetVertexBufferAttributes(enabled_vertex_fields);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, primitive);
  pb_end(p);

  auto vertex = vertex_buffer_->Lock();
  for (auto i = 0; i < vertex_buffer_->GetNumVertices(); ++i, ++vertex) {
    if (enabled_vertex_fields & WEIGHT) {
      SetWeight(vertex->weight[0]);
    }
    if (enabled_vertex_fields & NORMAL) {
      SetNormal(vertex->normal[0], vertex->normal[1], vertex->normal[2]);
    }
    if (enabled_vertex_fields & DIFFUSE) {
      SetDiffuse(vertex->diffuse[0], vertex->diffuse[1], vertex->diffuse[2], vertex->diffuse[3]);
    }
    if (enabled_vertex_fields & SPECULAR) {
      SetSpecular(vertex->specular[0], vertex->specular[1], vertex->specular[2], vertex->specular[3]);
    }
    if (enabled_vertex_fields & FOG_COORD) {
      SetFogCoord(vertex->fog_coord);
    }
    if (enabled_vertex_fields & POINT_SIZE) {
      SetPointSize(vertex->point_size);
    }
    if (enabled_vertex_fields & TEXCOORD0) {
      SetTexCoord0(vertex->texcoord0[0], vertex->texcoord0[1]);
    }
    if (enabled_vertex_fields & TEXCOORD1) {
      SetTexCoord0(vertex->texcoord1[0], vertex->texcoord1[1]);
    }
    if (enabled_vertex_fields & TEXCOORD2) {
      SetTexCoord0(vertex->texcoord2[0], vertex->texcoord2[1]);
    }
    if (enabled_vertex_fields & TEXCOORD3) {
      SetTexCoord0(vertex->texcoord3[0], vertex->texcoord3[1]);
    }

    // Setting the position locks in the previously set values and must be done last.
    if (enabled_vertex_fields & POSITION) {
      if (vertex_buffer_->position_count_ == 3) {
        SetVertex(vertex->pos[0], vertex->pos[1], vertex->pos[2]);
      } else {
        SetVertex(vertex->pos[0], vertex->pos[1], vertex->pos[2], vertex->pos[3]);
      }
    }
  }
  vertex_buffer_->Unlock();
  vertex_buffer_->SetCacheValid();

  p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  pb_end(p);
}

void TestHost::DrawInlineArray(uint32_t enabled_vertex_fields, DrawPrimitive primitive) {
  if (shader_program_) {
    shader_program_->PrepareDraw();
  }

  ASSERT(vertex_buffer_ && "Vertex buffer must be set before calling DrawInlineArray.");
  static constexpr int kElementsPerPush = 64;

  SetVertexBufferAttributes(enabled_vertex_fields);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, primitive);

  int num_pushed = 0;
  auto vertex = vertex_buffer_->Lock();
  for (auto i = 0; i < vertex_buffer_->GetNumVertices(); ++i, ++vertex) {
    // Note: Ordering is important and must follow the NV2A_VERTEX_ATTR_POSITION, ... ordering.
    if (enabled_vertex_fields & POSITION) {
      auto vals = (uint32_t *)vertex->pos;
      if (vertex_buffer_->position_count_ == 3) {
        p = pb_push3(p, NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), vals[0], vals[1], vals[2]);
        num_pushed += 3;
      } else {
        p = pb_push4(p, NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), vals[0], vals[1], vals[2], vals[3]);
        num_pushed += 4;
      }
    }
    if (enabled_vertex_fields & WEIGHT) {
      ASSERT(!"WEIGHT not supported");
    }
    if (enabled_vertex_fields & NORMAL) {
      auto vals = (uint32_t *)vertex->normal;
      p = pb_push3(p, NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), vals[0], vals[1], vals[2]);
      num_pushed += 3;
    }
    if (enabled_vertex_fields & DIFFUSE) {
      // TODO: Enable sending as a DWORD by changing the type and size sent via SetVertexBufferAttributes.
      auto vals = (uint32_t *)vertex->diffuse;
      p = pb_push4(p, NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), vals[0], vals[1], vals[2], vals[3]);
      num_pushed += 4;
    }
    if (enabled_vertex_fields & SPECULAR) {
      // TODO: Enable sending as a DWORD by changing the type and size sent via SetVertexBufferAttributes.
      auto vals = (uint32_t *)vertex->specular;
      p = pb_push4(p, NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), vals[0], vals[1], vals[2], vals[3]);
      num_pushed += 4;
    }
    if (enabled_vertex_fields & FOG_COORD) {
      ASSERT(!"FOG_COORD not supported");
    }
    if (enabled_vertex_fields & POINT_SIZE) {
      ASSERT(!"POINT_SIZE not supported");
    }
    if (enabled_vertex_fields & BACK_DIFFUSE) {
      ASSERT(!"BACK_DIFFUSE not supported");
    }
    if (enabled_vertex_fields & BACK_SPECULAR) {
      ASSERT(!"BACK_SPECULAR not supported");
    }
    if (enabled_vertex_fields & TEXCOORD0) {
      auto vals = (uint32_t *)vertex->texcoord0;
      p = pb_push2(p, NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), vals[0], vals[1]);
      num_pushed += 2;
    }
    if (enabled_vertex_fields & TEXCOORD1) {
      auto vals = (uint32_t *)vertex->texcoord1;
      p = pb_push2(p, NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), vals[0], vals[1]);
      num_pushed += 2;
    }
    if (enabled_vertex_fields & TEXCOORD2) {
      auto vals = (uint32_t *)vertex->texcoord2;
      p = pb_push2(p, NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), vals[0], vals[1]);
      num_pushed += 2;
    }
    if (enabled_vertex_fields & TEXCOORD3) {
      auto vals = (uint32_t *)vertex->texcoord3;
      p = pb_push2(p, NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY), vals[0], vals[1]);
      num_pushed += 2;
    }

    if (num_pushed > kElementsPerPush) {
      pb_end(p);
      p = pb_begin();
      num_pushed = 0;
    }
  }
  vertex_buffer_->Unlock();
  vertex_buffer_->SetCacheValid();

  p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  pb_end(p);
}

void TestHost::DrawInlineElements16(const std::vector<uint32_t> &indices, uint32_t enabled_vertex_fields,
                                    DrawPrimitive primitive) {
  if (shader_program_) {
    shader_program_->PrepareDraw();
  }

  ASSERT(vertex_buffer_ && "Vertex buffer must be set before calling DrawInlineElements.");
  static constexpr int kIndicesPerPush = 64;

  SetVertexBufferAttributes(enabled_vertex_fields);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, primitive);

  ASSERT(indices.size() < 0x7FFFFFFF);
  int indices_remaining = static_cast<int>(indices.size());
  int num_pushed = 0;
  const uint32_t *next_index = indices.data();
  while (indices_remaining >= 2) {
    if (num_pushed++ > kIndicesPerPush) {
      pb_end(p);
      p = pb_begin();
      num_pushed = 0;
    }

    uint32_t index_pair = *next_index++ & 0xFFFF;
    index_pair += *next_index++ << 16;

    p = pb_push1(p, NV097_ARRAY_ELEMENT16, index_pair);

    indices_remaining -= 2;
  }

  if (indices_remaining) {
    p = pb_push1(p, NV097_ARRAY_ELEMENT32, *next_index);
  }

  p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  pb_end(p);
}

void TestHost::DrawInlineElements32(const std::vector<uint32_t> &indices, uint32_t enabled_vertex_fields,
                                    DrawPrimitive primitive) {
  if (shader_program_) {
    shader_program_->PrepareDraw();
  }

  ASSERT(vertex_buffer_ && "Vertex buffer must be set before calling DrawInlineElementsForce32.");
  static constexpr int kIndicesPerPush = 64;

  SetVertexBufferAttributes(enabled_vertex_fields);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, primitive);

  int num_pushed = 0;
  for (auto index : indices) {
    if (num_pushed++ > kIndicesPerPush) {
      pb_end(p);
      p = pb_begin();
      num_pushed = 0;
    }

    p = pb_push1(p, NV097_ARRAY_ELEMENT32, index);
  }

  p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  pb_end(p);
}

void TestHost::SetVertex(float x, float y, float z) const {
  auto p = pb_begin();
  p = pb_push3(p, NV097_SET_VERTEX3F, *(uint32_t *)&x, *(uint32_t *)&y, *(uint32_t *)&z);
  pb_end(p);
}

void TestHost::SetVertex(float x, float y, float z, float w) const {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_VERTEX4F, x, y, z, w);
  pb_end(p);
}

void TestHost::SetWeight(float w1, float w2, float w3, float w4) const {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_WEIGHT4F, w1, w2, w3, w4);
  pb_end(p);
}

void TestHost::SetWeight(float w) const {
  auto p = pb_begin();
  p = pb_push1f(p, NV097_SET_WEIGHT1F, w);
  pb_end(p);
}

void TestHost::SetNormal(float x, float y, float z) const {
  auto p = pb_begin();
  p = pb_push3(p, NV097_SET_NORMAL3F, *(uint32_t *)&x, *(uint32_t *)&y, *(uint32_t *)&z);
  pb_end(p);
}

void TestHost::SetDiffuse(float r, float g, float b, float a) const {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_DIFFUSE_COLOR4F, r, g, b, a);
  pb_end(p);
}

void TestHost::SetSpecular(float r, float g, float b, float a) const {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_SPECULAR_COLOR4F, r, g, b, a);
  pb_end(p);
}

void TestHost::SetFogCoord(float fc) const {
  auto p = pb_begin();
  p = pb_push1f(p, NV097_SET_FOG_COORD, fc);
  pb_end(p);
}

void TestHost::SetPointSize(float ps) const {
  auto p = pb_begin();
  p = pb_push1f(p, NV097_SET_POINT_SIZE, ps);
  pb_end(p);
}

void TestHost::SetTexCoord0(float u, float v) const {
  auto p = pb_begin();
  p = pb_push2(p, NV097_SET_TEX_COORD0, *(uint32_t *)&u, *(uint32_t *)&v);
  pb_end(p);
}

void TestHost::SetTexCoord1(float u, float v) const {
  auto p = pb_begin();
  p = pb_push2(p, NV097_SET_TEX_COORD1, *(uint32_t *)&u, *(uint32_t *)&v);
  pb_end(p);
}

void TestHost::SetTexCoord2(float u, float v) const {
  auto p = pb_begin();
  p = pb_push2(p, NV097_SET_TEX_COORD2, *(uint32_t *)&u, *(uint32_t *)&v);
  pb_end(p);
}

void TestHost::SetTexCoord3(float u, float v) const {
  auto p = pb_begin();
  p = pb_push2(p, NV097_SET_TEX_COORD3, *(uint32_t *)&u, *(uint32_t *)&v);
  pb_end(p);
}

void TestHost::EnsureFolderExists(const std::string &folder_path) {
  if (folder_path.length() > MAX_FILE_PATH_SIZE) {
    ASSERT(!"Folder Path is too long.");
  }

  char buffer[MAX_FILE_PATH_SIZE + 1] = {0};
  const char *path_start = folder_path.c_str();
  const char *slash = strchr(path_start, '\\');
  slash = strchr(slash + 1, '\\');

  while (slash) {
    strncpy(buffer, path_start, slash - path_start);
    if (!CreateDirectory(buffer, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
      ASSERT(!"Failed to create output directory.");
    }

    slash = strchr(slash + 1, '\\');
  }

  // Handle case where there was no trailing slash.
  if (!CreateDirectory(path_start, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
    ASSERT(!"Failed to create output directory.");
  }
}

// Returns the full output filepath including the filename
// Creates output directory if it does not exist
std::string TestHost::PrepareSaveFilePNG(std::string output_directory, const std::string &filename) {
  EnsureFolderExists(output_directory);

  output_directory += "\\";
  output_directory += filename;
  output_directory += ".png";

  if (output_directory.length() > MAX_FILE_PATH_SIZE) {
    ASSERT(!"Full save file path is too long.");
  }

  return output_directory;
}

void TestHost::SaveBackBuffer(const std::string &output_directory, const std::string &name) {
  auto target_file = PrepareSaveFilePNG(output_directory, name);

  auto buffer = pb_agp_access(pb_back_buffer());
  auto width = static_cast<int>(pb_back_buffer_width());
  auto height = static_cast<int>(pb_back_buffer_height());
  auto pitch = static_cast<int>(pb_back_buffer_pitch());

  SDL_Surface *surface =
      SDL_CreateRGBSurfaceWithFormatFrom((void *)buffer, width, height, 32, pitch, SDL_PIXELFORMAT_ARGB8888);
  if (IMG_SavePNG(surface, target_file.c_str())) {
    PrintMsg("Failed to save PNG file '%s'\n", target_file.c_str());
    ASSERT(!"Failed to save PNG file.");
  }

  SDL_FreeSurface(surface);
}

void TestHost::SaveZBuffer(const std::string &output_directory, const std::string &name) const {
  auto target_file = PrepareSaveFilePNG(output_directory, name);

  auto buffer = pb_agp_access(pb_depth_stencil_buffer());
  auto size = pb_depth_stencil_size();

  PrintMsg("Saving z-buffer to %s. Size: %lu. Pitch %lu.\n", target_file.c_str(), size, pb_depth_stencil_pitch());

  // The Z buffer set up by pbkit uses a 32bpp pitch regardless of the actual format being used by the HW.
  int pitch = static_cast<int>(pb_depth_stencil_pitch());
  int depth = depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? 16 : 32;
  int format =
      depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? SDL_PIXELFORMAT_RGB565 : SDL_PIXELFORMAT_ARGB8888;

  SDL_Surface *surface =
      SDL_CreateRGBSurfaceWithFormatFrom((void *)buffer, static_cast<int>(framebuffer_width_),
                                         static_cast<int>(framebuffer_height_), depth, pitch, format);

  if (IMG_SavePNG(surface, target_file.c_str())) {
    PrintMsg("Failed to save PNG file '%s'\n", target_file.c_str());
    ASSERT(!"Failed to save PNG file.");
  }

  SDL_FreeSurface(surface);
}

void TestHost::SetupControl0() const {
  // yuv requires color space conversion
  bool requires_colorspace_conversion =
      texture_format_.xbox_format == NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_CR8YB8CB8YA8 ||
      texture_format_.xbox_format == NV097_SET_TEXTURE_FORMAT_COLOR_LC_IMAGE_YB8CR8YA8CB8;

  uint32_t control0 = NV097_SET_CONTROL0_STENCIL_WRITE_ENABLE;
  control0 |= MASK(NV097_SET_CONTROL0_Z_FORMAT,
                   depth_buffer_mode_float_ ? NV097_SET_CONTROL0_Z_FORMAT_FLOAT : NV097_SET_CONTROL0_Z_FORMAT_FIXED);

  if (requires_colorspace_conversion) {
    control0 |= MASK(NV097_SET_CONTROL0_COLOR_SPACE_CONVERT, NV097_SET_CONTROL0_COLOR_SPACE_CONVERT_CRYCB_TO_RGB);
  }
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTROL0, control0);
  pb_end(p);
}

void TestHost::SetupTextureStages() const {
  if (!texture_format_.xbox_bpp) {
    PrintMsg("No texture format specified. This will cause an invalid pgraph state exception and a crash.");
    ASSERT(!"No texture format specified. This will cause an invalid pgraph state exception and a crash.");
  }

  auto p = pb_begin();
  // FIXME: Use constants instead of the hardcoded values below

  uint32_t format_mask =
      MASK(NV097_SET_TEXTURE_FORMAT_CONTEXT_DMA, 1) | MASK(NV097_SET_TEXTURE_FORMAT_CUBEMAP_ENABLE, 0) |
      MASK(NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE, NV097_SET_TEXTURE_FORMAT_BORDER_SOURCE_COLOR) |
      MASK(NV097_SET_TEXTURE_FORMAT_DIMENSIONALITY, 2) |
      MASK(NV097_SET_TEXTURE_FORMAT_COLOR, texture_format_.xbox_format) |
      MASK(NV097_SET_TEXTURE_FORMAT_MIPMAP_LEVELS, 1) |
      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_U, texture_format_.xbox_swizzled ? bsf(texture_width_) : 0) |
      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_V, texture_format_.xbox_swizzled ? bsf(texture_height_) : 0) |
      MASK(NV097_SET_TEXTURE_FORMAT_BASE_SIZE_P, 0);

  // set stage 0 texture address & format
  p = pb_push2(p, NV20_TCL_PRIMITIVE_3D_TX_OFFSET(0), (DWORD)texture_memory_ & 0x03ffffff, format_mask);

  if (!texture_format_.xbox_swizzled) {
    // set stage 0 texture pitch (pitch<<16)
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(0), (texture_format_.xbox_bpp * texture_width_) << 16);

    // set stage 0 texture width & height
    // ((width<<16)|height)
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(0), (texture_width_ << 16) | texture_height_);
  }

  // set stage 0 texture modes
  // (0x0W0V0U wrapping: 1=wrap 2=mirror 3=clamp 4=border 5=clamp to edge)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(0), 0x00030303);

  // set stage 0 texture enable flags
  if (texture_stage_enabled_[0]) {
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(0),
                 NV097_SET_TEXTURE_CONTROL0_ENABLE | NV097_SET_TEXTURE_CONTROL0_MAX_LOD_CLAMP);
  } else {
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(0), 0);
  }

  // set stage 0 texture filters (AA!)
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(0), 0x04074000);

  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(1), 0);
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(2), 0);
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(3), 0);

  pb_end(p);
}

void TestHost::SetTextureFormat(const TextureFormatInfo &fmt) { texture_format_ = fmt; }

void TestHost::SetDepthBufferFormat(uint32_t fmt) {
  depth_buffer_format_ = fmt;

  switch (fixed_function_matrix_mode_) {
    case MATRIX_MODE_USER:
      break;

    case MATRIX_MODE_DEFAULT_XDK:
      SetXDKDefaultViewportAndFixedFunctionMatrices();
      break;

    case MATRIX_MODE_DEFAULT_NXDK:
      SetDefaultViewportAndFixedFunctionMatrices();
      break;
  }
}

void TestHost::SetDepthBufferFloatMode(bool enabled) { depth_buffer_mode_float_ = enabled; }

int TestHost::SetTexture(SDL_Surface *gradient_surface) {
  auto pixels = static_cast<uint32_t *>(gradient_surface->pixels);

  // if conversion required, do so, otherwise use SDL to convert
  if (texture_format_.require_conversion) {
    uint8_t *dest = texture_memory_;

    // TODO: potential reference material -
    // https://github.com/scalablecory/colors/blob/master/color.c
    switch (texture_format_.xbox_format) {
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
    SDL_Surface *new_surf = SDL_ConvertSurfaceFormat(gradient_surface, texture_format_.sdl_format, 0);
    if (!new_surf) {
      return 4;
    }

    // copy pixels over to texture memory, swizzling if desired
    if (texture_format_.xbox_swizzled) {
      swizzle_rect((uint8_t *)new_surf->pixels, new_surf->w, new_surf->h, texture_memory_, new_surf->pitch,
                   new_surf->format->BytesPerPixel);
    } else {
      memcpy(texture_memory_, new_surf->pixels, new_surf->pitch * new_surf->h);
    }

    SDL_FreeSurface(new_surf);
  }

  return 0;
}

void TestHost::FinishDraw(bool allow_saving, const std::string &output_directory, const std::string &name,
                          const std::string &z_buffer_name) {
  bool perform_save = allow_saving && save_results_;
  if (!perform_save) {
    pb_printat(0, 55, (char *)"ns");
    pb_draw_text_screen();
  }

  while (pb_busy()) {
    /* Wait for completion... */
  }

  if (perform_save) {
    // TODO: See why waiting for tiles to be non-busy results in the screen not updating anymore.
    // In theory this should wait for all tiles to be rendered before capturing.
    pb_wait_for_vbl();

    SaveBackBuffer(output_directory, name);

    if (!z_buffer_name.empty()) {
      SaveZBuffer(output_directory, z_buffer_name);
    }
  }

  /* Swap buffers (if we can) */
  while (pb_finished()) {
    /* Not ready to swap yet */
  }
}

void TestHost::SetShaderProgram(std::shared_ptr<ShaderProgram> program) {
  shader_program_ = std::move(program);

  if (shader_program_) {
    shader_program_->Activate();
  } else {
    // Use the untextured shader to set up the combiner state.
    ShaderProgram::LoadUntexturedPixelShader();
    ShaderProgram::DisablePixelShader();

    auto p = pb_begin();
    p = pb_push1(
        p, NV097_SET_TRANSFORM_EXECUTION_MODE,
        MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_FIXED) |
            MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0x0);
    p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 0x0);
    pb_end(p);
  }
}

std::shared_ptr<VertexBuffer> TestHost::AllocateVertexBuffer(uint32_t num_vertices) {
  vertex_buffer_.reset();
  vertex_buffer_ = std::make_shared<VertexBuffer>(num_vertices);
  return vertex_buffer_;
}

void TestHost::SetVertexBuffer(std::shared_ptr<VertexBuffer> buffer) { vertex_buffer_ = std::move(buffer); }

void TestHost::SetXDKDefaultViewportAndFixedFunctionMatrices() {
  SetViewportOffset(0.531250f, 0.531250f, 0, 0);
  SetViewportScale(0, -0, 0, 0);

  MATRIX matrix;
  matrix[_11] = 1.0f;
  matrix[_21] = 0.0f;
  matrix[_31] = 0.0f;
  matrix[_41] = 0.0f;

  matrix[_12] = 0.0f;
  matrix[_22] = 1.0f;
  matrix[_32] = 0.0f;
  matrix[_42] = 0.0f;

  matrix[_13] = 0.0f;
  matrix[_23] = 0.0f;
  matrix[_33] = 1.0f;
  matrix[_43] = 7.0f;

  matrix[_14] = 0.0f;
  matrix[_24] = 0.0f;
  matrix[_34] = 0.0f;
  matrix[_44] = 1.0f;
  SetFixedFunctionModelViewMatrix(matrix);

  matrix[_11] = 579.411194f;
  matrix[_21] = 0.0f;
  matrix[_31] = 320.0f;
  matrix[_41] = 2240.0f;

  matrix[_12] = 0.0f;
  matrix[_22] = -579.411194;
  matrix[_32] = 240.0f;
  matrix[_42] = 1680.0f;

  matrix[_13] = 0.0f;
  matrix[_23] = 0.0f;

  if (depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    matrix[_33] = 65864.320312f;
    matrix[_43] = 395185.9375f;
  } else {
    matrix[_33] = 16861522.0f;
    matrix[_43] = 101169136.0f;
  }

  matrix[_14] = 0.0f;
  matrix[_24] = 0.0f;
  matrix[_34] = 1.0f;
  matrix[_44] = 7.0f;
  SetFixedFunctionProjectionMatrix(matrix);

  fixed_function_matrix_mode_ = MATRIX_MODE_DEFAULT_XDK;
}

void TestHost::SetDefaultViewportAndFixedFunctionMatrices() {
  SetViewportOffset(320, 240, 0, 0);
  if (depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    SetViewportScale(320.000000, -240.000000, 65535.000000, 0);
  } else {
    SetViewportScale(320.000000, -240.000000, 16777215.000000, 0);
  }

  MATRIX matrix;
  matrix_unit(matrix);
  SetFixedFunctionModelViewMatrix(matrix);

  matrix[_11] = 640.0f;
  matrix[_21] = 0.0f;
  matrix[_31] = 0.0f;
  matrix[_41] = 640.0f;

  matrix[_12] = 0.0f;
  matrix[_22] = -240.0;
  matrix[_32] = 0.0f;
  matrix[_42] = 240.0f;

  matrix[_13] = 0.0f;
  matrix[_23] = 0.0f;
  if (depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    matrix[_33] = 65535.0f;
  } else {
    matrix[_33] = 16777215.0f;
  }
  matrix[_43] = 0.0f;

  matrix[_14] = 0.0f;
  matrix[_24] = 0.0f;
  matrix[_34] = 0.0f;
  matrix[_44] = 1.0f;
  SetFixedFunctionProjectionMatrix(matrix);

  fixed_function_matrix_mode_ = MATRIX_MODE_DEFAULT_NXDK;
}

void TestHost::SetViewportOffset(float x, float y, float z, float w) const {
  auto p = pb_begin();
  pb_push_to(SUBCH_3D, p++, NV097_SET_VIEWPORT_OFFSET, 4);
  *(p++) = *(uint32_t *)&x;
  *(p++) = *(uint32_t *)&y;
  *(p++) = *(uint32_t *)&z;
  *(p++) = *(uint32_t *)&w;
  pb_end(p);
}

void TestHost::SetViewportScale(float x, float y, float z, float w) const {
  auto p = pb_begin();
  pb_push_to(SUBCH_3D, p++, NV097_SET_VIEWPORT_SCALE, 4);
  *(p++) = *(uint32_t *)&x;
  *(p++) = *(uint32_t *)&y;
  *(p++) = *(uint32_t *)&z;
  *(p++) = *(uint32_t *)&w;
  pb_end(p);
}

void TestHost::SetFixedFunctionModelViewMatrix(const MATRIX model_matrix) {
  memcpy(fixed_function_model_view_matrix_, model_matrix, sizeof(fixed_function_model_view_matrix_));

  auto p = pb_begin();
  p = pb_push_transposed_matrix(p, NV20_TCL_PRIMITIVE_3D_MODELVIEW_MATRIX, fixed_function_model_view_matrix_);
  MATRIX inverse;
  matrix_inverse(inverse, fixed_function_model_view_matrix_);
  p = pb_push_4x3_matrix(p, NV20_TCL_PRIMITIVE_3D_INVERSE_MODELVIEW_MATRIX, inverse);
  pb_end(p);

  fixed_function_matrix_mode_ = MATRIX_MODE_USER;
}

void TestHost::SetFixedFunctionProjectionMatrix(const MATRIX projection_matrix) {
  memcpy(fixed_function_projection_matrix_, projection_matrix, sizeof(fixed_function_projection_matrix_));
  auto p = pb_begin();
  p = pb_push_transposed_matrix(p, NV20_TCL_PRIMITIVE_3D_PROJECTION_MATRIX, fixed_function_projection_matrix_);
  pb_end(p);

  fixed_function_matrix_mode_ = MATRIX_MODE_USER;
}

void TestHost::SetTextureStageEnabled(uint32_t stage, bool enabled) {
  ASSERT(stage == 0 && "Only 1 texture stage is fully implemented.");
  ASSERT(stage < 4 && "Only 4 texture stages are supported.");
  texture_stage_enabled_[stage] = enabled;
}

std::string TestHost::GetPrimitiveName(TestHost::DrawPrimitive primitive) {
  switch (primitive) {
    case PRIMITIVE_POINTS:
      return "Points";

    case PRIMITIVE_LINES:
      return "Lines";

    case PRIMITIVE_LINE_LOOP:
      return "LineLoop";

    case PRIMITIVE_LINE_STRIP:
      return "LineStrip";

    case PRIMITIVE_TRIANGLES:
      return "Triangles";

    case PRIMITIVE_TRIANGLE_STRIP:
      return "TriStrip";

    case PRIMITIVE_TRIANGLE_FAN:
      return "TriFan";

    case PRIMITIVE_QUADS:
      return "Quads";

    case PRIMITIVE_QUAD_STRIP:
      return "QuadStrip";

    case PRIMITIVE_POLYGON:
      return "Polygon";
  }
}

/* Set an attribute pointer */
static void set_attrib_pointer(uint32_t index, uint32_t format, uint32_t size, uint32_t stride, const void *data) {
  uint32_t *p = pb_begin();
  p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + index * 4,
               MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, format) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, size) |
                   MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, stride));
  if (size && data) {
    p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET + index * 4, (uint32_t)data & 0x03ffffff);
  }
  pb_end(p);
}

static void clear_attrib(uint32_t index) {
  // Note: xemu has asserts on the count for several formats, so any format without that ASSERT must be used.
  set_attrib_pointer(index, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 0, 0, nullptr);
}
