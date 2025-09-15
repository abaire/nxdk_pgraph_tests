#include "pvideo_control.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "nxdk_ext.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

namespace PvideoControl {

static constexpr uint32_t kNumPvideoOverlays = 2;

void ClearPvideoInterrupts() { VIDEOREG(NV_PVIDEO_INTR) = NV_PVIDEO_INTR_BUFFER_0 | NV_PVIDEO_INTR_BUFFER_1; }

void SetPvideoInterruptEnabled(bool enable_zero, bool enable_one) {
  uint32_t val = SET_MASK(NV_PVIDEO_INTR_EN_BUFFER_0, enable_zero) | SET_MASK(NV_PVIDEO_INTR_EN_BUFFER_1, enable_one);
  VIDEOREG(NV_PVIDEO_INTR_EN) = val;
}

void SetPvideoBuffer(bool enable_zero, bool enable_one) {
  uint32_t val = SET_MASK(NV_PVIDEO_BUFFER_0_USE, enable_zero) | SET_MASK(NV_PVIDEO_BUFFER_1_USE, enable_one);
  VIDEOREG(NV_PVIDEO_BUFFER) = val;
}

void SetPvideoStop(uint32_t value) { VIDEOREG(NV_PVIDEO_STOP) = value; }

void SetPvideoBase(uint32_t addr, uint32_t buffer) {
  ASSERT(buffer < kNumPvideoOverlays);
  VIDEOREG(NV_PVIDEO_BASE + buffer * 0x04) = addr;
}

void SetPvideoLimit(uint32_t limit, uint32_t buffer) { VIDEOREG(NV_PVIDEO_LIMIT + buffer * 0x04) = limit; }

void SetPvideoOffset(uint32_t offset, uint32_t buffer) { VIDEOREG(NV_PVIDEO_OFFSET + buffer * 0x04) = offset; }

void SetPvideoIn(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t buffer) {
  VIDEOREG(NV_PVIDEO_POINT_IN + buffer * 0x04) = SET_MASK(NV_PVIDEO_POINT_IN_S, x) | SET_MASK(NV_PVIDEO_POINT_IN_T, y);
  VIDEOREG(NV_PVIDEO_SIZE_IN + buffer * 0x04) =
      SET_MASK(NV_PVIDEO_SIZE_IN_WIDTH, width) | SET_MASK(NV_PVIDEO_SIZE_IN_HEIGHT, height);
}

void SetPvideoOut(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t buffer) {
  VIDEOREG(NV_PVIDEO_POINT_OUT + buffer * 0x04) =
      SET_MASK(NV_PVIDEO_POINT_OUT_X, x) | SET_MASK(NV_PVIDEO_POINT_OUT_Y, y);
  VIDEOREG(NV_PVIDEO_SIZE_OUT + buffer * 0x04) =
      SET_MASK(NV_PVIDEO_SIZE_OUT_WIDTH, width) | SET_MASK(NV_PVIDEO_SIZE_OUT_HEIGHT, height);
}

void SetPvideoFormat(uint32_t format, uint32_t pitch, bool color_keyed, uint32_t buffer) {
  VIDEOREG(NV_PVIDEO_FORMAT + buffer * 0x04) = SET_MASK(NV_PVIDEO_FORMAT_PITCH, pitch) |
                                               SET_MASK(NV_PVIDEO_FORMAT_COLOR, format) |
                                               SET_MASK(NV_PVIDEO_FORMAT_DISPLAY, color_keyed);
}

void SetPvideoColorKey(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  VIDEOREG(NV_PVIDEO_COLOR_KEY) = SET_MASK(NV_PVIDEO_COLOR_KEY_RED, r) | SET_MASK(NV_PVIDEO_COLOR_KEY_GREEN, g) |
                                  SET_MASK(NV_PVIDEO_COLOR_KEY_BLUE, b) | SET_MASK(NV_PVIDEO_COLOR_KEY_ALPHA, a);
}

void SetPvideoLuminanceChrominance(uint32_t buffer) {
  VIDEOREG(NV_PVIDEO_LUMINANCE + buffer * 0x04) = 0x00001000;
  VIDEOREG(NV_PVIDEO_CHROMINANCE + buffer * 0x04) = 0x00001000;
}

void PvideoInit() {
  // Reset behavior based on observations of Dragon Ball Z Sagas FMV.
  ClearPvideoInterrupts();
  SetPvideoOffset(0);
  VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;
  VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
  VIDEOREG(NV_PVIDEO_SIZE_IN + 4) = 0xFFFFFFFF;
  VIDEOREG(NV_PVIDEO_POINT_IN + 4) = 0;
  SetPvideoBase(0);
  SetPvideoLuminanceChrominance();
}

void PvideoTeardown() {
  SetPvideoStop(1);
  SetPvideoInterruptEnabled(false, false);
  SetPvideoBuffer(false, false);
  ClearPvideoInterrupts();
  VIDEOREG(NV_PVIDEO_OFFSET) = 0;
  VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;
  VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
  VIDEOREG(NV_PVIDEO_BASE) = 0;
  VIDEOREG(NV_PVIDEO_LUMINANCE) = 0x1000;
  VIDEOREG(NV_PVIDEO_CHROMINANCE) = 0x1000;

  VIDEOREG(NV_PVIDEO_OFFSET + 4) = 0;
  VIDEOREG(NV_PVIDEO_SIZE_IN + 4) = 0xFFFFFFFF;
  VIDEOREG(NV_PVIDEO_POINT_IN + 4) = 0;
  VIDEOREG(NV_PVIDEO_BASE + 4) = 0;
  VIDEOREG(NV_PVIDEO_LUMINANCE + 4) = 0x1000;
  VIDEOREG(NV_PVIDEO_CHROMINANCE + 4) = 0x1000;
}

static inline uint32_t CalculateDelta(uint32_t in_size, uint32_t out_size) {
  ASSERT(in_size > 1);
  ASSERT(out_size > 1);
  return ((in_size - 1) << 20) / (out_size - 1);
}

void SetDsDx(uint32_t in_width, uint32_t out_width, uint32_t buffer) {
  VIDEOREG(NV_PVIDEO_DS_DX + buffer * 0x04) = CalculateDelta(in_width, out_width);
}

void SetDtDy(uint32_t in_height, uint32_t out_height, uint32_t buffer) {
  VIDEOREG(NV_PVIDEO_DT_DY + buffer * 0x04) = CalculateDelta(in_height, out_height);
}

/** Sets the texel mapping to 1:1 for both X and Y. */
void SetSquareDsDxDtDy(uint32_t buffer) {
  SetDsDx(2, 2, buffer);
  SetDtDy(2, 2, buffer);
}

}  // namespace PvideoControl
