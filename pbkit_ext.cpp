#include "pbkit_ext.h"

#include <pbkit/pbkit.h>

#include <cassert>
#include <cmath>

#include "nxdk_ext.h"

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
      p = pb_push1(p, NV097_SET_ZSTENCIL_CLEAR_VALUE, ((depth_value & 0x00FFFFFF) << 8) | stencil_value);
      break;

    default:
      assert(!"Invalid depth_buffer_format");
  }

  p = pb_push1(p, NV097_CLEAR_SURFACE, NV097_CLEAR_SURFACE_Z | NV097_CLEAR_SURFACE_STENCIL);

  pb_end(p);
}

uint16_t float_to_z16(float val) {
  if (val == 0.0f) {
    return 0;
  }

  auto int_val = reinterpret_cast<uint32_t*>(&val);
  return (*int_val >> 11) - 0x3F8000;
}

float z16_to_float(uint32_t val) {
  if (!val) {
    return 0.0f;
  }

  val = (val << 11) + 0x3C000000;
  return *(float*)&val;
}

uint32_t float_to_z24(float val) {
  if (val == 0.0f) {
    return 0;
  }

  auto int_val = reinterpret_cast<uint32_t*>(&val);
  return ((*int_val >> 7) - 0x3000000) & 0x00FFFFFF;
}

float z24_to_float(uint32_t val) {
  val &= 0x00FFFFFF;

  if (!val) {
    return 0.0f;
  }

  // XBOX 24 bit format is e8m16, convert it to a 32-bit float by shifting the exponent portion to 23:30.
  //  val = ((val & 0x00FF0000) << 7) + (val & 0x0000FFFF);
  val <<= 7;
  return *(float*)&val;
}

void pb_print_float(float value) {
  auto shift = powf(10, static_cast<float>(6));

  auto int_val = static_cast<uint32_t>(value);
  if (value < 0) {
    int_val *= -1;
  }

  value -= static_cast<float>(int_val);
  auto mantissa = static_cast<uint32_t>(value * shift);
  pb_print("%d.%06d", int_val, mantissa);
}
