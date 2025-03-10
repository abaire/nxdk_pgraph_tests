#include "pbkit_ext.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"

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
