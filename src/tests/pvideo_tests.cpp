#include "pvideo_tests.h"

// Note: pbkit eats any PVIDEO interrupts in `DPC`, so they cannot be used to feed the buffer.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#include <windows.h>
#pragma clang diagnostic pop

#include "debug_output.h"
#include "shaders/precalculated_vertex_shader.h"
#include "swizzle.h"
#include "texture_generator.h"

static constexpr const char kStopBehavior[] = "Stop";
static constexpr const char kAlternateStop[] = "Stop Alt";

static constexpr const char kSizeInMaxUnity[] = "Size In Max - dI/dO = 1";
static constexpr const char kSizeInMaxLarge[] = "Size In Max - dI/dO > 1";
static constexpr const char kSizeInMaxSmall[] = "Size In Max - dI/dO < 1";
static constexpr const char kSizeInMaxOutSmallUnity[] = "Size In Max Out Small - dI/dO = 1";
static constexpr const char kSizeInMaxOutSmallCorrect[] = "Size In Max Out Small - dI/d0 Correct";
static constexpr const char kSizeInLargerThanSizeOutUnity[] = "Size In larger than out - dI/dO = 1";
static constexpr const char kSizeInLargerThanSizeOutCorrect[] = "Size In larger than out - dI/d0 Correct";
static constexpr const char kSizeInSmallerThanSizeOutUnity[] = "Size In smaller than out - dI/dO = 1";
static constexpr const char kSizeInSmallerThanSizeOutCorrect[] = "Size In smaller than out - dI/d0 Correct";
static constexpr const char kPitchLessThanCompact[] = "Pitch less than compact";
static constexpr const char kPitchLargerThanCompact[] = "Pitch larger than compact";
static constexpr const char kPALIntoNTSC[] = "PAL into NTSC overlay";

PvideoTests::PvideoTests(TestHost &host, std::string output_dir) : TestSuite(host, std::move(output_dir), "PVIDEO") {
  tests_[kPALIntoNTSC] = [this]() { TestPALIntoNTSC(); };
  tests_[kStopBehavior] = [this]() { TestStopBehavior(); };
  // This seems to permanently kill video output on 1.0 devkit.
  //  tests_[kAlternateStop] = [this]() { TestAlternateStopBehavior(); };

  tests_[kSizeInMaxUnity] = [this]() { TestSizeInMaxUnityDeltas(); };
  tests_[kSizeInMaxLarge] = [this]() { TestSizeInMaxLargeDelta(); };
  tests_[kSizeInMaxSmall] = [this]() { TestSizeInMaxSmallDelta(); };

  tests_[kSizeInMaxOutSmallUnity] = [this]() { TestSizeMaxOutSmallUnityDeltas(); };
  tests_[kSizeInMaxOutSmallCorrect] = [this]() { TestSizeMaxOutSmallCorrectDeltas(); };

  tests_[kSizeInLargerThanSizeOutUnity] = [this]() { TestSizeInLargerThanSizeOutUnityDeltas(); };
  tests_[kSizeInLargerThanSizeOutCorrect] = [this]() { TestSizeInLargerThanSizeOutCorrectDeltas(); };

  tests_[kSizeInSmallerThanSizeOutUnity] = [this]() { TestSizeInSmallerThanSizeOutUnityDeltas(); };
  tests_[kSizeInSmallerThanSizeOutCorrect] = [this]() { TestSizeInSmallerThanSizeOutCorrectDeltas(); };

  tests_[kPitchLessThanCompact] = [this]() { TestPitchLessThanCompact(); };
  tests_[kPitchLargerThanCompact] = [this]() { TestPitchLargerThanCompact(); };
}

void PvideoTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  // Twice the actual space needed is allocated to facilitate tests with pitch > compact.
  const uint32_t size = host_.GetFramebufferWidth() * 4 * host_.GetFramebufferHeight();
  video_ = (uint8_t *)MmAllocateContiguousMemory(size);
  memset(video_, 0x7F, size);
}

void PvideoTests::Deinitialize() {
  TestSuite::Deinitialize();
  if (video_) {
    MmFreeContiguousMemory(video_);
    video_ = nullptr;
  }
}

void PvideoTests::SetCheckerboardVideoFrameCR8YB8CB8YA8(uint32_t first_color, uint32_t second_color,
                                                        uint32_t checker_size, uint32_t x_offset, uint32_t y_offset,
                                                        uint32_t width, uint32_t height) {
  if (!width) {
    width = host_.GetFramebufferWidth();
  }
  if (!height) {
    height = host_.GetFramebufferHeight();
  }

  const uint32_t pitch = width * 4;
  auto *temp = new uint8_t[pitch * height];
  GenerateRGBACheckerboard(temp, 0, 0, width, height, pitch, first_color, second_color, checker_size);
  SetVideoFrameCR8YB8CB8YA8(temp, width, height);
  delete[] temp;
}

void PvideoTests::SetTestPatternVideoFrameCR8YB8CB8YA8(uint32_t width, uint32_t height) {
  if (!width) {
    width = host_.GetFramebufferWidth();
  }
  if (!height) {
    height = host_.GetFramebufferHeight();
  }

  const uint32_t pitch = width * 4;
  auto *temp = new uint8_t[pitch * height];
  GenerateRGBATestPattern(temp, width, height);
  SetVideoFrameCR8YB8CB8YA8(temp, width, height);
  delete[] temp;
}

void PvideoTests::SetVideoFrameCR8YB8CB8YA8(const void *pixels, uint32_t width, uint32_t height) {
  uint8_t *row = video_;
  uint32_t dest_pitch = host_.GetFramebufferWidth() * 2;

  auto source = reinterpret_cast<const uint32_t *>(pixels);
  for (int y = 0; y < height; ++y) {
    uint8_t *dest = row;
    row += dest_pitch;
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

static void PvideoInit() {
  // Reset behavior based on observations of Dragon Ball Z Sagas FMV.
  ClearPvideoInterrupts();
  SetPvideoOffset(0);
  VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;
  VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
  SetPvideoBase(0);
  SetPvideoLuminanceChrominance();
}

static void PvideoTeardown() {
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
}

static inline uint32_t CalculateDelta(uint32_t in_size, uint32_t out_size) {
  ASSERT(in_size > 1);
  ASSERT(out_size > 1);
  return ((in_size - 1) << 20) / (out_size - 1);
}

static void SetDsDx(uint32_t in_width, uint32_t out_width) {
  VIDEOREG(NV_PVIDEO_DS_DX) = CalculateDelta(in_width, out_width);
}

static void SetDtDy(uint32_t in_height, uint32_t out_height) {
  VIDEOREG(NV_PVIDEO_DT_DY) = CalculateDelta(in_height, out_height);
}

void PvideoTests::TestStopBehavior() {
  host_.PrepareDraw(0xFF250535);

  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFF00FFFF, 0xFF000000, 8);
  PvideoInit();
  DbgPrint("Displaying video overlay...\n");

  for (uint32_t i = 0; i < 30; ++i) {
    DrawFullscreenOverlay();
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");

  // This is the key value, if stop's low bit is set, the PVIDEO overlay is torn down completely. If it is not, it
  // stays up in spite of being stopped.
  // SetPvideoStop(0xFFFFFFFE);
  SetPvideoStop(1);
  ClearPvideoInterrupts();
  VIDEOREG(NV_PVIDEO_OFFSET) = 0;
  VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;
  VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
  VIDEOREG(NV_PVIDEO_BASE) = 0;
  VIDEOREG(NV_PVIDEO_LUMINANCE) = 0x1000;
  VIDEOREG(NV_PVIDEO_CHROMINANCE) = 0x1000;

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kStopBehavior);
}

void PvideoTests::TestAlternateStopBehavior() {
  host_.PrepareDraw(0xFF250535);

  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFF00FFFF, 0xFF000000, 8);
  PvideoInit();
  DbgPrint("Displaying video overlay...\n");
  DrawFullscreenOverlay();

  DbgPrint("Immediately stopping video overlay with unknown registers\n");

  //  const uint32_t NV_PVIDEO_UNKNOWN_88 = 0x00008088;
  //  const uint32_t NV_PVIDEO_UNKNOWN_8C = 0x0000808C;
  //  VIDEOREG(NV_PVIDEO_UNKNOWN_88) = 0x04000400;
  //  VIDEOREG(NV_PVIDEO_UNKNOWN_88) = 0x04000400;
  //  VIDEOREG(NV_PVIDEO_UNKNOWN_8C) = 0x04000400;
  //  VIDEOREG(NV_PVIDEO_UNKNOWN_8C) = 0x04000400;

  SetPvideoLuminanceChrominance();
  VIDEOREG(NV_PVIDEO_DS_DX) = 0x00100000;
  VIDEOREG(NV_PVIDEO_DT_DY) = 0x00100000;
  VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
  VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;
  SetPvideoLuminanceChrominance();
  VIDEOREG(NV_PVIDEO_DS_DX) = 0x00100000;
  VIDEOREG(NV_PVIDEO_DT_DY) = 0x00100000;
  VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
  VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;

  // Sequence used in bootup and in Ultimate Beach Soccer.
  VIDEOREG(NV_PMC_ENABLE) = 0x0;
  VIDEOREG(NV_PMC_ENABLE) = 0x0;
  VIDEOREG(NV_PMC_ENABLE) = 0xFFFFFFFF;
  VIDEOREG(NV_PMC_ENABLE) = 0xFFFFFFFF;
  VIDEOREG(NV_PMC_ENABLE) = 0x0;
  VIDEOREG(NV_PMC_ENABLE) = 0x1000;

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kStopBehavior);

  Sleep(2000);

  PvideoTeardown();
}

void PvideoTests::TestSizeInMaxUnityDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Displaying video overlay...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFFFFFF00, 0xFF333333, 16, 0, 0, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());
  for (uint32_t i = 0; i < 30; ++i) {
    DrawFullscreenOverlay();
    Sleep(33);
  }

  DbgPrint("Setting size_in to 0xFFFFFFFF, out_size to fullscreen, dI/dO unity...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFF113311, 0xFF33FF33, 8);
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
    VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;
    SetDsDx(host_.GetFramebufferWidth(), host_.GetFramebufferWidth());
    SetDtDy(host_.GetFramebufferHeight(), host_.GetFramebufferHeight());
    SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kSizeInMaxUnity);
}

void PvideoTests::TestSizeInMaxLargeDelta() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Displaying video overlay...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFFFFFF00, 0xFF333333, 16, 0, 0, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());
  for (uint32_t i = 0; i < 30; ++i) {
    DrawFullscreenOverlay();
    Sleep(33);
  }

  DbgPrint("Setting size_in to 0xFFFFFFFF, out_size to fullscreen, dI/dO implies larger...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFF113311, 0xFF88FF33, 8);
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
    VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;
    SetDsDx(host_.GetFramebufferWidth() << 1, host_.GetFramebufferWidth());
    SetDtDy(host_.GetFramebufferHeight() << 1, host_.GetFramebufferHeight());
    SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kSizeInMaxLarge);
}

void PvideoTests::TestSizeInMaxSmallDelta() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Displaying video overlay...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFFFFFF00, 0xFF333333, 16, 0, 0, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());
  for (uint32_t i = 0; i < 30; ++i) {
    DrawFullscreenOverlay();
    Sleep(33);
  }

  DbgPrint("Setting size_in to 0xFFFFFFFF, out_size to fullscreen, dI/dO implies smaller...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFF113311, 0xFF33FF88, 8);
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
    VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;
    SetDsDx(host_.GetFramebufferWidth() >> 2, host_.GetFramebufferWidth());
    SetDtDy(host_.GetFramebufferHeight() >> 2, host_.GetFramebufferHeight());
    SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kSizeInMaxSmall);
}

void PvideoTests::TestSizeMaxOutSmallUnityDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 0xFFFFFFFF, out_size to subscreen, dI/dO unity...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFF111133, 0xFF3333FF, 8);
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
    VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;
    SetDsDx(host_.GetFramebufferWidth(), host_.GetFramebufferWidth());
    SetDtDy(host_.GetFramebufferHeight(), host_.GetFramebufferHeight());
    SetPvideoOut(10, 10, 128, 64);
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kSizeInMaxOutSmallUnity);
}

void PvideoTests::TestSizeMaxOutSmallCorrectDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 0xFFFFFFFF, out_size to subscreen, dI/dO correct...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFF111133, 0xFF3377FF, 8);
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
    VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;
    SetDsDx(host_.GetFramebufferWidth(), 128);
    SetDtDy(host_.GetFramebufferHeight(), 64);
    SetPvideoOut(10, 10, 128, 64);
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kSizeInMaxOutSmallCorrect);
}

void PvideoTests::TestSizeInLargerThanSizeOutUnityDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 2x size_out, dI/dO unity...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFF113311, 0xFF33FF33, 8);
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    SetPvideoIn(0, 0, 256, 128);
    SetDsDx(host_.GetFramebufferWidth(), host_.GetFramebufferWidth());
    SetDtDy(host_.GetFramebufferHeight(), host_.GetFramebufferHeight());
    SetPvideoOut(64, 64, 128, 64);
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kSizeInLargerThanSizeOutUnity);
}

void PvideoTests::TestSizeInLargerThanSizeOutCorrectDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 2x size_out, dI/dO correct...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFF113311, 0xFF33FF33, 8);
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    SetPvideoIn(0, 0, 256, 128);
    SetDsDx(256, 128);
    SetDtDy(128, 64);
    SetPvideoOut(64, 64, 128, 64);
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kSizeInLargerThanSizeOutCorrect);
}

void PvideoTests::TestPALIntoNTSC() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in PAL rendering to NTSC, dI/dO correct...\n");
  SetTestPatternVideoFrameCR8YB8CB8YA8();
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    SetPvideoIn(0, 0, 720, 576);
    SetDsDx(720, 640);
    SetDtDy(576, 480);
    SetPvideoOut(0, 0, 640, 480);
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kSizeInLargerThanSizeOutCorrect);
}

void PvideoTests::TestSizeInSmallerThanSizeOutUnityDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 2x size_out, dI/dO unity...\n");
  memset(video_, 0xFF, host_.GetFramebufferWidth() * 2 * host_.GetFramebufferHeight());
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFF666600, 0xFF33337F, 8, 0, 0, 128, 64);
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    SetPvideoIn(0, 0, 128, 64);
    SetDsDx(11, 11);
    SetDtDy(11, 11);
    SetPvideoOut(0, 0, 256, 128);
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kSizeInSmallerThanSizeOutUnity);
}

void PvideoTests::TestSizeInSmallerThanSizeOutCorrectDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 2x size_out, dI/dO correct...\n");
  memset(video_, 0x7F, host_.GetFramebufferWidth() * 2 * host_.GetFramebufferHeight());
  SetCheckerboardVideoFrameCR8YB8CB8YA8(0xFF666600, 0xFF7F3333, 8, 0, 0, 128, 64);
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    SetPvideoIn(0, 0, 128, 64);
    SetDsDx(128, 256);
    SetDtDy(64, 128);
    SetPvideoOut(256, 128, 256, 128);
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kSizeInSmallerThanSizeOutCorrect);
}

void PvideoTests::TestPitchLessThanCompact() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting pitch < width * 2...\n");
  SetTestPatternVideoFrameCR8YB8CB8YA8();
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    SetPvideoIn(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    VIDEOREG(NV_PVIDEO_DS_DX) = 0x00100000;
    VIDEOREG(NV_PVIDEO_DT_DY) = 0x00100000;
    SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

    // Set the pitch to the width (1/2 of a YUYV line, which is 2bpp).
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth(), false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kPitchLessThanCompact);
}

void PvideoTests::TestPitchLargerThanCompact() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting pitch > width * 2 (likely would crash if limit did not allow reads past the buffer)...\n");
  SetTestPatternVideoFrameCR8YB8CB8YA8();
  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoStop();
    SetPvideoColorKey(0, 0, 0, 0);
    SetPvideoOffset(VRAM_ADDR(video_));
    SetPvideoIn(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    VIDEOREG(NV_PVIDEO_DS_DX) = 0x00100000;
    VIDEOREG(NV_PVIDEO_DT_DY) = 0x00100000;
    SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 4, false);
    SetPvideoLimit(VRAM_MAX);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, kPitchLargerThanCompact);
}

void PvideoTests::DrawFullscreenOverlay() {
  SetPvideoStop();
  SetPvideoColorKey(0, 0, 0, 0);
  SetPvideoOffset(VRAM_ADDR(video_));
  SetPvideoIn(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  VIDEOREG(NV_PVIDEO_DS_DX) = 0x00100000;
  VIDEOREG(NV_PVIDEO_DT_DY) = 0x00100000;
  SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);

  // Dragon Ball Z Sagas just sets the limit to the maximum possible, it should be possible to set it to the actual
  // calculated limit as well.
  SetPvideoLimit(VRAM_MAX);
  //  uint32_t end = VRAM_ADDR(video_) + 2 * host_.GetFramebufferWidth() * host_.GetFramebufferHeight();
  //  SetPvideoLimit(end);

  SetPvideoInterruptEnabled(true, false);
  SetPvideoBuffer(true, false);
}
