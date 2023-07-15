#include "pbkit_ext.h"

#include <pbkit/pbkit.h>

#include <cmath>
#include <vector>

#include "debug_output.h"
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
      ASSERT(!"Invalid depth_buffer_format");
  }

  p = pb_push1(p, NV097_CLEAR_SURFACE, NV097_CLEAR_SURFACE_Z | NV097_CLEAR_SURFACE_STENCIL);

  pb_end(p);
}

uint16_t float_to_z16(float val) {
  if (val == 0.0f) {
    return 0;
  }

  auto int_val = reinterpret_cast<uint32_t *>(&val);
  return (*int_val >> 11) - 0x3F8000;
}

float z16_to_float(uint32_t val) {
  if (!val) {
    return 0.0f;
  }

  val = (val << 11) + 0x3C000000;
  return *(float *)&val;
}

uint32_t float_to_z24(float val) {
  if (val == 0.0f) {
    return 0;
  }

  auto int_val = reinterpret_cast<uint32_t *>(&val);
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
  return *(float *)&val;
}

#define DMA_CLASS_3D 0x3D
#define PB_SETOUTER 0xB2A

void pb_set_dma_address(const struct s_CtxDma *context, const void *address, uint32_t limit) {
  uint32_t dma_addr = reinterpret_cast<uint32_t>(address) & 0x03FFFFFF;
  uint32_t dma_flags = DMA_CLASS_3D | 0x0000B000;
  dma_addr |= 3;

  auto p = pb_begin();
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_WAIT_MAKESPACE, 0);

  // set params addr,data
  p = pb_push2(p, NV20_TCL_PRIMITIVE_3D_PARAMETER_A, NV_PRAMIN + (context->Inst << 4) + 0x08, dma_addr);

  // calls subprogID PB_SETOUTER: does VIDEOREG(addr)=data
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_FIRE_INTERRUPT, PB_SETOUTER);
  p = pb_push2(p, NV20_TCL_PRIMITIVE_3D_PARAMETER_A, NV_PRAMIN + (context->Inst << 4) + 0x0C, dma_addr);
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_FIRE_INTERRUPT, PB_SETOUTER);
  p = pb_push2(p, NV20_TCL_PRIMITIVE_3D_PARAMETER_A, NV_PRAMIN + (context->Inst << 4) + 0x00, dma_flags);
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_FIRE_INTERRUPT, PB_SETOUTER);
  p = pb_push2(p, NV20_TCL_PRIMITIVE_3D_PARAMETER_A, NV_PRAMIN + (context->Inst << 4) + 0x04, limit);
  p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_FIRE_INTERRUPT, PB_SETOUTER);

  pb_end(p);
}

void pb_bind_subchannel(uint32_t subchannel, const struct s_CtxDma *context) {
  auto p = pb_begin();
  p = pb_push1_to(subchannel, p, NV20_TCL_PRIMITIVE_SET_MAIN_OBJECT, context->ChannelID);
  pb_end(p);
}

void *pb_agp_access(void *fb_memory_pointer) {
  return reinterpret_cast<void *>(reinterpret_cast<uint32_t>(fb_memory_pointer) | AGP_MEMORY_REMAP);
}

uint32_t *pb_push_4x3_matrix(uint32_t *p, DWORD command, const float *m) {
  pb_push_to(SUBCH_3D, p++, command, 12);

  *((float *)p++) = m[_11];
  *((float *)p++) = m[_12];
  *((float *)p++) = m[_13];
  *((float *)p++) = m[_14];

  *((float *)p++) = m[_21];
  *((float *)p++) = m[_22];
  *((float *)p++) = m[_23];
  *((float *)p++) = m[_24];

  *((float *)p++) = m[_31];
  *((float *)p++) = m[_32];
  *((float *)p++) = m[_33];
  *((float *)p++) = m[_34];

  return p;
}

uint32_t *pb_push_4x4_matrix(uint32_t *p, DWORD command, const float *m) {
  pb_push_to(SUBCH_3D, p++, command, 16);

  *((float *)p++) = m[_11];
  *((float *)p++) = m[_12];
  *((float *)p++) = m[_13];
  *((float *)p++) = m[_14];

  *((float *)p++) = m[_21];
  *((float *)p++) = m[_22];
  *((float *)p++) = m[_23];
  *((float *)p++) = m[_24];

  *((float *)p++) = m[_31];
  *((float *)p++) = m[_32];
  *((float *)p++) = m[_33];
  *((float *)p++) = m[_34];

  *((float *)p++) = m[_41];
  *((float *)p++) = m[_42];
  *((float *)p++) = m[_43];
  *((float *)p++) = m[_44];

  return p;
}

void pb_print_with_floats(const char *format, ...) {
  char buffer[512];

  va_list argList;
  va_start(argList, format);
  vsnprintf_(buffer, 512, format, argList);
  va_end(argList);

  char *str = buffer;
  while (*str != 0) {
    pb_print_char(*str++);
  }
}

uint32_t *pb_push1f(uint32_t *p, DWORD command, float param1) {
  pb_push_to(SUBCH_3D, p, command, 1);
  *((float *)(p + 1)) = param1;
  return p + 2;
}

uint32_t *pb_push2f(uint32_t *p, DWORD command, float param1, float param2) {
  pb_push_to(SUBCH_3D, p, command, 2);
  *((float *)(p + 1)) = param1;
  *((float *)(p + 2)) = param2;
  return p + 3;
}

uint32_t *pb_push3f(uint32_t *p, DWORD command, float param1, float param2, float param3) {
  pb_push_to(SUBCH_3D, p, command, 3);
  *((float *)(p + 1)) = param1;
  *((float *)(p + 2)) = param2;
  *((float *)(p + 3)) = param3;
  return p + 4;
}

void pb_fetch_pgraph_registers(uint8_t *registers) {
  // See https://github.com/XboxDev/nv2a-trace/blob/65bdd2369a5b216cfc47c9545f870c49d118276b/Trace.py#L32

  while (pb_busy()) {
    /* Wait for any pending pgraph commands to be flushed */
  }

  constexpr uint32_t kPGRAPHRegisterEnd = PGRAPH_REGISTER_BASE + PGRAPH_REGISTER_ARRAY_SIZE;

  struct Range {
    uint32_t begin;
    uint32_t end;
  };

  // clang-format off
  static constexpr Range kReadRanges[] = {
      {PGRAPH_REGISTER_BASE, 0xFD400200}, // From nv2a-trace: "0xFD400200 hangs Xbox, I just skipped to 0x400."
      // Spotchecked 300, 360, 3B0, 3F0 also hang
      {0xFD400400, 0xFD400754}, // From xemu, 0xFD400754 is the RDI interface.
      {0xFD400758, kPGRAPHRegisterEnd},
  };
  // clang-format on

  // Read a dword at a time as the target is MMIO.
  uint32_t *last_empty = nullptr;
  for (const auto &range : kReadRanges) {
    auto src = reinterpret_cast<const uint32_t *>(range.begin);
    auto end = reinterpret_cast<const uint32_t *>(range.end);
    auto offset = range.begin - PGRAPH_REGISTER_BASE;
    auto dst = reinterpret_cast<uint32_t *>(registers + offset);

    while (last_empty && last_empty < dst) {
      *last_empty++ = 0xBAD0BAD0;
    }

    while (src < end) {
      if ((intptr_t)src >= 0xFD400200 && (intptr_t)src < 0xFD400400) {
        PrintMsg("Checking 0x%p\n", src);
        Sleep(10);
      }
      *dst++ = *src++;
    }
    last_empty = dst;
  }
}

void pb_diff_registers(const uint8_t *a, const uint8_t *b, std::list<uint32_t> &modified_registers) {
  modified_registers.clear();

  auto old_val = reinterpret_cast<const uint32_t *>(a);
  auto new_val = reinterpret_cast<const uint32_t *>(b);

  for (auto i = 0; i < PGRAPH_REGISTER_ARRAY_SIZE / 4; ++i, ++old_val, ++new_val) {
    if (*old_val != *new_val) {
      modified_registers.push_back(PGRAPH_REGISTER_BASE + i * 4);
    }
  }
}
