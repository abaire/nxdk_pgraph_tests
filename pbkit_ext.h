#ifndef NXDK_PGRAPH_TESTS_PBKIT_EXT_H
#define NXDK_PGRAPH_TESTS_PBKIT_EXT_H

#include <pbkit/pbkit.h>
#include <strings.h>

#include <cstdint>

// "Beta" class for blending operations (see xf86-video-nouveau).
#define GR_CLASS_12 0x12

#define MASK(mask, val) (((val) << (ffs(mask) - 1)) & (mask))

void set_depth_stencil_buffer_region(uint32_t depth_buffer_format, uint32_t depth_value, uint8_t stencil_value,
                                     uint32_t left, uint32_t top, uint32_t width, uint32_t height);

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

void pb_print_float(float value);

// Points an existing DMA context object at a new address.
void pb_set_dma_address(const struct s_CtxDma* context, const void* address, uint32_t limit);

// Binds a subchannel to the given context.
void pb_bind_subchannel(uint32_t subchannel, const struct s_CtxDma* context);

#endif  // NXDK_PGRAPH_TESTS_PBKIT_EXT_H
