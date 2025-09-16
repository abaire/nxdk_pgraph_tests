#include "pvideo_tests.h"

// Note: pbkit eats any PVIDEO interrupts in `DPC`, so they cannot be used to feed the buffer.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#include <windows.h>
#pragma clang diagnostic pop

#include "debug_output.h"
#include "pvideo_control.h"
#include "shaders/passthrough_vertex_shader.h"
#include "texture_generator.h"
#include "xbox-swizzle/swizzle.h"

using namespace PvideoControl;

static constexpr const char kStopBehaviorTest[] = "Stop";
// static constexpr const char kAlternateStop[] = "Stop Alt";

static constexpr const char kSizeInMaxUnityTest[] = "Size In Max - dI/dO = 1";
static constexpr const char kSizeInMaxLargeTest[] = "Size In Max - dI/dO > 1";
static constexpr const char kSizeInMaxSmallTest[] = "Size In Max - dI/dO < 1";
static constexpr const char kSizeInMaxOutSmallUnityTest[] = "Size In Max Out Small - dI/dO = 1";
static constexpr const char kSizeInMaxOutSmallCorrectTest[] = "Size In Max Out Small - dI/d0 Correct";
static constexpr const char kSizeInLargerThanSizeOutUnityTest[] = "Size In larger than out - dI/dO = 1";
static constexpr const char kSizeInLargerThanSizeOutCorrectTest[] = "Size In larger than out - dI/d0 Correct";
static constexpr const char kSizeInSmallerThanSizeOutUnityTest[] = "Size In smaller than out - dI/dO = 1";
static constexpr const char kSizeInSmallerThanSizeOutCorrectTest[] = "Size In smaller than out - dI/d0 Correct";
static constexpr const char kPitchTest[] = "Pitch";
static constexpr const char kPitchLessThanCompactTest[] = "Pitch less than compact";
static constexpr const char kPitchLargerThanCompactTest[] = "Pitch larger than compact";
static constexpr const char kPALIntoNTSCTest[] = "PAL into NTSC overlay";

static constexpr const char kOverlay0Test[] = "Overlay0";
static constexpr const char kOverlay1Test[] = "Overlay1";
static constexpr const char kOverlappedOverlaysTest[] = "Overlapped overlays";
static constexpr const char kColorKeyTest[] = "Color key";
static constexpr const char kInPointTest[] = "In point";
static constexpr const char kInSizeTest[] = "In size";
static constexpr const char kOutPointTest[] = "Out point";
static constexpr const char kOutSizeTest[] = "Out size";
static constexpr const char kRatioTest[] = "Ratio";

PvideoTests::PvideoTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "PVIDEO", config) {
  tests_[kPALIntoNTSCTest] = [this]() { TestPALIntoNTSC(); };
  tests_[kStopBehaviorTest] = [this]() { TestStopBehavior(); };
  // This seems to permanently kill video output on 1.0 devkit.
  //  tests_[kAlternateStop] = [this]() { TestAlternateStopBehavior(); };

  tests_[kSizeInMaxUnityTest] = [this]() { TestSizeInMaxUnityDeltas(); };
  tests_[kSizeInMaxLargeTest] = [this]() { TestSizeInMaxLargeDelta(); };
  tests_[kSizeInMaxSmallTest] = [this]() { TestSizeInMaxSmallDelta(); };

  tests_[kSizeInMaxOutSmallUnityTest] = [this]() { TestSizeMaxOutSmallUnityDeltas(); };
  tests_[kSizeInMaxOutSmallCorrectTest] = [this]() { TestSizeMaxOutSmallCorrectDeltas(); };

  tests_[kSizeInLargerThanSizeOutUnityTest] = [this]() { TestSizeInLargerThanSizeOutUnityDeltas(); };
  tests_[kSizeInLargerThanSizeOutCorrectTest] = [this]() { TestSizeInLargerThanSizeOutCorrectDeltas(); };

  tests_[kSizeInSmallerThanSizeOutUnityTest] = [this]() { TestSizeInSmallerThanSizeOutUnityDeltas(); };
  tests_[kSizeInSmallerThanSizeOutCorrectTest] = [this]() { TestSizeInSmallerThanSizeOutCorrectDeltas(); };

  tests_[kPitchTest] = [this]() { TestPitch(); };
  tests_[kPitchLessThanCompactTest] = [this]() { TestPitchLessThanCompact(); };
  tests_[kPitchLargerThanCompactTest] = [this]() { TestPitchLargerThanCompact(); };

  tests_[kColorKeyTest] = [this]() { TestColorKey(); };

  tests_[kOverlay0Test] = [this]() { TestSimpleFullscreenOverlay0(); };
  tests_[kOverlay1Test] = [this]() { TestOverlay1(); };
  tests_[kOverlappedOverlaysTest] = [this]() { TestOverlappedOverlays(); };
  tests_[kInPointTest] = [this]() { TestInPoint(); };
  tests_[kInSizeTest] = [this]() { TestInSize(); };
  tests_[kOutPointTest] = [this]() { TestOutPoint(); };
  tests_[kOutSizeTest] = [this]() { TestOutSize(); };
  tests_[kRatioTest] = [this]() { TestRatios(); };
}

void PvideoTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  // Twice the actual space needed is allocated to facilitate tests with pitch > compact.
  const uint32_t size = host_.GetFramebufferWidth() * 4 * host_.GetFramebufferHeight();
  video_ = (uint8_t *)MmAllocateContiguousMemory(size);
  memset(video_, 0x7F, size);

  video2_ = (uint8_t *)MmAllocateContiguousMemory(size);
  memset(video2_, 0x7F, size);
}

void PvideoTests::Deinitialize() {
  TestSuite::Deinitialize();
  if (video_) {
    MmFreeContiguousMemory(video_);
    video_ = nullptr;
  }
  if (video2_) {
    MmFreeContiguousMemory(video2_);
    video2_ = nullptr;
  }
}

static void SetVideoFrameCR8YB8CB8YA8(uint8_t *dest, const void *pixels, uint32_t width, uint32_t height) {
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

static void DrawBorder(uint8_t *target, uint32_t width, uint32_t height, uint32_t color = 0xFFFFFFFF) {
  auto *pixel = reinterpret_cast<uint32_t *>(target);

  const auto bottom_row_offset = (height - 1) * width;
  for (auto x = 0; x < width; ++x) {
    *pixel = color;
    *(pixel + bottom_row_offset) = color;
  }

  pixel = reinterpret_cast<uint32_t *>(target);
  for (auto y = 0; y < height; ++y) {
    *pixel = color;
    *(pixel + width - 1) = color;
    pixel += width;
  }
}

static void SetCheckerboardVideoFrameCR8YB8CB8YA8(uint8_t *target, uint32_t first_color, uint32_t second_color,
                                                  uint32_t checker_size, uint32_t width, uint32_t height,
                                                  uint32_t x_offset = 0, uint32_t y_offset = 0) {
  const uint32_t pitch = width * 4;
  auto *temp = new uint8_t[pitch * height];
  GenerateRGBACheckerboard(temp, x_offset, y_offset, width, height, pitch, first_color, second_color, checker_size);
  SetVideoFrameCR8YB8CB8YA8(target, temp, width, height);
  delete[] temp;
}

static void SetTestPatternVideoFrameCR8YB8CB8YA8(uint8_t *target, uint32_t width, uint32_t height,
                                                 uint32_t column_interval = 0) {
  const uint32_t pitch = width * 4;
  auto *temp = new uint8_t[pitch * height];
  GenerateRGBATestPattern(temp, width, height);

  if (column_interval) {
    for (uint32_t x = 0; x < width; x += column_interval) {
      auto *pixel = reinterpret_cast<uint32_t *>(temp + (x * 4));
      for (auto y = 0; y < height; ++y) {
        *pixel = 0xFFFFFFFF;
        pixel += width;
      }
    }
  }

  SetVideoFrameCR8YB8CB8YA8(target, temp, width, height);
  delete[] temp;
}

static void SetStepPatternVideoFrameCR8YB8CB8YA8(uint8_t *target, uint32_t width, uint32_t height,
                                                 uint32_t line_spacing = 16, uint32_t background_color = 0,
                                                 bool draw_border = false) {
  const uint32_t pitch = width * 4;
  auto *temp = new uint8_t[pitch * height];
  GenerateRGBDiagonalLinePattern(temp, width, height, line_spacing, background_color);

  if (draw_border) {
    DrawBorder(temp, width, height);
  }

  SetVideoFrameCR8YB8CB8YA8(target, temp, width, height);
  delete[] temp;
}

static void SetLadderPatternVideoFrameCR8YB8CB8YA8(uint8_t *target, uint32_t width, uint32_t height,
                                                   uint32_t column_interval = 16) {
  const uint32_t pitch = width * 4;
  auto *temp = new uint8_t[pitch * height];
  auto *pixel = reinterpret_cast<uint32_t *>(temp);

  const uint32_t kColors[] = {
      0xFFFF0000, 0xFF000000, 0xFF00FF00, 0xFF000000, 0xFF0000FF, 0xFF000000, 0xFF007F7F, 0xFF000000,
      0xFFFF00FF, 0xFF000000, 0xFFFFFF00, 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFF7F007F, 0xFF000000,
  };

  for (auto y = 0; y < height; ++y) {
    uint32_t color = kColors[y % (sizeof(kColors) / sizeof(kColors[0]))];

    for (auto x = 0; x < width; ++x) {
      if (column_interval && (x % column_interval) == 0) {
        *pixel++ = 0xFFFFFFFF;
      } else {
        *pixel++ = color;
      }
    }
  }

  SetVideoFrameCR8YB8CB8YA8(target, temp, width, height);
  delete[] temp;
}

void PvideoTests::TestStopBehavior() {
  host_.PrepareDraw(0xFF250535);

  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF00FFFF, 0xFF000000, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

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

  host_.FinishDraw(false, output_dir_, suite_name_, kStopBehaviorTest);
}

void PvideoTests::TestAlternateStopBehavior() {
  host_.PrepareDraw(0xFF250535);

  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF00FFFF, 0xFF000000, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());
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
  SetSquareDsDxDtDy();
  VIDEOREG(NV_PVIDEO_POINT_IN) = 0;
  VIDEOREG(NV_PVIDEO_SIZE_IN) = 0xFFFFFFFF;
  SetPvideoLuminanceChrominance();
  SetSquareDsDxDtDy();
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

  host_.FinishDraw(false, output_dir_, suite_name_, kStopBehaviorTest);

  Sleep(2000);

  PvideoTeardown();
}

void PvideoTests::TestSizeInMaxUnityDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Displaying video overlay...\n");

  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFFFFFF00, 0xFF333333, 16, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());
  for (uint32_t i = 0; i < 30; ++i) {
    DrawFullscreenOverlay();
    Sleep(33);
  }

  DbgPrint("Setting size_in to 0xFFFFFFFF, out_size to fullscreen, dI/dO unity...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF113311, 0xFF33FF33, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

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

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kSizeInMaxUnityTest);
}

void PvideoTests::TestSizeInMaxLargeDelta() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Displaying video overlay...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFFFFFF00, 0xFF333333, 16, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());
  for (uint32_t i = 0; i < 30; ++i) {
    DrawFullscreenOverlay();
    Sleep(33);
  }

  DbgPrint("Setting size_in to 0xFFFFFFFF, out_size to fullscreen, dI/dO implies larger...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF113311, 0xFF88FF33, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

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

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kSizeInMaxLargeTest);
}

void PvideoTests::TestSizeInMaxSmallDelta() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Displaying video overlay...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFFFFFF00, 0xFF333333, 16, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());
  for (uint32_t i = 0; i < 30; ++i) {
    DrawFullscreenOverlay();
    Sleep(33);
  }

  DbgPrint("Setting size_in to 0xFFFFFFFF, out_size to fullscreen, dI/dO implies smaller...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF113311, 0xFF33FF88, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

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

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kSizeInMaxSmallTest);
}

void PvideoTests::TestSizeMaxOutSmallUnityDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 0xFFFFFFFF, out_size to subscreen, dI/dO unity...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF111133, 0xFF3333FF, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

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

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kSizeInMaxOutSmallUnityTest);
}

void PvideoTests::TestSizeMaxOutSmallCorrectDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 0xFFFFFFFF, out_size to subscreen, dI/dO correct...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF111133, 0xFF3377FF, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

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

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kSizeInMaxOutSmallCorrectTest);
}

void PvideoTests::TestSizeInLargerThanSizeOutUnityDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 2x size_out, dI/dO unity...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF113311, 0xFF33FF33, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

  SetPvideoStop();
  SetPvideoColorKey(0, 0, 0, 0);
  SetPvideoOffset(VRAM_ADDR(video_));
  SetPvideoIn(0, 0, 256, 128);
  SetDsDx(host_.GetFramebufferWidth(), host_.GetFramebufferWidth());
  SetDtDy(host_.GetFramebufferHeight(), host_.GetFramebufferHeight());
  SetPvideoOut(64, 64, 128, 64);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
  SetPvideoLimit(VRAM_MAX);

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kSizeInLargerThanSizeOutUnityTest);
}

void PvideoTests::TestSizeInLargerThanSizeOutCorrectDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 2x size_out, dI/dO correct...\n");
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF113311, 0xFF33FF33, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

  SetPvideoStop();
  SetPvideoColorKey(0, 0, 0, 0);
  SetPvideoOffset(VRAM_ADDR(video_));
  SetPvideoIn(0, 0, 256, 128);
  SetDsDx(256, 128);
  SetDtDy(128, 64);
  SetPvideoOut(64, 64, 128, 64);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
  SetPvideoLimit(VRAM_MAX);

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kSizeInLargerThanSizeOutCorrectTest);
}

void PvideoTests::TestPALIntoNTSC() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in PAL rendering to NTSC, dI/dO correct...\n");
  SetTestPatternVideoFrameCR8YB8CB8YA8(video_, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  SetPvideoStop();
  SetPvideoColorKey(0, 0, 0, 0);
  SetPvideoOffset(VRAM_ADDR(video_));
  SetPvideoIn(0, 0, 720, 576);
  SetDsDx(720, 640);
  SetDtDy(576, 480);
  SetPvideoOut(0, 0, 640, 480);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
  SetPvideoLimit(VRAM_MAX);

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kSizeInLargerThanSizeOutCorrectTest);
}

void PvideoTests::TestSizeInSmallerThanSizeOutUnityDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 2x size_out, dI/dO unity...\n");
  memset(video_, 0xFF, host_.GetFramebufferWidth() * 2 * host_.GetFramebufferHeight());
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF666600, 0xFF33337F, 8, 128, 64);

  SetPvideoStop();
  SetPvideoColorKey(0, 0, 0, 0);
  SetPvideoOffset(VRAM_ADDR(video_));
  SetPvideoIn(0, 0, 128, 64);
  SetDsDx(11, 11);
  SetDtDy(11, 11);
  SetPvideoOut(0, 0, 256, 128);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
  SetPvideoLimit(VRAM_MAX);

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kSizeInSmallerThanSizeOutUnityTest);
}

void PvideoTests::TestSizeInSmallerThanSizeOutCorrectDeltas() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting size_in to 2x size_out, dI/dO correct...\n");
  memset(video_, 0x7F, host_.GetFramebufferWidth() * 2 * host_.GetFramebufferHeight());
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF666600, 0xFF7F3333, 8, 128, 64);

  SetPvideoStop();
  SetPvideoColorKey(0, 0, 0, 0);
  SetPvideoOffset(VRAM_ADDR(video_));
  SetPvideoIn(0, 0, 128, 64);
  SetDsDx(128, 256);
  SetDtDy(64, 128);
  SetPvideoOut(256, 128, 256, 128);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false);
  SetPvideoLimit(VRAM_MAX);

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kSizeInSmallerThanSizeOutCorrectTest);
}

void PvideoTests::TestPitchLessThanCompact() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting pitch < width * 2...\n");
  SetTestPatternVideoFrameCR8YB8CB8YA8(video_, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  SetPvideoStop();
  SetPvideoColorKey(0, 0, 0, 0);
  SetPvideoOffset(VRAM_ADDR(video_));
  SetPvideoIn(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  SetSquareDsDxDtDy();
  SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  // Set the pitch to the width (1/2 of a YUYV line, which is 2bpp).
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth(), false);
  SetPvideoLimit(VRAM_MAX);

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kPitchLessThanCompactTest);
}

void PvideoTests::TestPitchLargerThanCompact() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  DbgPrint("Setting pitch > width * 2 (likely would crash if limit did not allow reads past the buffer)...\n");
  SetTestPatternVideoFrameCR8YB8CB8YA8(video_, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  SetPvideoStop();
  SetPvideoColorKey(0, 0, 0, 0);
  SetPvideoOffset(VRAM_ADDR(video_));
  SetPvideoIn(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  SetSquareDsDxDtDy();
  SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 4, false);
  SetPvideoLimit(VRAM_MAX);

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);
    Sleep(33);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  pb_print("DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kPitchLargerThanCompactTest);
}

void PvideoTests::TestPitch() {
  host_.SetBlend(false);

  static constexpr auto kBackgroundColor = 0xFF250535;

  PvideoInit();

  static constexpr uint32_t kTestRegion = 128;

  SetLadderPatternVideoFrameCR8YB8CB8YA8(video_, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  SetPvideoStop();
  SetPvideoColorKey(kBackgroundColor);
  SetPvideoOffset(VRAM_ADDR(video_), 0);
  SetSquareDsDxDtDy(0);
  SetPvideoIn(0, 0, kTestRegion / 2, kTestRegion, 0);
  SetPvideoOut((host_.GetFramebufferWidth() - kTestRegion) / 2, (host_.GetFramebufferHeight() - kTestRegion) / 2,
               kTestRegion, kTestRegion, 0);
  SetPvideoLimit(VRAM_MAX, 0);

  const uint32_t pitches[] = {
      kTestRegion, kTestRegion * 2, kTestRegion * 4, host_.GetFramebufferWidth(), host_.GetFramebufferWidth() * 2,
  };

  for (auto pitch : pitches) {
    host_.PrepareDraw(kBackgroundColor);
    pb_erase_text_screen();
    pb_print("Pitch %d\n", pitch);
    pb_print(
        "Source is a ladder with each color spanning a single row\nfollowed by a black row.\nIn and Out size is set to "
        "%d",
        kTestRegion);
    pb_draw_text_screen();
    host_.FinishDraw(false, output_dir_, suite_name_, kPitchTest);

    SetPvideoStop();
    SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, pitch, true, 0);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);

    Sleep(250);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  host_.PrepareDraw(kBackgroundColor);
  pb_printat(0, 0, "DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kPitchTest);

  host_.SetBlend();
}

static void RenderColorKeyTargetScreen(TestHost &host) {
  static constexpr auto kTop = 64.f;
  static constexpr auto kQuadSize = 148.f;

  static constexpr uint32_t kColors[]{
      0xFFFF0000, 0x7FFF0000, 0x00FF0000, 0xFF000000, 0x80000000, 0x7F000000, 0x01000000, 0x00000000,
  };
  static constexpr uint32_t kNumQuads = sizeof(kColors) / sizeof(kColors[0]);

  const float width = host.GetFramebufferWidthF();
  const float height = host.GetFramebufferWidthF() - kTop;
  const auto quads_per_row = static_cast<uint32_t>(width / kQuadSize);
  const auto rows = kNumQuads / quads_per_row;

  const auto x_start = floorf((width - kQuadSize * static_cast<float>(quads_per_row)) * 0.5f);
  const auto y_start = floorf((height - kQuadSize * static_cast<float>(rows)) * 0.5f);
  static constexpr auto kZ = 1.f;

  auto x = x_start;
  auto y = y_start;
  auto quad = 0;

  auto text_row = 7;
  auto text_column = 3;

  for (auto color : kColors) {
    host.SetDiffuse(color);
    host.Begin(TestHost::PRIMITIVE_QUADS);
    host.SetVertex(x, y, kZ);
    host.SetVertex(x + kQuadSize, y, kZ);
    host.SetVertex(x + kQuadSize, y + kQuadSize, kZ);
    host.SetVertex(x, y + kQuadSize, kZ);
    host.End();

    pb_printat(text_row, text_column, "0x%X", color);

    if (++quad >= quads_per_row) {
      x = x_start;
      y += kQuadSize;
      quad = 0;
      text_row += 6;
      text_column = 3;
    } else {
      x += kQuadSize;
      text_column += 15;
    }
  }
}

void PvideoTests::TestColorKey() {
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  static constexpr uint32_t kFramesPerColorKey = 40;
  static constexpr uint32_t kColorKeys[] = {
      0xFF000000,
      0x030000FF,
  };

  struct SurfaceFormatInfo {
    const char *name;
    TestHost::SurfaceColorFormat format;
  };
  static constexpr SurfaceFormatInfo kSurfaceFormats[] = {
      // TODO: change video mode via XVideoSetMode to test 16bpp formats.
      //      {"Fmt_X1R5G5B5_Z1R5G5B5", TestHost::SCF_X1R5G5B5_Z1R5G5B5},
      //      {"Fmt_X1R5G5B5_O1R5G5B5", TestHost::SCF_X1R5G5B5_O1R5G5B5},
      //      {"Fmt_R5G6B5", TestHost::SCF_R5G6B5},
      {"Fmt_X8R8G8B8_Z8R8G8B8", TestHost::SCF_X8R8G8B8_Z8R8G8B8},
      // TODO: Reenable when xemu#2427 is fixed.
      //      {"Fmt_X8R8G8B8_O8R8G8B8", TestHost::SCF_X8R8G8B8_O8R8G8B8},
      // TODO: Reenable when xemu#2426 is fixed.
      //      {"Fmt_X1A7R8G8B8_Z1A7R8G8B8", TestHost::SCF_X1A7R8G8B8_Z1A7R8G8B8},
      //      {"Fmt_X1A7R8G8B8_O1A7R8G8B8", TestHost::SCF_X1A7R8G8B8_O1A7R8G8B8},
      {"Fmt_A8R8G8B8", TestHost::SCF_A8R8G8B8},
  };

  // Alpha blending is disabled so that the alpha values written into the framebuffer will be dictated by the quads.
  host_.SetBlend(false);

  SetTestPatternVideoFrameCR8YB8CB8YA8(video_, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  for (const auto &surface_format_info : kSurfaceFormats) {
    for (auto color_key : kColorKeys) {
      host_.SetSurfaceFormat(surface_format_info.format, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                             host_.GetFramebufferHeight());
      host_.PrepareDraw(0xFF250530);

      RenderColorKeyTargetScreen(host_);

      pb_printat(1, 0, "Surface format: %s", surface_format_info.name);
      pb_printat(2, 0, "Gradient overlay color key: 0x%X\n", color_key);
      pb_draw_text_screen();

      host_.FinishDraw(false, output_dir_, suite_name_, kColorKeyTest);

      PvideoInit();

      SetPvideoOffset(VRAM_ADDR(video_));
      SetPvideoIn(0, 0, host_.GetFramebufferWidth() / 2, host_.GetFramebufferHeight());
      SetSquareDsDxDtDy();
      SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
      SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, true);
      SetPvideoLimit(VRAM_MAX);
      SetPvideoColorKey(color_key);

      for (uint32_t i = 0; i < kFramesPerColorKey; ++i) {
        SetPvideoStop();

        SetPvideoInterruptEnabled(true, false);
        SetPvideoBuffer(true, false);

        Sleep(33);
      }

      DbgPrint("Stopping video overlay\n");
      PvideoTeardown();
    }
  }

  // Draw results
  pb_erase_text_screen();
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  host_.PrepareDraw(0xFF116611);

  RenderColorKeyTargetScreen(host_);
  pb_draw_text_screen();

  pb_printat(0, 0, "DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kColorKeyTest);

  host_.SetBlend();
}

static inline uint32_t abgr_to_argb(uint32_t color) {
  return (color & 0xFF000000) | (color & 0xFF) << 16 | (color & 0x0000FF00) | ((color >> 16) & 0xFF);
}

void PvideoTests::TestSimpleFullscreenOverlay0() {
  host_.SetBlend(false);

  static constexpr auto kBackgroundColor = 0xFF250535;

  PvideoInit();

  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF666600, 0xFFAAAA55, 160, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

  SetPvideoStop();
  SetPvideoColorKey(kBackgroundColor);
  SetPvideoOffset(VRAM_ADDR(video_), 0);
  SetSquareDsDxDtDy(0);
  SetPvideoIn(0, 0, host_.GetFramebufferWidth() / 2, host_.GetFramebufferHeight(), 0);
  SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(), 0);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, true, 0);
  SetPvideoLimit(VRAM_MAX, 0);

  host_.PrepareDraw(kBackgroundColor);
  pb_erase_text_screen();
  pb_print("PVIDEO overlay 0\n160x160 checkerboard");
  host_.FinishDraw(false, output_dir_, suite_name_, kOverlay0Test);

  SetPvideoBuffer(true, false);

  Sleep(33 * 30);

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  host_.PrepareDraw(kBackgroundColor);
  pb_printat(0, 0, "DONE\n");
  pb_printat(1, 0, "Video was displayed using overlay 0\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kOverlay0Test);

  host_.SetBlend();
}

void PvideoTests::TestOverlay1() {
  host_.SetBlend(false);

  static constexpr auto kBackgroundColor = 0xFF250535;

  PvideoInit();

  SetTestPatternVideoFrameCR8YB8CB8YA8(video2_, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(), 16);

  host_.PrepareDraw(kBackgroundColor);
  pb_erase_text_screen();
  pb_print("PVIDEO overlay 1\nGradient test pattern\n");
  host_.FinishDraw(false, output_dir_, suite_name_, kOverlay1Test);

  SetPvideoStop();
  SetPvideoColorKey(kBackgroundColor);
  SetPvideoOffset(VRAM_ADDR(video2_), 1);
  SetSquareDsDxDtDy(1);
  SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(), 1);
  SetPvideoLimit(VRAM_MAX, 1);

  SetPvideoIn(0, 0, host_.GetFramebufferWidth() / 2, host_.GetFramebufferHeight(), 1);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, true, 1);

  SetPvideoInterruptEnabled(false, true);
  SetPvideoBuffer(false, true);
  Sleep(3000);

  Sleep(33 * 30);

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  host_.PrepareDraw(kBackgroundColor);

  pb_printat(0, 0, "DONE\n");
  pb_printat(1, 0, "Video was displayed using overlay 1\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kOverlay1Test);

  host_.SetBlend();
}

void PvideoTests::TestOverlappedOverlays() {
  host_.PrepareDraw(0xFF250535);

  PvideoInit();

  SetTestPatternVideoFrameCR8YB8CB8YA8(video_, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video2_, 0xFF777733, 0xFF006666, 64, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

  SetPvideoStop();

  SetPvideoOffset(VRAM_ADDR(video_), 0);
  SetPvideoOffset(VRAM_ADDR(video2_), 1);

  SetPvideoIn(0, 0, host_.GetFramebufferWidth() / 2, host_.GetFramebufferHeight(), 0);
  SetPvideoIn(0, 0, host_.GetFramebufferWidth() / 2, host_.GetFramebufferHeight(), 1);

  SetSquareDsDxDtDy(0);
  SetSquareDsDxDtDy(1);

  const auto half_width = host_.GetFramebufferWidth() >> 1;
  SetPvideoOut(0, 64, half_width, host_.GetFramebufferHeight() - 64, 0);
  SetPvideoOut(half_width, 64, half_width, host_.GetFramebufferHeight() - 64, 1);

  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false, 0);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, false, 1);

  SetPvideoLimit(VRAM_MAX, 0);
  SetPvideoLimit(VRAM_MAX, 1);

  SetPvideoInterruptEnabled(false, false);

  for (uint32_t i = 0; i < 30; ++i) {
    SetPvideoBuffer(true, true);
    Sleep(33);
  }

  SetPvideoInterruptEnabled(false, false);

  PvideoTeardown();

  pb_printat(0, 0, "DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kColorKeyTest);
}

void PvideoTests::TestInPoint() {
  host_.SetBlend(false);

  static constexpr auto kBackgroundColor = 0xFF250535;

  PvideoInit();

  static constexpr auto kBoxSize = 8;
  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF666600, 0xFFAAAA55, kBoxSize, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

  SetPvideoStop();
  SetPvideoColorKey(kBackgroundColor);
  SetPvideoOffset(VRAM_ADDR(video_), 0);
  SetSquareDsDxDtDy(0);
  SetPvideoOut(0, 0, host_.GetFramebufferWidth(), host_.GetFramebufferHeight(), 0);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, true, 0);
  SetPvideoLimit(VRAM_MAX, 0);
  SetPvideoBuffer(true, false);

  for (auto s = 0; s < (kBoxSize * 2 << 4); ++s) {
    host_.PrepareDraw(kBackgroundColor);
    pb_erase_text_screen();
    pb_printat(0, 0, "S 8.4 - the texture will move 2 boxes from right to left");
    pb_printat(1, 0, "S: 0x%X (%d) - %d.%d", s, s, s >> 4, s & 0xF);
    pb_draw_text_screen();
    host_.FinishDraw(false, output_dir_, suite_name_, kInPointTest);

    SetPvideoStop();
    SetPvideoIn(s, 0, host_.GetFramebufferWidth() / 2, host_.GetFramebufferHeight(), 0);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);

    Sleep(10);
  }
  Sleep(1500);

  for (auto t = 0; t < (kBoxSize * 2 << 3); ++t) {
    host_.PrepareDraw(kBackgroundColor);
    pb_erase_text_screen();
    pb_printat(0, 0, "T - the texture will move 2 boxes from top to bottom");
    pb_printat(1, 0, "T: 0x%X (%d) - %d.%d", t, t, t >> 3, t & 0x7);
    pb_draw_text_screen();
    host_.FinishDraw(false, output_dir_, suite_name_, kInPointTest);

    SetPvideoStop();
    SetPvideoIn(0, t, host_.GetFramebufferWidth() / 2, host_.GetFramebufferHeight(), 0);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);

    Sleep(10);
  }
  Sleep(1500);

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  host_.PrepareDraw(kBackgroundColor);
  pb_printat(0, 0, "DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kInPointTest);

  host_.SetBlend();
}

void PvideoTests::TestInSize() {
  host_.SetBlend(false);

  static constexpr auto kBackgroundColor = 0xFF250535;

  PvideoInit();

  SetStepPatternVideoFrameCR8YB8CB8YA8(video_, host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  static constexpr uint32_t kTestWidthTexels = 16;

  static constexpr float kMeasurementBoxSize = 8.f;
  static constexpr float kMeasurementStripTop = 64.f;
  static constexpr float kZ = 1.f;

  auto draw_measurement_boxes = [this]() {
    host_.SetDiffuse(0xFFFFFFFF);

    auto x = 0;
    for (auto y = kMeasurementStripTop; y < host_.GetFramebufferHeightF() - kMeasurementBoxSize;
         y += kMeasurementBoxSize) {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetVertex(x, y, kZ);
      host_.SetVertex(x + kMeasurementBoxSize, y, kZ);
      host_.SetVertex(x + kMeasurementBoxSize, y + kMeasurementBoxSize, kZ);
      host_.SetVertex(x, y + kMeasurementBoxSize, kZ);
      host_.End();

      x += kMeasurementBoxSize;
      if (x >= host_.GetFramebufferWidthF() - kMeasurementBoxSize) {
        x = 0.f;
      }
    }
  };

  SetPvideoStop();
  SetPvideoColorKey(kBackgroundColor);
  SetPvideoOffset(VRAM_ADDR(video_), 0);
  SetSquareDsDxDtDy(0);
  // The out target is intentionally twice the actual draw to demonstrate clamping behavior.
  SetPvideoOut(kMeasurementBoxSize, kMeasurementStripTop, kTestWidthTexels * 4, kTestWidthTexels * 2, 0);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, true, 0);
  SetPvideoLimit(VRAM_MAX, 0);
  SetPvideoBuffer(true, false);

  for (auto w = 1; w <= kTestWidthTexels; ++w) {
    host_.PrepareDraw(kBackgroundColor);
    draw_measurement_boxes();
    pb_erase_text_screen();
    pb_printat(0, 0, "W: %d texels (%d pixels)", w, w * 2);
    pb_printat(1, 0, "White squares are %d pixels via 3D for measurement", static_cast<uint32_t>(kMeasurementBoxSize));
    pb_draw_text_screen();
    host_.FinishDraw(false, output_dir_, suite_name_, kInSizeTest);

    SetPvideoStop();
    SetPvideoIn(0, 0, w, host_.GetFramebufferHeight(), 0);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);

    Sleep(250);
  }

  Sleep(1500);

  for (auto h = 1; h <= kTestWidthTexels; ++h) {
    host_.PrepareDraw(kBackgroundColor);
    draw_measurement_boxes();
    pb_erase_text_screen();
    pb_printat(0, 0, "H: %d texels (%d pixels)", h, h);
    pb_printat(1, 0, "White squares are %d pixels via 3D for measurement", static_cast<uint32_t>(kMeasurementBoxSize));
    pb_draw_text_screen();
    host_.FinishDraw(false, output_dir_, suite_name_, kInSizeTest);

    SetPvideoStop();
    SetPvideoIn(0, 0, host_.GetFramebufferWidth() / 2, h, 0);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);

    Sleep(250);
  }

  Sleep(1500);

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  host_.PrepareDraw(kBackgroundColor);
  pb_printat(0, 0, "DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kInSizeTest);

  host_.SetBlend();
}

void PvideoTests::TestOutPoint() {
  host_.SetBlend(false);

  static constexpr auto kBackgroundColor = 0xFF250535;
  static constexpr auto kLeft = 272;
  static constexpr auto kTop = 192;
  static constexpr auto kMaxDelta = 32;
  static constexpr auto kOutSize = 64;

  PvideoInit();

  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF666600, 0xFF333399, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

  SetPvideoStop();
  SetPvideoColorKey(kBackgroundColor);
  SetPvideoOffset(VRAM_ADDR(video_), 0);
  SetPvideoIn(0, 0, host_.GetFramebufferWidth() / 2, host_.GetFramebufferHeight(), 0);
  SetSquareDsDxDtDy(0);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, true, 0);
  SetPvideoLimit(VRAM_MAX, 0);
  SetPvideoBuffer(true, false);

  static constexpr float kZ = 1.f;
  static constexpr float kRight = kLeft + kMaxDelta + kOutSize;
  static constexpr float kBottom = kTop + kMaxDelta + kOutSize;
  auto draw_measurement_bars = [this]() {
    host_.SetDiffuse(0xFF888888);
    host_.Begin(TestHost::PRIMITIVE_QUADS);

    host_.SetVertex(kLeft - 16.f, kTop, kZ);
    host_.SetVertex(kLeft, kTop, kZ);
    host_.SetVertex(kLeft, kBottom, kZ);
    host_.SetVertex(kLeft - 16.f, kBottom, kZ);

    host_.SetVertex(kLeft, kTop - 16.f, kZ);
    host_.SetVertex(kRight, kTop - 16.f, kZ);
    host_.SetVertex(kRight, kTop, kZ);
    host_.SetVertex(kLeft, kTop, kZ);

    host_.End();
  };

  for (auto x = 0; x < kMaxDelta; ++x) {
    host_.PrepareDraw(kBackgroundColor);
    draw_measurement_bars();
    pb_erase_text_screen();
    pb_printat(0, 0, "X: %d", kLeft + x);
    pb_draw_text_screen();
    host_.FinishDraw(false, output_dir_, suite_name_, kInPointTest);

    SetPvideoStop();
    SetPvideoOut(kLeft + x, kTop, kOutSize, kOutSize, 0);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);

    Sleep(10);
  }
  Sleep(1500);

  for (auto y = 0; y < kMaxDelta; ++y) {
    host_.PrepareDraw(kBackgroundColor);
    draw_measurement_bars();
    pb_erase_text_screen();
    pb_printat(0, 0, "Y: %d", kTop + y);
    pb_draw_text_screen();
    host_.FinishDraw(false, output_dir_, suite_name_, kInPointTest);

    SetPvideoStop();
    SetPvideoOut(kLeft, kTop + y, kOutSize, kOutSize, 0);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);

    Sleep(10);
  }
  Sleep(1500);

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  host_.PrepareDraw(kBackgroundColor);
  pb_printat(0, 0, "DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kInPointTest);

  host_.SetBlend();
}

void PvideoTests::TestOutSize() {
  host_.SetBlend(false);

  static constexpr auto kBackgroundColor = 0xFF250535;
  static constexpr auto kMaxDelta = 32;
  static constexpr auto kLeft = 32;
  static constexpr auto kTop = 128;

  static constexpr float kZ = 1.f;
  static constexpr float kRight = kLeft + kMaxDelta;
  static constexpr float kBottom = kTop + kMaxDelta;
  auto draw_measurement_bars = [this]() {
    host_.SetDiffuse(0xFF888888);
    host_.Begin(TestHost::PRIMITIVE_QUADS);

    host_.SetVertex(kLeft - 16.f, kTop, kZ);
    host_.SetVertex(kLeft, kTop, kZ);
    host_.SetVertex(kLeft, kBottom, kZ);
    host_.SetVertex(kLeft - 16.f, kBottom, kZ);

    host_.SetVertex(kLeft, kTop - 16.f, kZ);
    host_.SetVertex(kRight, kTop - 16.f, kZ);
    host_.SetVertex(kRight, kTop, kZ);
    host_.SetVertex(kLeft, kTop, kZ);

    host_.End();
  };

  PvideoInit();

  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFF666600, 0xFF333399, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

  SetPvideoStop();
  SetPvideoColorKey(kBackgroundColor);
  SetPvideoOffset(VRAM_ADDR(video_), 0);
  SetPvideoIn(0, 0, host_.GetFramebufferWidth() / 2, host_.GetFramebufferHeight(), 0);
  SetSquareDsDxDtDy(0);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, true, 0);
  SetPvideoLimit(VRAM_MAX, 0);
  SetPvideoBuffer(true, false);

  for (auto w = 0; w < kMaxDelta; ++w) {
    host_.PrepareDraw(kBackgroundColor);
    draw_measurement_bars();
    pb_erase_text_screen();
    pb_printat(0, 0, "Width: %d", w);
    pb_draw_text_screen();
    host_.FinishDraw(false, output_dir_, suite_name_, kInPointTest);

    SetPvideoStop();
    SetPvideoOut(kLeft, kTop, w, 256, 0);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);

    Sleep(10);
  }
  Sleep(1500);

  for (auto h = 0; h < kMaxDelta; ++h) {
    host_.PrepareDraw(kBackgroundColor);
    draw_measurement_bars();
    pb_erase_text_screen();
    pb_printat(0, 0, "Height: %d", h);
    pb_draw_text_screen();
    host_.FinishDraw(false, output_dir_, suite_name_, kInPointTest);

    SetPvideoStop();
    SetPvideoOut(kLeft, kTop, 256, h, 0);
    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);

    Sleep(10);
  }
  Sleep(1500);

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  host_.PrepareDraw(kBackgroundColor);
  pb_printat(0, 0, "DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kInPointTest);

  host_.SetBlend();
}

void PvideoTests::TestRatios() {
  host_.SetBlend(false);

  static constexpr auto kBackgroundColor = 0xFF250535;

  PvideoInit();

  static constexpr uint32_t kTestRegion = 256;

  SetCheckerboardVideoFrameCR8YB8CB8YA8(video_, 0xFFCCCC33, 0xFF222222, 8, host_.GetFramebufferWidth(),
                                        host_.GetFramebufferHeight());

  SetPvideoStop();
  SetPvideoColorKey(kBackgroundColor);
  SetPvideoOffset(VRAM_ADDR(video_), 0);
  SetPvideoIn(0, 0, kTestRegion / 2, kTestRegion, 0);
  SetPvideoOut((host_.GetFramebufferWidth() - kTestRegion) / 2, (host_.GetFramebufferHeight() - kTestRegion) / 2,
               kTestRegion, kTestRegion, 0);
  SetPvideoFormat(NV_PVIDEO_FORMAT_COLOR_LE_CR8YB8CB8YA8, host_.GetFramebufferWidth() * 2, true, 0);
  SetPvideoLimit(VRAM_MAX, 0);

  static constexpr uint32_t kRatioNumerators[] = {10,  25,  50,  60,  75,  80,  85,  90,
                                                  100, 110, 125, 150, 175, 200, 250, 400};

  SetSquareDsDxDtDy(0);
  for (auto ratio_numerator : kRatioNumerators) {
    host_.PrepareDraw(kBackgroundColor);
    pb_erase_text_screen();
    pb_printat(0, 0, "DsDx: %d : 100", ratio_numerator);
    pb_draw_text_screen();
    host_.FinishDraw(false, output_dir_, suite_name_, kInPointTest);

    SetPvideoStop();

    SetDsDx(ratio_numerator, 100, 0);

    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);

    Sleep(200);
  }

  SetSquareDsDxDtDy(0);
  for (auto ratio_numerator : kRatioNumerators) {
    host_.PrepareDraw(kBackgroundColor);
    pb_erase_text_screen();
    pb_printat(0, 0, "DtDy: %d : 100", ratio_numerator);
    pb_draw_text_screen();
    host_.FinishDraw(false, output_dir_, suite_name_, kInPointTest);

    SetPvideoStop();

    SetDtDy(ratio_numerator, 100, 0);

    SetPvideoInterruptEnabled(true, false);
    SetPvideoBuffer(true, false);

    Sleep(200);
  }

  DbgPrint("Stopping video overlay\n");
  PvideoTeardown();

  host_.PrepareDraw(kBackgroundColor);
  pb_printat(0, 0, "DONE\n");
  pb_draw_text_screen();

  host_.FinishDraw(false, output_dir_, suite_name_, kPitchTest);

  host_.SetBlend();
}

void PvideoTests::DrawFullscreenOverlay() {
  SetPvideoStop();
  SetPvideoColorKey(0, 0, 0, 0);
  SetPvideoOffset(VRAM_ADDR(video_));
  SetPvideoIn(0, 0, host_.GetFramebufferWidth() / 2, host_.GetFramebufferHeight());
  SetSquareDsDxDtDy();
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
