#ifndef NXDK_PGRAPH_TESTS_PBKIT_EXT_H
#define NXDK_PGRAPH_TESTS_PBKIT_EXT_H

#include <cstdint>

void set_depth_buffer_format(uint32_t depth_buffer_format);
void set_depth_stencil_buffer_region(uint32_t depth_value, uint8_t stencil_value, uint32_t left, uint32_t top,
                                     uint32_t width, uint32_t height);

#endif  // NXDK_PGRAPH_TESTS_PBKIT_EXT_H
