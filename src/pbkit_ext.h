#ifndef NXDK_PGRAPH_TESTS_PBKIT_EXT_H
#define NXDK_PGRAPH_TESTS_PBKIT_EXT_H

#include <cstdint>
#include <list>

#define MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

constexpr float kF16Max = 511.9375f;
constexpr float kF24Max = 3.4027977E38;

// Converts a float to the format used by the 16-bit fixed point Z-buffer.
uint16_t float_to_z16(float val);

// Converts a 16-bit fixed point Z-buffer value to a float.
float z16_to_float(uint32_t val);

// Converts a float to the format used by the 24-bit fixed point Z-buffer.
uint32_t float_to_z24(float val);

// Converts a 24-bit fixed point Z-buffer value to a float.
float z24_to_float(uint32_t val);

#define PGRAPH_REGISTER_BASE 0xFD400000
#define PGRAPH_REGISTER_ARRAY_SIZE 0x2000

// Fetches (most of) the PGRAPH region from the nv2a hardware.
// `registers` must be an array of at least size PGRAPH_REGISTER_ARRAY_SIZE bytes.
void pb_fetch_pgraph_registers(uint8_t* registers);

void pb_diff_registers(const uint8_t* a, const uint8_t* b, std::list<uint32_t>& modified_registers);

#if defined(__cplusplus)
extern "C" {
#endif

// Versions of pb_print that use a full-featured printf implementation instead of the PCDLIB one that does not yet
// support floats.
void pb_print_with_floats(const char* format, ...);
#define pb_print pb_print_with_floats

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // NXDK_PGRAPH_TESTS_PBKIT_EXT_H
