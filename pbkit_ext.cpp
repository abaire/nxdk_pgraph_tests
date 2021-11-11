#include "pbkit_ext.h"

#include <pbkit/pbkit.h>

void set_depth_buffer_format(uint32_t depth_buffer_format) {
  // Override the values set in pb_init. Unfortunately this is not exposed
  auto p = pb_begin();
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_W_YUV_FPZ_FLAGS, 0x00110001);
  uint32_t color_format = NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8;
  uint32_t frame_buffer_format = 0x100 | (depth_buffer_format << 4) | color_format;
  uint32_t fbv_flag = 0;
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_BUFFER_FORMAT, frame_buffer_format | fbv_flag);
  pb_end(p);
}

void set_depth_stencil_buffer_region(uint32_t depth_value, uint8_t stencil_value, uint32_t left, uint32_t top,
                                     uint32_t width, uint32_t height) {
  // See: pbkit.c: pb_erase_depth_stencil_buffer
  uint32_t right = left + width;
  uint32_t bottom = top + height;

  auto p = pb_begin();

  // sets rectangle coordinates
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_CLEAR_VALUE_HORIZ, 2);
  *(p++) = ((right - 1) << 16) | left;
  *(p++) = ((bottom - 1) << 16) | top;

  // sets data used to fill in rectangle
  pb_push(p++, NV20_TCL_PRIMITIVE_3D_CLEAR_VALUE_DEPTH, 3);
  *(p++) = ((depth_value & 0x00FFFFFF) << 8) | stencil_value;
  *(p++) = 0;     // color - ignored due to mode flag below.
  *(p++) = 0x03;  // mode: depth & stencil.

  pb_end(p);
}
