#include "pvideo_tests.h"

#include <windows.h>

#include "shaders/precalculated_vertex_shader.h"
#include "texture_generator.h"

static constexpr const char kStopBehavior[] = "Stop";

PvideoTests::PvideoTests(TestHost &host, std::string output_dir) : TestSuite(host, std::move(output_dir), "PVIDEO") {
  tests_[kStopBehavior] = [this]() { TestStopBehavior(); };
}

void PvideoTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  video_ = (uint8_t *)MmAllocateContiguousMemory(host_.GetFramebufferWidth() * 2 * host_.GetFramebufferHeight());

  uint32_t pitch = host_.GetFramebufferWidth() * 4;
  auto *temp = new uint8_t[pitch * host_.GetFramebufferHeight()];
  GenerateRGBACheckerboard(temp, 0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(), pitch);
  SetVideoFrameCR8YB8CB8YA8(temp, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  delete[] temp;
}

void PvideoTests::Deinitialize() {
  TestSuite::Deinitialize();
  if (video_) {
    MmFreeContiguousMemory(video_);
    video_ = nullptr;
  }
}

void PvideoTests::SetVideoFrameCR8YB8CB8YA8(const void *pixels, uint32_t width, uint32_t height) {
  uint8_t *dest = video_;
  auto source = reinterpret_cast<const uint32_t *>(pixels);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x += 2, source += 2) {
      float R0, G0, B0, R1, G1, B1;

      R0 = static_cast<float>(source[0] & 0xFF);
      G0 = static_cast<float>((source[0] >> 8) & 0xFF);
      B0 = static_cast<float>((source[0] >> 16) & 0xFF);

      R1 = static_cast<float>(source[1] & 0xFF);
      G1 = static_cast<float>((source[1] >> 8) & 0xFF);
      B1 = static_cast<float>((source[1] >> 16) & 0xFF);

      dest[0] = static_cast<uint8_t>((0.257f * R0) + (0.504f * G0) + (0.098f * B0) + 16);    // Y0
      dest[1] = static_cast<uint8_t>(-(0.148f * R1) - (0.291f * G1) + (0.439f * B1) + 128);  // U
      dest[2] = static_cast<uint8_t>((0.257f * R1) + (0.504f * G1) + (0.098f * B1) + 16);    // Y1
      dest[3] = static_cast<uint8_t>((0.439f * R1) - (0.368f * G1) - (0.071f * B1) + 128);   // V
      dest += 4;
    }
  }
}

static void ClearPvideoInterrupts() { VIDEOREG(NV_PVIDEO_INTR) = 0x00000011; }

static void SetPvideoInterruptEnabled(bool enable_zero, bool enable_one) {
  uint32_t val = SET_MASK(NV_PVIDEO_INTR_EN_BUFFER_0, enable_zero) | SET_MASK(NV_PVIDEO_INTR_EN_BUFFER_1, enable_one);
  VIDEOREG(NV_PVIDEO_INTR_EN) = val;
}

static void SetPvideoBuffer(bool enable_zero, bool enable_one) {
  uint32_t val = SET_MASK(NV_PVIDEO_BUFFER_0_USE, enable_zero) | SET_MASK(NV_PVIDEO_BUFFER_1_USE, enable_one);
  VIDEOREG(NV_PVIDEO_BUFFER) = val;
}

static void SetPvideoStop(uint32_t value = 0) { VIDEOREG(NV_PVIDEO_STOP) = value; }

static void SetPvideoBase(uint32_t addr) { VIDEOREG(NV_PVIDEO_BASE) = addr; }

static void SetPvideoLimit(uint32_t limit) { VIDEOREG(NV_PVIDEO_LIMIT) = limit; }

static void SetPvideoOffset(uint32_t offset) { VIDEOREG(NV_PVIDEO_OFFSET) = offset; }

static void SetPvideoIn(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
  VIDEOREG(NV_PVIDEO_POINT_IN) = SET_MASK(NV_PVIDEO_POINT_IN_S, x) | SET_MASK(NV_PVIDEO_POINT_IN_T, y);
  VIDEOREG(NV_PVIDEO_SIZE_IN) = SET_MASK(NV_PVIDEO_SIZE_IN_WIDTH, width) | SET_MASK(NV_PVIDEO_SIZE_IN_HEIGHT, height);
}

static void SetPvideoOut(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
  VIDEOREG(NV_PVIDEO_POINT_OUT) = SET_MASK(NV_PVIDEO_POINT_OUT_X, x) | SET_MASK(NV_PVIDEO_POINT_OUT_Y, y);
  VIDEOREG(NV_PVIDEO_SIZE_OUT) =
      SET_MASK(NV_PVIDEO_SIZE_OUT_WIDTH, width) | SET_MASK(NV_PVIDEO_SIZE_OUT_HEIGHT, height);
}

static void SetPvideoFormat(uint32_t format, uint32_t pitch, bool color_keyed) {
  VIDEOREG(NV_PVIDEO_FORMAT) = SET_MASK(NV_PVIDEO_FORMAT_PITCH, pitch) | SET_MASK(NV_PVIDEO_FORMAT_COLOR, format) |
                               SET_MASK(NV_PVIDEO_FORMAT_DISPLAY, color_keyed);
}

static void SetPvideoColorKey(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  VIDEOREG(NV_PVIDEO_COLOR_KEY) = SET_MASK(NV_PVIDEO_COLOR_KEY_RED, r) | SET_MASK(NV_PVIDEO_COLOR_KEY_GREEN, g) |
                                  SET_MASK(NV_PVIDEO_COLOR_KEY_BLUE, b) | SET_MASK(NV_PVIDEO_COLOR_KEY_ALPHA, a);
}

static void SetPvideoLuminanceChrominance() {
  VIDEOREG(NV_PVIDEO_LUMINANCE) = 0x00001000;
  VIDEOREG(NV_PVIDEO_CHROMINANCE) = 0x00001000;
}

void PvideoTests::TestStopBehavior() {
  host_.PrepareDraw(0xFF250535);

  uint32_t pitch = host_.GetFramebufferWidth() * 2;

  ClearPvideoInterrupts();

  // Seen in Dragon Ball Z Saga, has no observable effect.
  // VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;

  VIDEOREG(NV_PVIDEO_POINT_IN) = 0;

  SetPvideoBase(VRAM_ADDR(video_));
  uint32_t end = VRAM_ADDR(video_) + pitch * host_.GetFramebufferHeight();
  SetPvideoLimit(end);
  SetPvideoOffset(0);

  // TODO: See if it's necessary to set these.
  SetPvideoLuminanceChrominance();

  for (uint32_t i = 0; i < 10; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    //    SetPvideoOffset(0);
    //    SetPvideoOffset(VRAM_ADDR(video_));
    SetPvideoIn(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    VIDEOREG(NV_PVIDEO_DS_DX) = 0x00100000;
    VIDEOREG(NV_PVIDEO_DT_DY) = 0x00100000;
    SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, pitch, false);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
  }

  // This is the key value, if stop's low bit is set, the PVIDEO overlay is torn down completely. If it is not, it
  // stays up in spite of being stopped.
  // SetPvideoStop(0xFFFFFFFE);
  SetPvideoStop(1);
  SetPvideoBuffer(false, false);

  host_.FinishDraw(false, output_dir_, kStopBehavior);
}
