#ifndef NXDK_PGRAPH_TESTS_PBKIT_EXT_H
#define NXDK_PGRAPH_TESTS_PBKIT_EXT_H

#include <pbkit/pbkit.h>
#include <printf/printf.h>
#include <strings.h>

#include <cstdint>

// "Beta" class for blending operations (see xf86-video-nouveau).
#define GR_CLASS_12 0x12

// "Beta4" class for blending operations (see xf86-video-nouveau).
#define GR_CLASS_72 0x72

// Enumerated DMA channel destinations
#define DMA_CHANNEL_PIXEL_RENDERER 9
#define DMA_CHANNEL_DEPTH_STENCIL_RENDERER 10
#define DMA_CHANNEL_BITBLT_IMAGES 11

// Value that may be added to contiguous memory addresses to access as ADDR_AGPMEM, which is guaranteed to be linear
// (and thus may be slower than tiled ADDR_FBMEM but can be manipulated directly).
#define AGP_MEMORY_REMAP 0xF0000000

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

// Points an existing DMA context object at a new address.
void pb_set_dma_address(const struct s_CtxDma* context, const void* address, uint32_t limit);

// Binds a subchannel to the given context.
void pb_bind_subchannel(uint32_t subchannel, const struct s_CtxDma* context);

// Returns an AGP version of the given nv2a tiled memory address.
void* pb_agp_access(void* fb_memory_pointer);

// Pushes a 4 column x 3 row matrix (used to push the inverse matrix which intentionally omits the 4th row and is
// not transposed).
uint32_t* pb_push_4x3_matrix(uint32_t* p, DWORD command, const float* m);

// Versions of pb_print that use a full-featured printf implementation instead of the PCDLIB one that does not yet
// support floats.
void pb_print_with_floats(const char* format, ...);
#define pb_print pb_print_with_floats

#endif  // NXDK_PGRAPH_TESTS_PBKIT_EXT_H
