#include "pbkit_ext.h"

#include <pbkit/pbkit.h>

#include <cassert>

#include "nxdk_ext.h"

void set_depth_buffer_format(uint32_t depth_buffer_format) {
  // Override the values set in pb_init. Unfortunately the default is not exposed and must be recreated here.
  auto p = pb_begin();
  uint32_t color_format = NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8;
  uint32_t frame_buffer_format = (NV097_SET_SURFACE_FORMAT_TYPE_PITCH << 8) | (depth_buffer_format << 4) | color_format;
  uint32_t fbv_flag = 0;
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_BUFFER_FORMAT, frame_buffer_format | fbv_flag);

  uint32_t max_depth;
  switch (depth_buffer_format) {
    case NV097_SET_SURFACE_FORMAT_ZETA_Z16:
      *((float*)&max_depth) = static_cast<float>(0x0000FFFF);
      break;

    case NV097_SET_SURFACE_FORMAT_ZETA_Z24S8:
      *((float*)&max_depth) = static_cast<float>(0x00FFFFFF);
      break;
  }

  p = pb_push1(p, NV097_SET_CLIP_MIN, 0);
  p = pb_push1(p, NV097_SET_CLIP_MAX, max_depth);

  pb_end(p);
}

void set_depth_stencil_buffer_region(uint32_t depth_buffer_format, uint32_t depth_value, uint8_t stencil_value,
                                     uint32_t left, uint32_t top, uint32_t width, uint32_t height) {
  // See: pbkit.c: pb_erase_depth_stencil_buffer
  uint32_t right = left + width;
  uint32_t bottom = top + height;

  auto p = pb_begin();

  // sets rectangle coordinates
  p = pb_push1(p, NV097_SET_CLEAR_RECT_HORIZONTAL, ((right - 1) << 16) | (left & 0xFFFF));
  p = pb_push1(p, NV097_SET_CLEAR_RECT_VERTICAL, ((bottom - 1) << 16) | (top & 0xFFFF));

  switch (depth_buffer_format) {
    case NV097_SET_SURFACE_FORMAT_ZETA_Z16:
      p = pb_push1(p, NV097_SET_ZSTENCIL_CLEAR_VALUE, depth_value & 0xFFFF);
      break;

    case NV097_SET_SURFACE_FORMAT_ZETA_Z24S8:
      p = pb_push1(p, NV097_SET_ZSTENCIL_CLEAR_VALUE, ((depth_value & 0xFFFFFF) << 8) | stencil_value);
      break;

    default:
      assert(!"Invalid depth_buffer_format");
  }

  depth_value &= 0x00FFFFFF;
  p = pb_push1(p, NV097_CLEAR_SURFACE, NV097_CLEAR_SURFACE_Z | NV097_CLEAR_SURFACE_STENCIL);

  pb_end(p);
}
