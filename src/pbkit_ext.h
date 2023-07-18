#ifndef NXDK_PGRAPH_TESTS_PBKIT_EXT_H
#define NXDK_PGRAPH_TESTS_PBKIT_EXT_H

#include <pbkit/pbkit.h>
#include <printf/printf.h>
#include <strings.h>

#include <cstdint>
#include <list>

// From pbkit.c
#define MAXRAM 0x03FFAFFF

// "Beta" class for blending operations (see xf86-video-nouveau).
#define GR_CLASS_12 0x12

// "Beta4" class for blending operations (see xf86-video-nouveau).
#define GR_CLASS_72 0x72

// Enumerated DMA channel destinations
#define DMA_CHANNEL_3D_3 3
#define DMA_CHANNEL_PIXEL_RENDERER 9
#define DMA_CHANNEL_DEPTH_STENCIL_RENDERER 10
#define DMA_CHANNEL_BITBLT_IMAGES 11

// Value that may be added to contiguous memory addresses to access as ADDR_AGPMEM, which is guaranteed to be linear
// (and thus may be slower than tiled ADDR_FBMEM but can be manipulated directly).
#define AGP_MEMORY_REMAP 0xF0000000

#define MASK(mask, val) (((val) << (ffs(mask) - 1)) & (mask))

// Indicates that all values being pushed in a pb_push* invocation should be sent to the same command.
//
// The default behavior is for pgraph commands that do not operate on fixed size arrays to treat multiple parameters as
// invocations of subsequent pgraph commands.
// E.g., NV097_INLINE_ARRAY (0x00001818) expects a single parameter which is added to the inline array being built up
// in graphics memory. Rather than invoke pb_push1 many times over and waste time/space re-sending the
// NV097_INLINE_ARRAY command, it is desirable to send multiple components (e.g., all 3 coordinates for a vertex) in a
// single push. Setting bit 30 (i.e., NV2A_SUPPRESS_COMMAND_INCREMENT(NV097_INLINE_ARRAY)) instructs the pushbuffer to
// treat all the parameters in the command packet as part of the NV097_INLINE_ARRAY command.
// Failure to set bit 30 would utilize the default behavior, so the first parameter would go to NV097_INLINE_ARRAY, but
// the next would go to NV097_SET_EYE_VECTOR (0x0000181C), which itself will expect 3 parameters and will at best lead
// to unexpected behavior and potentially an exception being raised by the hardware depending on how many parameters in
// total are being sent.
#define NV2A_SUPPRESS_COMMAND_INCREMENT(cmd) (0x40000000 | (cmd))

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

// Pushes a 4x4 matrix (not transposed).
uint32_t* pb_push_4x4_matrix(uint32_t* p, DWORD command, const float* m);

uint32_t* pb_push1f(uint32_t* p, DWORD command, float param1);
uint32_t* pb_push2f(uint32_t* p, DWORD command, float param1, float param2);
uint32_t* pb_push3f(uint32_t* p, DWORD command, float param1, float param2, float param3);

uint32_t* pb_push2fv(uint32_t* p, DWORD command, const float* vector2);
uint32_t* pb_push3fv(uint32_t* p, DWORD command, const float* vector3);
uint32_t* pb_push4fv(uint32_t* p, DWORD command, const float* vector4);

uint32_t* pb_push2v(uint32_t* p, DWORD command, const uint32_t* vector2);
uint32_t* pb_push3v(uint32_t* p, DWORD command, const uint32_t* vector3);
uint32_t* pb_push4v(uint32_t* p, DWORD command, const uint32_t* vector4);

// Versions of pb_print that use a full-featured printf implementation instead of the PCDLIB one that does not yet
// support floats.
void pb_print_with_floats(const char* format, ...);
#define pb_print pb_print_with_floats

#define PGRAPH_REGISTER_BASE 0xFD400000
#define PGRAPH_REGISTER_ARRAY_SIZE 0x2000

// Fetches (most of) the PGRAPH region from the nv2a hardware.
// `registers` must be an array of at least size PGRAPH_REGISTER_ARRAY_SIZE bytes.
void pb_fetch_pgraph_registers(uint8_t* registers);

void pb_diff_registers(const uint8_t* a, const uint8_t* b, std::list<uint32_t>& modified_registers);

#endif  // NXDK_PGRAPH_TESTS_PBKIT_EXT_H
