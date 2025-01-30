#include "test_host.h"

// clang-format off
#define _USE_MATH_DEFINES
#include <cmath>
// clang-format on

#include <SDL.h>
#include <SDL_image.h>
#include <fpng/src/fpng.h>
#include <strings.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#include <windows.h>
#pragma clang diagnostic pop

#include <xboxkrnl/xboxkrnl.h>

#include <algorithm>
#include <utility>

#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "shaders/vertex_shader_program.h"
#include "vertex_buffer.h"
#include "xbox_math_d3d.h"
#include "xbox_math_matrix.h"
#include "xbox_math_types.h"

using namespace XboxMath;

#define SAVE_Z_AS_PNG

#define MAX_FILE_PATH_SIZE 248
static void SetVertexAttribute(uint32_t index, uint32_t format, uint32_t size, uint32_t stride, const void *data);
static void ClearVertexAttribute(uint32_t index);
static void GetCompositeMatrix(matrix4_t &result, const matrix4_t &model_view, const matrix4_t &projection);

TestHost::TestHost(std::shared_ptr<FTPLogger> ftp_logger, uint32_t framebuffer_width, uint32_t framebuffer_height,
                   uint32_t max_texture_width, uint32_t max_texture_height, uint32_t max_texture_depth)
    : framebuffer_width_(framebuffer_width),
      framebuffer_height_(framebuffer_height),
      max_texture_width_(max_texture_width),
      max_texture_height_(max_texture_height),
      max_texture_depth_(max_texture_depth),
      ftp_logger_{std::move(ftp_logger)} {
  // allocate texture memory buffer large enough for all types
  uint32_t stride = max_texture_width_ * 4;
  max_single_texture_size_ = stride * max_texture_height * max_texture_depth;

  static constexpr uint32_t kMaxPaletteSize = 256 * 4;
  uint32_t palette_size = kMaxPaletteSize * 4;

  static constexpr uint32_t kMaxTextures = 4;
  texture_memory_size_ = max_single_texture_size_ * kMaxTextures;
  uint32_t total_size = texture_memory_size_ + palette_size;

  texture_memory_ = static_cast<uint8_t *>(
      MmAllocateContiguousMemoryEx(total_size, 0, MAXRAM, 0, PAGE_WRITECOMBINE | PAGE_READWRITE));
  ASSERT(texture_memory_ && "Failed to allocate texture memory.");

  texture_palette_memory_ = texture_memory_ + max_single_texture_size_;

  MatrixSetIdentity(fixed_function_model_view_matrix_);
  MatrixSetIdentity(fixed_function_projection_matrix_);
  MatrixSetIdentity(fixed_function_composite_matrix_);
  MatrixSetIdentity(fixed_function_inverse_composite_matrix_);

  uint32_t texture_offset = 0;
  uint32_t palette_offset = 0;
  for (auto i = 0; i < 4; ++i, texture_offset += max_single_texture_size_, palette_offset += kMaxPaletteSize) {
    texture_stage_[i].SetStage(i);
    texture_stage_[i].SetTextureDimensions(max_texture_width, max_texture_height);
    texture_stage_[i].SetImageDimensions(max_texture_width, max_texture_height);
    texture_stage_[i].SetTextureOffset(texture_offset);
    texture_stage_[i].SetPaletteOffset(palette_offset);
  }

  SetSurfaceFormat(SCF_A8R8G8B8, SZF_Z24S8, framebuffer_width_, framebuffer_height_, surface_swizzle_);
}

TestHost::~TestHost() {
  vertex_buffer_.reset();
  if (texture_memory_) {
    MmFreeContiguousMemory(texture_memory_);
  }
  // texture_palette_memory_ is an offset into texture_memory_ and is intentionally not freed.
  texture_palette_memory_ = nullptr;
}

void TestHost::ClearDepthStencilRegion(uint32_t depth_value, uint8_t stencil_value, uint32_t left, uint32_t top,
                                       uint32_t width, uint32_t height) const {
  if (!width || width > framebuffer_width_) {
    width = framebuffer_width_;
  }
  if (!height || height > framebuffer_height_) {
    height = framebuffer_height_;
  }

  set_depth_stencil_buffer_region(depth_buffer_format_, depth_value, stencil_value, left, top, width, height);
}

void TestHost::ClearColorRegion(uint32_t argb, uint32_t left, uint32_t top, uint32_t width, uint32_t height) const {
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
  ClearColorRegion(argb);
  ClearDepthStencilRegion(depth_value, stencil_value);
  EraseText();
}

void TestHost::SetSurfaceFormat(SurfaceColorFormat color_format, SurfaceZetaFormat depth_format, uint32_t width,
                                uint32_t height, bool swizzle, uint32_t clip_x, uint32_t clip_y, uint32_t clip_width,
                                uint32_t clip_height, AntiAliasingSetting aa) {
  surface_color_format_ = color_format;
  depth_buffer_format_ = depth_format;
  surface_swizzle_ = swizzle;
  surface_width_ = width;
  surface_height_ = height;
  surface_clip_x_ = clip_x;
  surface_clip_y_ = clip_y;
  surface_clip_width_ = clip_width;
  surface_clip_height_ = clip_height;
  antialiasing_setting_ = aa;

  HandleDepthBufferFormatChange();
}

void TestHost::SetSurfaceFormatImmediate(SurfaceColorFormat color_format, SurfaceZetaFormat depth_format,
                                         uint32_t width, uint32_t height, bool swizzle, uint32_t clip_x,
                                         uint32_t clip_y, uint32_t clip_width, uint32_t clip_height,
                                         AntiAliasingSetting aa) {
  SetSurfaceFormat(color_format, depth_format, width, height, swizzle, clip_x, clip_y, clip_width, clip_height, aa);
  CommitSurfaceFormat();
}

void TestHost::CommitSurfaceFormat() const {
  uint32_t value = SET_MASK(NV097_SET_SURFACE_FORMAT_COLOR, surface_color_format_) |
                   SET_MASK(NV097_SET_SURFACE_FORMAT_ZETA, depth_buffer_format_) |
                   SET_MASK(NV097_SET_SURFACE_FORMAT_ANTI_ALIASING, antialiasing_setting_) |
                   SET_MASK(NV097_SET_SURFACE_FORMAT_TYPE, surface_swizzle_ ? NV097_SET_SURFACE_FORMAT_TYPE_SWIZZLE
                                                                            : NV097_SET_SURFACE_FORMAT_TYPE_PITCH);
  if (surface_swizzle_) {
    uint32_t log_width = 31 - __builtin_clz(surface_width_);
    value |= SET_MASK(NV097_SET_SURFACE_FORMAT_WIDTH, log_width);
    uint32_t log_height = 31 - __builtin_clz(surface_height_);
    value |= SET_MASK(NV097_SET_SURFACE_FORMAT_HEIGHT, log_height);
  }

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SURFACE_FORMAT, value);
  if (!surface_swizzle_) {
    uint32_t width = surface_clip_width_ ? surface_clip_width_ : surface_width_;
    uint32_t height = surface_clip_height_ ? surface_clip_height_ : surface_height_;
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_HORIZONTAL, (width << 16) + surface_clip_x_);
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_VERTICAL, (height << 16) + surface_clip_y_);
  }
  pb_end(p);

  float max_depth = MaxDepthBufferValue(depth_buffer_format_, depth_buffer_mode_float_);
  SetDepthClip(0.0f, max_depth);
}

void TestHost::SetDepthClip(float min, float max) const {
  auto p = pb_begin();
  p = pb_push1f(p, NV097_SET_CLIP_MIN, min);
  p = pb_push1f(p, NV097_SET_CLIP_MAX, max);
  pb_end(p);
}

float TestHost::MaxDepthBufferValue(uint32_t depth_buffer_format, bool float_mode) {
  float max_depth;
  if (depth_buffer_format == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    if (float_mode) {
      *(uint32_t *)&max_depth = 0x43FFF800;  // z16_max as 32-bit float.
    } else {
      max_depth = static_cast<float>(0xFFFF);
    }
  } else {
    if (float_mode) {
      // max_depth = 0x7F7FFF80;  // z24_max as 32-bit float.
      *(uint32_t *)&max_depth =
          0x7149F2CA;  // Observed value, 1e+30 (also used for directional lighting as "infinity").
    } else {
      max_depth = static_cast<float>(0x00FFFFFF);
    }
  }

  return max_depth;
}

void TestHost::PrepareDraw(uint32_t argb, uint32_t depth_value, uint8_t stencil_value) {
  pb_wait_for_vbl();
  pb_reset();

  SetupTextureStages();
  CommitSurfaceFormat();

  // Override the values set in pb_init. Unfortunately the default is not exposed and must be recreated here.

  Clear(argb, depth_value, stencil_value);

  if (vertex_shader_program_) {
    vertex_shader_program_->PrepareDraw();
  }

  while (pb_busy()) {
    /* Wait for completion... */
  }
}

void TestHost::SetVertexBufferAttributes(uint32_t enabled_fields) {
  ASSERT(vertex_buffer_ && "Vertex buffer must be set before calling SetVertexBufferAttributes.");
  if (!vertex_buffer_->IsCacheValid()) {
    auto p = pb_begin();
    p = pb_push1(p, NV097_BREAK_VERTEX_BUFFER_CACHE, 0);
    pb_end(p);
    vertex_buffer_->SetCacheValid();
  }

  // FIXME: Linearize on a per-stage basis instead of basing entirely on stage 0.
  // E.g., if texture unit 0 uses linear and 1 uses swizzle, TEX0 should be linearized, TEX1 should be normalized.
  bool is_linear = texture_stage_[0].enabled_ && texture_stage_[0].IsLinear();
  Vertex *vptr = is_linear ? vertex_buffer_->linear_vertex_buffer_ : vertex_buffer_->normalized_vertex_buffer_;

  auto set = [this, enabled_fields](VertexAttribute attribute, uint32_t attribute_index, uint32_t format, uint32_t size,
                                    const void *data) {
    if (enabled_fields & attribute) {
      uint32_t stride = sizeof(Vertex);
      if (vertex_attribute_stride_override_[attribute_index] != kNoStrideOverride) {
        stride = vertex_attribute_stride_override_[attribute_index];
      }
      SetVertexAttribute(attribute_index, format, size, stride, data);
    } else {
      ClearVertexAttribute(attribute_index);
    }
  };

  set(POSITION, NV2A_VERTEX_ATTR_POSITION, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, vertex_buffer_->position_count_,
      &vptr[0].pos);
  set(WEIGHT, NV2A_VERTEX_ATTR_WEIGHT, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, &vptr[0].weight);
  set(NORMAL, NV2A_VERTEX_ATTR_NORMAL, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 3, &vptr[0].normal);
  set(DIFFUSE, NV2A_VERTEX_ATTR_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, &vptr[0].diffuse);
  set(SPECULAR, NV2A_VERTEX_ATTR_SPECULAR, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, &vptr[0].specular);
  set(FOG_COORD, NV2A_VERTEX_ATTR_FOG_COORD, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 1, &vptr[0].fog_coord);
  set(POINT_SIZE, NV2A_VERTEX_ATTR_POINT_SIZE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 1, &vptr[0].point_size);

  //  set(BACK_DIFFUSE, NV2A_VERTEX_ATTR_BACK_DIFFUSE, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4,
  //  &vptr[0].back_diffuse);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_BACK_DIFFUSE);

  //  set(BACK_SPECULAR, NV2A_VERTEX_ATTR_BACK_SPECULAR, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4,
  //  &vptr[0].back_specular);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_BACK_SPECULAR);

  set(TEXCOORD0, NV2A_VERTEX_ATTR_TEXTURE0, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
      vertex_buffer_->tex0_coord_count_, &vptr[0].texcoord0);
  set(TEXCOORD1, NV2A_VERTEX_ATTR_TEXTURE1, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
      vertex_buffer_->tex1_coord_count_, &vptr[0].texcoord1);
  set(TEXCOORD2, NV2A_VERTEX_ATTR_TEXTURE2, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
      vertex_buffer_->tex2_coord_count_, &vptr[0].texcoord2);
  set(TEXCOORD3, NV2A_VERTEX_ATTR_TEXTURE3, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
      vertex_buffer_->tex3_coord_count_, &vptr[0].texcoord3);

  //  set(V13, NV2A_VERTEX_ATTR_13, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, &vptr[0].v13);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_13);

  //  set(V14, NV2A_VERTEX_ATTR_14, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, &vptr[0].v14);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_14);

  //  set(V15, NV2A_VERTEX_ATTR_15, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 4, &vptr[0].v15);
  ClearVertexAttribute(NV2A_VERTEX_ATTR_15);
}

void TestHost::DrawArrays(uint32_t enabled_vertex_fields, DrawPrimitive primitive) {
  if (vertex_shader_program_) {
    vertex_shader_program_->PrepareDraw();
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

void TestHost::Begin(DrawPrimitive primitive) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, primitive);
  pb_end(p);
}

void TestHost::End() const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
  pb_end(p);
}

void TestHost::DrawInlineBuffer(uint32_t enabled_vertex_fields, DrawPrimitive primitive) {
  if (vertex_shader_program_) {
    vertex_shader_program_->PrepareDraw();
  }

  ASSERT(vertex_buffer_ && "Vertex buffer must be set before calling DrawInlineBuffer.");
  SetVertexBufferAttributes(enabled_vertex_fields);

  Begin(primitive);

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
      SetTexCoord1(vertex->texcoord1[0], vertex->texcoord1[1]);
    }
    if (enabled_vertex_fields & TEXCOORD2) {
      SetTexCoord2(vertex->texcoord2[0], vertex->texcoord2[1]);
    }
    if (enabled_vertex_fields & TEXCOORD3) {
      SetTexCoord3(vertex->texcoord3[0], vertex->texcoord3[1]);
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

  End();
}

void TestHost::DrawInlineArray(uint32_t enabled_vertex_fields, DrawPrimitive primitive) {
  if (vertex_shader_program_) {
    vertex_shader_program_->PrepareDraw();
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
  if (vertex_shader_program_) {
    vertex_shader_program_->PrepareDraw();
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
  if (vertex_shader_program_) {
    vertex_shader_program_->PrepareDraw();
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
  p = pb_push3f(p, NV097_SET_VERTEX3F, x, y, z);
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

void TestHost::SetNormal3S(int x, int y, int z) const {
  auto p = pb_begin();
  uint32_t xy = (x & 0xFFFF) | y << 16;
  uint32_t z0 = z & 0xFFFF;
  p = pb_push2(p, NV097_SET_NORMAL3S, xy, z0);
  pb_end(p);
}

void TestHost::SetDiffuse(float r, float g, float b, float a) const {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_DIFFUSE_COLOR4F, r, g, b, a);
  pb_end(p);
}

void TestHost::SetDiffuse(float r, float g, float b) const {
  auto p = pb_begin();
  p = pb_push3f(p, NV097_SET_DIFFUSE_COLOR3F, r, g, b);
  pb_end(p);
}

void TestHost::SetDiffuse(uint32_t color) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DIFFUSE_COLOR4I, color);
  pb_end(p);
}

void TestHost::SetSpecular(float r, float g, float b, float a) const {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_SPECULAR_COLOR4F, r, g, b, a);
  pb_end(p);
}

void TestHost::SetSpecular(float r, float g, float b) const {
  auto p = pb_begin();
  p = pb_push3f(p, NV097_SET_SPECULAR_COLOR3F, r, g, b);
  pb_end(p);
}

void TestHost::SetSpecular(uint32_t color) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SPECULAR_COLOR4I, color);
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
  p = pb_push2(p, NV097_SET_TEXCOORD0_2F, *(uint32_t *)&u, *(uint32_t *)&v);
  pb_end(p);
}

void TestHost::SetTexCoord0S(int u, int v) const {
  auto p = pb_begin();
  uint32_t uv = (u & 0xFFFF) | (v << 16);
  p = pb_push1(p, NV097_SET_TEXCOORD0_2S, uv);
  pb_end(p);
}

void TestHost::SetTexCoord0(float s, float t, float p, float q) const {
  auto pb = pb_begin();
  pb = pb_push4f(pb, NV097_SET_TEXCOORD0_4F, s, t, p, q);
  pb_end(pb);
}

void TestHost::SetTexCoord0S(int s, int t, int p, int q) const {
  auto pb = pb_begin();
  uint32_t st = (s & 0xFFFF) | (t << 16);
  uint32_t pq = (p & 0xFFFF) | (q << 16);
  pb = pb_push2(pb, NV097_SET_TEXCOORD0_4S, st, pq);
  pb_end(pb);
}

void TestHost::SetTexCoord1(float u, float v) const {
  auto p = pb_begin();
  p = pb_push2(p, NV097_SET_TEXCOORD1_2F, *(uint32_t *)&u, *(uint32_t *)&v);
  pb_end(p);
}

void TestHost::SetTexCoord1S(int u, int v) const {
  auto p = pb_begin();
  uint32_t uv = (u & 0xFFFF) | (v << 16);
  p = pb_push1(p, NV097_SET_TEXCOORD1_2S, uv);
  pb_end(p);
}

void TestHost::SetTexCoord1(float s, float t, float p, float q) const {
  auto pb = pb_begin();
  pb = pb_push4f(pb, NV097_SET_TEXCOORD1_4F, s, t, p, q);
  pb_end(pb);
}

void TestHost::SetTexCoord1S(int s, int t, int p, int q) const {
  auto pb = pb_begin();
  uint32_t st = (s & 0xFFFF) | (t << 16);
  uint32_t pq = (p & 0xFFFF) | (q << 16);
  pb = pb_push2(pb, NV097_SET_TEXCOORD1_4S, st, pq);
  pb_end(pb);
}

void TestHost::SetTexCoord2(float u, float v) const {
  auto p = pb_begin();
  p = pb_push2f(p, NV097_SET_TEXCOORD2_2F, u, v);
  pb_end(p);
}

void TestHost::SetTexCoord2S(int u, int v) const {
  auto p = pb_begin();
  uint32_t uv = (u & 0xFFFF) | (v << 16);
  p = pb_push1(p, NV097_SET_TEXCOORD2_2S, uv);
  pb_end(p);
}

void TestHost::SetTexCoord2(float s, float t, float p, float q) const {
  auto pb = pb_begin();
  pb = pb_push4f(pb, NV097_SET_TEXCOORD2_4F, s, t, p, q);
  pb_end(pb);
}

void TestHost::SetTexCoord2S(int s, int t, int p, int q) const {
  auto pb = pb_begin();
  uint32_t st = (s & 0xFFFF) | (t << 16);
  uint32_t pq = (p & 0xFFFF) | (q << 16);
  pb = pb_push2(pb, NV097_SET_TEXCOORD2_4S, st, pq);
  pb_end(pb);
}

void TestHost::SetTexCoord3(float u, float v) const {
  auto p = pb_begin();
  p = pb_push2(p, NV097_SET_TEXCOORD3_2F, *(uint32_t *)&u, *(uint32_t *)&v);
  pb_end(p);
}

void TestHost::SetTexCoord3S(int u, int v) const {
  auto p = pb_begin();
  uint32_t uv = (u & 0xFFFF) | (v << 16);
  p = pb_push1(p, NV097_SET_TEXCOORD3_2S, uv);
  pb_end(p);
}

void TestHost::SetTexCoord3(float s, float t, float p, float q) const {
  auto pb = pb_begin();
  pb = pb_push4f(pb, NV097_SET_TEXCOORD3_4F, s, t, p, q);
  pb_end(pb);
}

void TestHost::SetTexCoord3S(int s, int t, int p, int q) const {
  auto pb = pb_begin();
  uint32_t st = (s & 0xFFFF) | (t << 16);
  uint32_t pq = (p & 0xFFFF) | (q << 16);
  pb = pb_push2(pb, NV097_SET_TEXCOORD3_4S, st, pq);
  pb_end(pb);
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
std::string TestHost::PrepareSaveFile(std::string output_directory, const std::string &filename,
                                      const std::string &extension) {
  EnsureFolderExists(output_directory);

  output_directory += "\\";
  output_directory += filename;
  output_directory += extension;

  if (output_directory.length() > MAX_FILE_PATH_SIZE) {
    ASSERT(!"Full save file path is too long.");
  }

  return output_directory;
}

std::string TestHost::SaveBackBuffer(const std::string &output_directory, const std::string &name) {
  auto target_file = PrepareSaveFile(output_directory, name);

  auto buffer = pb_agp_access(pb_back_buffer());
  auto width = static_cast<int>(pb_back_buffer_width());
  auto height = static_cast<int>(pb_back_buffer_height());
  auto pitch = static_cast<int>(pb_back_buffer_pitch());

  // FIXME: Support 16bpp surfaces
  ASSERT((pitch == width * 4) && "Expected packed 32bpp surface");

  // Swizzle color channels ARGB -> ABGR
  unsigned int num_pixels = width * height;
  uint32_t *pre_enc_buf = (uint32_t *)malloc(num_pixels * 4);
  ASSERT(pre_enc_buf && "Failed to allocate pre-encode buffer");
  for (unsigned int i = 0; i < num_pixels; i++) {
    uint32_t c = static_cast<uint32_t *>(buffer)[i];
    pre_enc_buf[i] = (c & 0xff00ff00) | ((c >> 16) & 0xff) | ((c & 0xff) << 16);
  }

  std::vector<uint8_t> out_buf;
  if (!fpng::fpng_encode_image_to_memory((void *)pre_enc_buf, width, height, 4, out_buf)) {
    ASSERT(!"Failed to encode PNG image");
  }
  free(pre_enc_buf);

  FILE *pFile = fopen(target_file.c_str(), "wb");
  ASSERT(pFile && "Failed to open output PNG image");
  if (fwrite(out_buf.data(), 1, out_buf.size(), pFile) != out_buf.size()) {
    ASSERT(!"Failed to write output PNG image");
  }
  if (fclose(pFile)) {
    ASSERT(!"Failed to close output PNG image");
  }

  return target_file;
}

std::string TestHost::SaveZBuffer(const std::string &output_directory, const std::string &name) const {
  uint32_t depth = depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? 16 : 32;
#ifdef SAVE_Z_AS_PNG
  auto format =
      depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? SDL_PIXELFORMAT_RGB565 : SDL_PIXELFORMAT_ARGB8888;
  return SaveTexture(output_directory, name, pb_depth_stencil_buffer(), framebuffer_width_, framebuffer_height_,
                     pb_depth_stencil_pitch(), depth, format);
#else
  return SaveRawTexture(output_directory, name, pb_depth_stencil_buffer(), framebuffer_width_, framebuffer_height_,
                        framebuffer_width_ * 4, depth);
#endif
}

std::string TestHost::SaveTexture(const std::string &output_directory, const std::string &name, const uint8_t *texture,
                                  uint32_t width, uint32_t height, uint32_t pitch, uint32_t bits_per_pixel,
                                  SDL_PixelFormatEnum format) {
  auto target_file = PrepareSaveFile(output_directory, name);

  auto buffer = pb_agp_access(const_cast<void *>(static_cast<const void *>(texture)));
  auto size = pitch * height;

  PrintMsg("Saving to %s. Size: %lu. Pitch %lu.\n", target_file.c_str(), size, pitch);

  SDL_Surface *surface =
      SDL_CreateRGBSurfaceWithFormatFrom((void *)buffer, static_cast<int>(width), static_cast<int>(height),
                                         static_cast<int>(bits_per_pixel), static_cast<int>(pitch), format);

  if (IMG_SavePNG(surface, target_file.c_str())) {
    PrintMsg("Failed to save PNG file '%s'\n", target_file.c_str());
    ASSERT(!"Failed to save PNG file.");
  }

  SDL_FreeSurface(surface);

  return target_file;
}

std::string TestHost::SaveRawTexture(const std::string &output_directory, const std::string &name,
                                     const uint8_t *texture, uint32_t width, uint32_t height, uint32_t pitch,
                                     uint32_t bits_per_pixel) {
  auto target_file = PrepareSaveFile(output_directory, name, ".raw");

  auto buffer = static_cast<uint8_t *>(pb_agp_access(const_cast<void *>(static_cast<const void *>(texture))));
  const uint32_t bytes_per_pixel = (bits_per_pixel >> 3);
  const uint32_t populated_pitch = width * bytes_per_pixel;
  const auto size = populated_pitch * height;

  PrintMsg("Saving to %s. Size: %lu. Pitch %lu.\n", target_file.c_str(), size, pitch);

  FILE *f = fopen(target_file.c_str(), "wb");
  ASSERT(f && "Failed to open raw texture output file.");

  for (uint32_t y = 0; y < height; ++y) {
    auto written = fwrite(buffer, populated_pitch, 1, f);
    ASSERT(written == 1 && "Failed to write row to raw texture output file.");
    buffer += pitch;
  }

  fclose(f);

  return target_file;
}

void TestHost::SetupControl0(bool enable_stencil_write, bool w_buffered) const {
  // FIXME: Figure out what to do in cases where there are multiple stages with different conversion needs.
  // Is this supported by hardware?
  bool requires_colorspace_conversion = texture_stage_[0].RequiresColorspaceConversion();

  uint32_t control0 = enable_stencil_write ? NV097_SET_CONTROL0_STENCIL_WRITE_ENABLE : 0;
  control0 |= MASK(NV097_SET_CONTROL0_Z_FORMAT,
                   depth_buffer_mode_float_ ? NV097_SET_CONTROL0_Z_FORMAT_FLOAT : NV097_SET_CONTROL0_Z_FORMAT_FIXED);
  control0 |= MASK(NV097_SET_CONTROL0_Z_PERSPECTIVE_ENABLE, w_buffered ? 1 : 0);

  if (requires_colorspace_conversion) {
    control0 |= MASK(NV097_SET_CONTROL0_COLOR_SPACE_CONVERT, NV097_SET_CONTROL0_COLOR_SPACE_CONVERT_CRYCB_TO_RGB);
  }
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTROL0, control0);
  pb_end(p);
}

void TestHost::SetupTextureStages() const {
  // TODO: Support texture memory that is not allocated from the base of the DMA target registered by pbkit.
  auto texture_dma_offset = reinterpret_cast<uint32_t>(texture_memory_);
  auto palette_dma_offset = reinterpret_cast<uint32_t>(texture_palette_memory_);
  for (auto &stage : texture_stage_) {
    stage.Commit(texture_dma_offset, palette_dma_offset);
  }
}

void TestHost::SetTextureFormat(const TextureFormatInfo &fmt, uint32_t stage) { texture_stage_[stage].SetFormat(fmt); }

void TestHost::SetDefaultTextureParams(uint32_t stage) {
  texture_stage_[stage].Reset();
  texture_stage_[stage].SetTextureDimensions(max_texture_width_, max_texture_height_);
  texture_stage_[stage].SetImageDimensions(max_texture_width_, max_texture_height_);
}

void TestHost::HandleDepthBufferFormatChange() {
  // Note: This method intentionally recalculates matrices even if the format has not changed as it is called by
  // SetDepthBufferFloatMode when that mode changes.
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

void TestHost::SetDepthBufferFloatMode(bool enabled) {
  if (enabled == depth_buffer_mode_float_) {
    return;
  }
  depth_buffer_mode_float_ = enabled;
  HandleDepthBufferFormatChange();
}

int TestHost::SetTexture(SDL_Surface *surface, uint32_t stage) {
  return texture_stage_[stage].SetTexture(surface, texture_memory_);
}

int TestHost::SetVolumetricTexture(const SDL_Surface **surface, uint32_t depth, uint32_t stage) {
  return texture_stage_[stage].SetVolumetricTexture(surface, depth, texture_memory_);
}

int TestHost::SetRawTexture(const uint8_t *source, uint32_t width, uint32_t height, uint32_t depth, uint32_t pitch,
                            uint32_t bytes_per_pixel, bool swizzle, uint32_t stage) {
  const uint32_t max_stride = max_texture_width_ * 4;
  const uint32_t max_texture_size = max_stride * max_texture_height_ * max_texture_depth_;

  const uint32_t layer_size = pitch * height;
  const uint32_t surface_size = layer_size * depth;
  ASSERT(surface_size < max_texture_size && "Texture too large.");

  return texture_stage_[stage].SetRawTexture(source, width, height, depth, pitch, bytes_per_pixel, swizzle,
                                             texture_memory_);
}

int TestHost::SetPalette(const uint32_t *palette, PaletteSize size, uint32_t stage) {
  return texture_stage_[stage].SetPalette(palette, size, texture_palette_memory_);
}

void TestHost::SetPaletteSize(PaletteSize size, uint32_t stage) { texture_stage_[stage].SetPaletteSize(size); }

void TestHost::FinishDraw(bool allow_saving, const std::string &output_directory, const std::string &suite_name,
                          const std::string &name, const std::string &z_buffer_name) {
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

    auto output_path = SaveBackBuffer(output_directory, name);

    if (ftp_logger_) {
      auto remote_filename = "'" + suite_name + "::" + output_path.substr(output_directory.length() + 1) + "'";
      if (!ftp_logger_->Connect() || !ftp_logger_->PutFile(output_path, remote_filename)) {
        PrintMsg("Failed to store backbuffer artifact %s to FTP server!\n", output_path.c_str());
      }
    }

    if (!z_buffer_name.empty()) {
      auto z_buffer_output_path = SaveZBuffer(output_directory, z_buffer_name);

      if (ftp_logger_) {
        auto remote_filename = suite_name + "::" + z_buffer_output_path.substr(output_directory.length() + 1);
        if (!ftp_logger_->Connect() || !ftp_logger_->PutFile(z_buffer_output_path, remote_filename)) {
          PrintMsg("Failed to store z-buffer artifact %s to FTP server!\n", z_buffer_output_path.c_str());
        }
      }
    }
  }

  /* Swap buffers (if we can) */
  while (pb_finished()) {
    /* Not ready to swap yet */
  }
}

void TestHost::SetVertexShaderProgram(std::shared_ptr<VertexShaderProgram> program) {
  vertex_shader_program_ = std::move(program);

  if (vertex_shader_program_) {
    vertex_shader_program_->Activate();
  } else {
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
  SetWindowClip(framebuffer_width_, framebuffer_height_);
  SetViewportOffset(0.531250f, 0.531250f, 0, 0);
  SetViewportScale(0, -0, 0, 0);

  matrix4_t matrix;
  BuildDefaultXDKModelViewMatrix(matrix);
  SetFixedFunctionModelViewMatrix(matrix);

  BuildDefaultXDKProjectionMatrix(matrix);
  SetFixedFunctionProjectionMatrix(matrix);

  fixed_function_matrix_mode_ = MATRIX_MODE_DEFAULT_XDK;
}

void TestHost::SetDefaultViewportAndFixedFunctionMatrices() {
  SetWindowClip(framebuffer_width_, framebuffer_height_);
  SetViewportOffset(320, 240, 0, 0);
  if (depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    SetViewportScale(320.000000, -240.000000, 65535.000000, 0);
  } else {
    SetViewportScale(320.000000, -240.000000, 16777215.000000, 0);
  }

  matrix4_t matrix;
  MatrixSetIdentity(matrix);
  SetFixedFunctionModelViewMatrix(matrix);

  matrix[0][0] = 640.0f;
  matrix[1][0] = 0.0f;
  matrix[2][0] = 0.0f;
  matrix[3][0] = 640.0f;

  matrix[0][1] = 0.0f;
  matrix[1][1] = -240.0;
  matrix[2][1] = 0.0f;
  matrix[3][1] = 240.0f;

  matrix[0][2] = 0.0f;
  matrix[1][2] = 0.0f;
  if (depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    matrix[2][2] = 65535.0f;
  } else {
    matrix[2][2] = 16777215.0f;
  }
  matrix[3][2] = 0.0f;

  matrix[0][3] = 0.0f;
  matrix[1][3] = 0.0f;
  matrix[2][3] = 0.0f;
  matrix[3][3] = 1.0f;
  SetFixedFunctionProjectionMatrix(matrix);

  fixed_function_matrix_mode_ = MATRIX_MODE_DEFAULT_NXDK;
}

void TestHost::BuildDefaultXDKModelViewMatrix(matrix4_t &matrix) {
  vector_t eye{0.0f, 0.0f, -7.0f, 1.0f};
  vector_t at{0.0f, 0.0f, 0.0f, 1.0f};
  vector_t up{0.0f, 1.0f, 0.0f, 1.0f};
  BuildD3DModelViewMatrix(matrix, eye, at, up);
}

void TestHost::BuildD3DModelViewMatrix(matrix4_t &matrix, const vector_t &eye, const vector_t &at, const vector_t &up) {
  CreateD3DLookAtLH(matrix, eye, at, up);
}

void TestHost::BuildD3DProjectionViewportMatrix(matrix4_t &result, float fov, float z_near, float z_far) const {
  matrix4_t viewport;
  if (depth_buffer_format_ == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    if (depth_buffer_mode_float_) {
      CreateD3DStandardViewport16BitFloat(viewport, GetFramebufferWidthF(), GetFramebufferHeightF());
    } else {
      CreateD3DStandardViewport16Bit(viewport, GetFramebufferWidthF(), GetFramebufferHeightF());
    }
  } else {
    if (depth_buffer_mode_float_) {
      CreateD3DStandardViewport24BitFloat(viewport, GetFramebufferWidthF(), GetFramebufferHeightF());
    } else {
      CreateD3DStandardViewport24Bit(viewport, GetFramebufferWidthF(), GetFramebufferHeightF());
    }
  }

  matrix4_t projection;
  CreateD3DPerspectiveFOVLH(projection, fov, GetFramebufferWidthF() / GetFramebufferHeightF(), z_near, z_far);

  MatrixMultMatrix(projection, viewport, result);
}

void TestHost::BuildD3DOrthographicProjectionMatrix(XboxMath::matrix4_t &result, float left, float right, float top,
                                                    float bottom, float z_near, float z_far) {
  CreateD3DOrthographicLH(result, left, right, top, bottom, z_near, z_far);
}

void TestHost::BuildDefaultXDKProjectionMatrix(matrix4_t &matrix) const {
  BuildD3DProjectionViewportMatrix(matrix, M_PI * 0.25f, 1.0f, 200.0f);
}

void TestHost::ProjectPoint(vector_t &result, const vector_t &world_point) const {
  vector_t screen_point;
  VectorMultMatrix(world_point, fixed_function_composite_matrix_, screen_point);

  result[0] = screen_point[0] / screen_point[3];
  result[1] = screen_point[1] / screen_point[3];
  result[2] = screen_point[2] / screen_point[3];
  result[3] = 1.0f;
}

void TestHost::UnprojectPoint(vector_t &result, const vector_t &screen_point) const {
  VectorMultMatrix(screen_point, fixed_function_inverse_composite_matrix_, result);
}

void TestHost::UnprojectPoint(vector_t &result, const vector_t &screen_point, float world_z) const {
  vector_t work;
  VectorCopyVector(work, screen_point);

  // TODO: Get the near and far plane mappings from the viewport matrix.
  work[2] = 0.0f;
  vector_t near_plane;
  VectorMultMatrix(work, fixed_function_inverse_composite_matrix_, near_plane);
  VectorEuclidean(near_plane);

  work[2] = 64000.0f;
  vector_t far_plane;
  VectorMultMatrix(work, fixed_function_inverse_composite_matrix_, far_plane);
  VectorEuclidean(far_plane);

  float t = (world_z - near_plane[2]) / (far_plane[2] - near_plane[2]);
  result[0] = near_plane[0] + (far_plane[0] - near_plane[0]) * t;
  result[1] = near_plane[1] + (far_plane[1] - near_plane[1]) * t;
  result[2] = world_z;
  result[3] = 1.0f;
}

void TestHost::SetWindowClipExclusive(bool exclusive) {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_WINDOW_CLIP_TYPE, exclusive);
  pb_end(p);
}

void TestHost::SetWindowClip(uint32_t right, uint32_t bottom, uint32_t left, uint32_t top, uint32_t region) {
  auto p = pb_begin();
  const uint32_t offset = region * 4;
  p = pb_push1(p, NV097_SET_WINDOW_CLIP_HORIZONTAL + offset, left + (right << 16));
  p = pb_push1(p, NV097_SET_WINDOW_CLIP_VERTICAL + offset, top + (bottom << 16));
  pb_end(p);
}

void TestHost::SetViewportOffset(float x, float y, float z, float w) {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_VIEWPORT_OFFSET, x, y, z, w);
  pb_end(p);
}

void TestHost::SetViewportScale(float x, float y, float z, float w) {
  auto p = pb_begin();
  p = pb_push4f(p, NV097_SET_VIEWPORT_SCALE, x, y, z, w);
  pb_end(p);
}

void TestHost::SetFixedFunctionModelViewMatrix(const matrix4_t model_matrix) {
  memcpy(fixed_function_model_view_matrix_, model_matrix, sizeof(fixed_function_model_view_matrix_));

  auto p = pb_begin();
  p = pb_push_transposed_matrix(p, NV097_SET_MODEL_VIEW_MATRIX, fixed_function_model_view_matrix_[0]);
  matrix4_t inverse;
  MatrixInvert(fixed_function_model_view_matrix_, inverse);
  p = pb_push_4x3_matrix(p, NV097_SET_INVERSE_MODEL_VIEW_MATRIX, inverse[0]);
  pb_end(p);

  fixed_function_matrix_mode_ = MATRIX_MODE_USER;

  // Update the composite matrix.
  SetFixedFunctionProjectionMatrix(fixed_function_projection_matrix_);
}

void TestHost::SetFixedFunctionProjectionMatrix(const matrix4_t projection_matrix) {
  memcpy(fixed_function_projection_matrix_, projection_matrix, sizeof(fixed_function_projection_matrix_));

  GetCompositeMatrix(fixed_function_composite_matrix_, fixed_function_model_view_matrix_,
                     fixed_function_projection_matrix_);

  auto p = pb_begin();
  p = pb_push_transposed_matrix(p, NV097_SET_COMPOSITE_MATRIX, fixed_function_composite_matrix_[0]);
  pb_end(p);

  MatrixTranspose(fixed_function_composite_matrix_);
  MatrixInvert(fixed_function_composite_matrix_, fixed_function_inverse_composite_matrix_);

  fixed_function_matrix_mode_ = MATRIX_MODE_USER;

  // https://developer.download.nvidia.com/assets/gamedev/docs/W_buffering2.pdf
  auto &proj = fixed_function_projection_matrix_;
  w_near_ = proj[3][3] - (proj[3][2] / proj[2][2] * proj[2][3]);
  w_far_ = (proj[3][3] - proj[3][2]) / (proj[2][2] - proj[2][3]) * proj[2][3] + proj[3][3];
}

void TestHost::SetTextureStageEnabled(uint32_t stage, bool enabled) {
  ASSERT(stage < 4 && "Only 4 texture stages are supported.");
  texture_stage_[stage].SetEnabled(enabled);
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

void TestHost::SetColorMask(uint32_t mask) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COLOR_MASK, mask);
  pb_end(p);
}

void TestHost::SetBlend(bool enable, uint32_t func, uint32_t sfactor, uint32_t dfactor) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_BLEND_ENABLE, enable);
  if (enable) {
    p = pb_push1(p, NV097_SET_BLEND_EQUATION, func);
    p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, sfactor);
    p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, dfactor);
  }
  pb_end(p);
}

void TestHost::SetCombinerControl(int num_combiners, bool same_factor0, bool same_factor1, bool mux_msb) const {
  ASSERT(num_combiners > 0 && num_combiners < 8);
  uint32_t setting = MASK(NV097_SET_COMBINER_CONTROL_ITERATION_COUNT, num_combiners);
  if (!same_factor0) {
    setting |= MASK(NV097_SET_COMBINER_CONTROL_FACTOR0, NV097_SET_COMBINER_CONTROL_FACTOR0_EACH_STAGE);
  }
  if (!same_factor1) {
    setting |= MASK(NV097_SET_COMBINER_CONTROL_FACTOR1, NV097_SET_COMBINER_CONTROL_FACTOR1_EACH_STAGE);
  }
  if (mux_msb) {
    setting |= MASK(NV097_SET_COMBINER_CONTROL_MUX_SELECT, NV097_SET_COMBINER_CONTROL_MUX_SELECT_MSB);
  }

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_CONTROL, setting);
  pb_end(p);
}

void TestHost::SetInputColorCombiner(int combiner, CombinerSource a_source, bool a_alpha, CombinerMapping a_mapping,
                                     CombinerSource b_source, bool b_alpha, CombinerMapping b_mapping,
                                     CombinerSource c_source, bool c_alpha, CombinerMapping c_mapping,
                                     CombinerSource d_source, bool d_alpha, CombinerMapping d_mapping) const {
  uint32_t value = MakeInputCombiner(a_source, a_alpha, a_mapping, b_source, b_alpha, b_mapping, c_source, c_alpha,
                                     c_mapping, d_source, d_alpha, d_mapping);
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_COLOR_ICW + combiner * 4, value);
  pb_end(p);
}

void TestHost::ClearInputColorCombiner(int combiner) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_COLOR_ICW + combiner * 4, 0);
  pb_end(p);
}

void TestHost::ClearInputColorCombiners() const {
  auto p = pb_begin();
  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_COLOR_ICW, 8);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  pb_end(p);
}

void TestHost::SetInputAlphaCombiner(int combiner, CombinerSource a_source, bool a_alpha, CombinerMapping a_mapping,
                                     CombinerSource b_source, bool b_alpha, CombinerMapping b_mapping,
                                     CombinerSource c_source, bool c_alpha, CombinerMapping c_mapping,
                                     CombinerSource d_source, bool d_alpha, CombinerMapping d_mapping) const {
  uint32_t value = MakeInputCombiner(a_source, a_alpha, a_mapping, b_source, b_alpha, b_mapping, c_source, c_alpha,
                                     c_mapping, d_source, d_alpha, d_mapping);
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_ALPHA_ICW + combiner * 4, value);
  pb_end(p);
}

void TestHost::ClearInputAlphaColorCombiner(int combiner) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_ALPHA_ICW + combiner * 4, 0);
  pb_end(p);
}

void TestHost::ClearInputAlphaCombiners() const {
  auto p = pb_begin();
  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_ALPHA_ICW, 8);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  pb_end(p);
}

uint32_t TestHost::MakeInputCombiner(CombinerSource a_source, bool a_alpha, CombinerMapping a_mapping,
                                     CombinerSource b_source, bool b_alpha, CombinerMapping b_mapping,
                                     CombinerSource c_source, bool c_alpha, CombinerMapping c_mapping,
                                     CombinerSource d_source, bool d_alpha, CombinerMapping d_mapping) const {
  auto channel = [](CombinerSource src, bool alpha, CombinerMapping mapping) {
    return src + (alpha << 4) + (mapping << 5);
  };

  uint32_t ret = (channel(a_source, a_alpha, a_mapping) << 24) + (channel(b_source, b_alpha, b_mapping) << 16) +
                 (channel(c_source, c_alpha, c_mapping) << 8) + channel(d_source, d_alpha, d_mapping);
  return ret;
}

void TestHost::SetOutputColorCombiner(int combiner, TestHost::CombinerDest ab_dst, TestHost::CombinerDest cd_dst,
                                      TestHost::CombinerDest sum_dst, bool ab_dot_product, bool cd_dot_product,
                                      TestHost::CombinerSumMuxMode sum_or_mux, TestHost::CombinerOutOp op,
                                      bool alpha_from_ab_blue, bool alpha_from_cd_blue) const {
  uint32_t value = MakeOutputCombiner(ab_dst, cd_dst, sum_dst, ab_dot_product, cd_dot_product, sum_or_mux, op);
  if (alpha_from_ab_blue) {
    value |= (1 << 19);
  }
  if (alpha_from_cd_blue) {
    value |= (1 << 18);
  }

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_COLOR_OCW + combiner * 4, value);
  pb_end(p);
}

void TestHost::ClearOutputColorCombiner(int combiner) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_COLOR_OCW + combiner * 4, 0);
  pb_end(p);
}

void TestHost::ClearOutputColorCombiners() const {
  auto p = pb_begin();
  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_COLOR_OCW, 8);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  pb_end(p);
}

void TestHost::SetOutputAlphaCombiner(int combiner, CombinerDest ab_dst, CombinerDest cd_dst, CombinerDest sum_dst,
                                      bool ab_dot_product, bool cd_dot_product, CombinerSumMuxMode sum_or_mux,
                                      CombinerOutOp op) const {
  uint32_t value = MakeOutputCombiner(ab_dst, cd_dst, sum_dst, ab_dot_product, cd_dot_product, sum_or_mux, op);
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_ALPHA_OCW + combiner * 4, value);
  pb_end(p);
}

void TestHost::ClearOutputAlphaColorCombiner(int combiner) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_ALPHA_OCW + combiner * 4, 0);
  pb_end(p);
}

void TestHost::ClearOutputAlphaCombiners() const {
  auto p = pb_begin();
  pb_push_to(SUBCH_3D, p++, NV097_SET_COMBINER_ALPHA_OCW, 8);
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  *(p++) = 0x0;
  pb_end(p);
}

uint32_t TestHost::MakeOutputCombiner(TestHost::CombinerDest ab_dst, TestHost::CombinerDest cd_dst,
                                      TestHost::CombinerDest sum_dst, bool ab_dot_product, bool cd_dot_product,
                                      TestHost::CombinerSumMuxMode sum_or_mux, TestHost::CombinerOutOp op) const {
  uint32_t ret = cd_dst | (ab_dst << 4) | (sum_dst << 8);
  if (cd_dot_product) {
    ret |= 1 << 12;
  }
  if (ab_dot_product) {
    ret |= 1 << 13;
  }
  if (sum_or_mux) {
    ret |= 1 << 14;
  }
  ret |= op << 15;

  return ret;
}

void TestHost::SetFinalCombiner0(TestHost::CombinerSource a_source, bool a_alpha, bool a_invert,
                                 TestHost::CombinerSource b_source, bool b_alpha, bool b_invert,
                                 TestHost::CombinerSource c_source, bool c_alpha, bool c_invert,
                                 TestHost::CombinerSource d_source, bool d_alpha, bool d_invert) const {
  auto channel = [](CombinerSource src, bool alpha, bool invert) { return src + (alpha << 4) + (invert << 5); };

  uint32_t value = (channel(a_source, a_alpha, a_invert) << 24) + (channel(b_source, b_alpha, b_invert) << 16) +
                   (channel(c_source, c_alpha, c_invert) << 8) + channel(d_source, d_alpha, d_invert);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW0, value);
  pb_end(p);
}

void TestHost::SetFinalCombiner1(TestHost::CombinerSource e_source, bool e_alpha, bool e_invert,
                                 TestHost::CombinerSource f_source, bool f_alpha, bool f_invert,
                                 TestHost::CombinerSource g_source, bool g_alpha, bool g_invert,
                                 bool specular_add_invert_r0, bool specular_add_invert_v1, bool specular_clamp) const {
  auto channel = [](CombinerSource src, bool alpha, bool invert) { return src + (alpha << 4) + (invert << 5); };

  // The V1+R0 sum is not available in CW1.
  ASSERT(e_source != SRC_SPEC_R0_SUM && f_source != SRC_SPEC_R0_SUM && g_source != SRC_SPEC_R0_SUM);

  uint32_t value = (channel(e_source, e_alpha, e_invert) << 24) + (channel(f_source, f_alpha, f_invert) << 16) +
                   (channel(g_source, g_alpha, g_invert) << 8);
  if (specular_add_invert_r0) {
    // NV097_SET_COMBINER_SPECULAR_FOG_CW1_SPECULAR_ADD_INVERT_R12 crashes on hardware.
    value += (1 << 5);
  }
  if (specular_add_invert_v1) {
    value += NV097_SET_COMBINER_SPECULAR_FOG_CW1_SPECULAR_ADD_INVERT_R5;
  }
  if (specular_clamp) {
    value += NV097_SET_COMBINER_SPECULAR_FOG_CW1_SPECULAR_CLAMP;
  }

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_SPECULAR_FOG_CW1, value);
  pb_end(p);
}

void TestHost::SetCombinerFactorC0(int combiner, uint32_t value) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_FACTOR0 + 4 * combiner, value);
  pb_end(p);
}

void TestHost::SetCombinerFactorC0(int combiner, float red, float green, float blue, float alpha) const {
  float rgba[4]{red, green, blue, alpha};
  SetCombinerFactorC0(combiner, TO_BGRA(rgba));
}

void TestHost::SetCombinerFactorC1(int combiner, uint32_t value) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_COMBINER_FACTOR1 + 4 * combiner, value);
  pb_end(p);
}

void TestHost::SetCombinerFactorC1(int combiner, float red, float green, float blue, float alpha) const {
  float rgba[4]{red, green, blue, alpha};
  SetCombinerFactorC1(combiner, TO_BGRA(rgba));
}

void TestHost::SetFinalCombinerFactorC0(uint32_t value) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SPECULAR_FOG_FACTOR, value);
  pb_end(p);
}

void TestHost::SetFinalCombinerFactorC0(float red, float green, float blue, float alpha) const {
  float rgba[4]{red, green, blue, alpha};
  SetFinalCombinerFactorC0(TO_BGRA(rgba));
}

void TestHost::SetFinalCombinerFactorC1(uint32_t value) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SPECULAR_FOG_FACTOR + 0x04, value);
  pb_end(p);
}

void TestHost::SetFinalCombinerFactorC1(float red, float green, float blue, float alpha) const {
  float rgba[4]{red, green, blue, alpha};
  SetFinalCombinerFactorC1(TO_BGRA(rgba));
}

void TestHost::SetShaderStageProgram(ShaderStageProgram stage_0, ShaderStageProgram stage_1, ShaderStageProgram stage_2,
                                     ShaderStageProgram stage_3) const {
  auto p = pb_begin();
  p = pb_push1(
      p, NV097_SET_SHADER_STAGE_PROGRAM,
      MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE0, stage_0) | MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE1, stage_1) |
          MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE2, stage_2) | MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE3, stage_3));
  pb_end(p);
}

void TestHost::SetShaderStageInput(uint32_t stage_2_input, uint32_t stage_3_input) const {
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SHADER_OTHER_STAGE_INPUT,
               MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE1, 0) |
                   MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE2, stage_2_input) |
                   MASK(NV097_SET_SHADER_OTHER_STAGE_INPUT_STAGE3, stage_3_input));
  pb_end(p);
}

void TestHost::OverrideVertexAttributeStride(TestHost::VertexAttribute attribute, uint32_t stride) {
  for (auto i = 0; i < 16; ++i) {
    if (attribute & (1 << i)) {
      vertex_attribute_stride_override_[i] = stride;
      return;
    }
  }

  ASSERT(!"Invalid attribute");
}

void TestHost::ClearVertexAttributeStrideOverride(TestHost::VertexAttribute attribute) {
  OverrideVertexAttributeStride(attribute, kNoStrideOverride);
}

float TestHost::NV2ARound(float input) {
  // The hardware rounding boundary is 1/16th of a pixel past 0.5
  float fraction = input - static_cast<float>(static_cast<uint32_t>(input));
  if (fraction >= 0.5625f) {
    return ceilf(input);
  }

  return floorf(input);
}

static void SetVertexAttribute(uint32_t index, uint32_t format, uint32_t size, uint32_t stride, const void *data) {
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

static void ClearVertexAttribute(uint32_t index) {
  // Note: xemu has asserts on the count for several formats, so any format without that ASSERT must be used.
  SetVertexAttribute(index, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F, 0, 0, nullptr);
}

static void GetCompositeMatrix(matrix4_t &result, const matrix4_t &model_view, const matrix4_t &projection) {
  MatrixMultMatrix(model_view, projection, result);
}
