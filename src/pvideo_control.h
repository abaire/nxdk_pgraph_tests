#ifndef NXDK_PGRAPH_TESTS_SRC_PVIDEO_CONTROLLER_H_
#define NXDK_PGRAPH_TESTS_SRC_PVIDEO_CONTROLLER_H_

#include <cstdint>

namespace PvideoControl {

void PvideoInit();

void PvideoTeardown();

/** Clears all active PVIDEO interrupt flags. */
void ClearPvideoInterrupts();

/**
 * Sets interrupt enable flag for the PVIDEO buffers.
 *
 * @param enable_buffer_zero Whether interrupts should be enabled for PVIDEO overlay 0
 * @param enable_buffer_one Whether interrupts should be enabled for PVIDEO overlay 1
 */
void SetPvideoInterruptEnabled(bool enable_buffer_zero, bool enable_buffer_one);

/**
 * Enables or disables PVIDEO overlays.
 *
 * Note that this needs to be called after each frame
 *
 * @param enable_buffer_zero Whether PVIDEO overlay 0 should be enabled
 * @param enable_buffer_one Whether PVIDEO overlay 1 should be enabled
 */
void SetPvideoBuffer(bool enable_buffer_zero, bool enable_buffer_one);

void SetPvideoStop(uint32_t value = 0);

/**
 * Sets the base address of the PVIDEO overlay content.
 * @param addr The address from which PVIDEO data should be read.
 * @param buffer The buffer whose base should be set.
 */
void SetPvideoBase(uint32_t addr, uint32_t buffer = 0);

void SetPvideoLimit(uint32_t limit, uint32_t buffer = 0);

/**
 * Sets the offset address of the PVIDEO overlay content.
 * @param offset offset added to the base address configured via SetPvideoBase from which PVIDEO data should be read.
 * @param buffer The buffer whose base should be set.
 */
void SetPvideoOffset(uint32_t offset, uint32_t buffer = 0);

/**
 * Defines the region in the source video buffer that the overlay will read from.
 */
void SetPvideoIn(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t buffer = 0);

/**
 * Defines the screen region that will be covered by the overlay.
 */
void SetPvideoOut(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t buffer = 0);

void SetPvideoFormat(uint32_t format, uint32_t pitch, bool color_keyed, uint32_t buffer = 0);

/** Sets the RGB value for PVIDEO overlay color keying.
 *
 *  Any pixels in the VGA framebuffer within the PVIDEO overlay region that match the given RGB value will be replaced
 *  with the content of the overlay.
 *
 *  NOTE: alpha is ignored by nv2a hardware.
 */
void SetPvideoColorKey(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0);
inline void SetPvideoColorKey(uint32_t color_key) {
  SetPvideoColorKey(color_key & 0xFF, (color_key >> 8) & 0xFF, (color_key >> 16) & 0xFF, (color_key & 24) & 0xFF);
}

// TODO: Allow values to be passed.
void SetPvideoLuminanceChrominance(uint32_t buffer = 0);

void SetDsDx(uint32_t in_width, uint32_t out_width, uint32_t buffer = 0);
void SetDtDy(uint32_t in_height, uint32_t out_height, uint32_t buffer = 0);

/** Sets the texel mapping to 1:1 for both X and Y. */
void SetSquareDsDxDtDy(uint32_t buffer = 0);

}  // namespace PvideoControl

#endif  // NXDK_PGRAPH_TESTS_SRC_PVIDEO_CONTROLLER_H_
