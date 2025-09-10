#include "zpass_pixel_count_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

static constexpr char kTestName[] = "ZPass";

// PBKit initializes channel 8 as the DMA_SEMAPHORE context object.
// https://github.com/XboxDev/nxdk/blob/4171d5bfe5260c0dd2d42f4efeb9ec1d44788867/lib/pbkit/pbkit.c#L2827
const uint32_t kDefaultSemaphoreContextChannel = 8;

// PBKit initializes channel 12 as the DMA_REPORT context object.
// https://github.com/XboxDev/nxdk/blob/4171d5bfe5260c0dd2d42f4efeb9ec1d44788867/lib/pbkit/pbkit.c#L2828
const uint32_t kDefaultReportContextChannel = 12;

// https://github.com/XboxDev/nxdk/blob/4171d5bfe5260c0dd2d42f4efeb9ec1d44788867/lib/pbkit/pbkit.c#L2657
// https://github.com/XboxDev/nxdk/blob/4171d5bfe5260c0dd2d42f4efeb9ec1d44788867/lib/pbkit/nv_objects.h#L862
// https://github.com/XboxDev/nxdk/blob/4171d5bfe5260c0dd2d42f4efeb9ec1d44788867/lib/pbkit/nv_regs.h#L44

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc ZPass
 *   Tests the basic behavior of ZPASS_PIXEL_COUNT reports.
 *   The counter is cleared via NV097_CLEAR_REPORT_VALUE and then four quads are rendered with depth testing enabled.
 *   The second quad is completely obscured by the first. The final quad is halfway offscreen. The expected pixel count
 *   is then compared to the report value. Next, without clearing the report, one final quad is rendered partially on
 *   top of the fourth quad and the report is fetched again, confirming that it now contains the result of the entire
 *   draw.
 */
ZPassPixelCountTests::ZPassPixelCountTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "ZPass pixel count", config) {
  tests_[kTestName] = [this]() { Test(); };
}

void ZPassPixelCountTests::Initialize() {
  TestSuite::Initialize();
  host_.SetVertexShaderProgram(nullptr);

  auto channel = kNextContextChannel;

  static constexpr auto semaphore_object_size = 32;
  semaphore_context_object_ =
      static_cast<uint32_t *>(MmAllocateContiguousMemoryEx(semaphore_object_size, 0, MAXRAM, 0, PAGE_READWRITE));
  memset(semaphore_context_object_, 0, semaphore_object_size);
  pb_create_dma_ctx(channel++, DMA_CLASS_3D, (DWORD)semaphore_context_object_, 0x20, &semaphore_dma_ctx_);
  pb_bind_channel(&semaphore_dma_ctx_);

  static constexpr auto kReportContextArraySize = sizeof(*report_context_object_) * 4;
  report_context_object_ =
      static_cast<ZPassReport *>(MmAllocateContiguousMemoryEx(kReportContextArraySize, 0, MAXRAM, 0, PAGE_READWRITE));
  memset(report_context_object_, 0, kReportContextArraySize);

  pb_create_dma_ctx(channel++, DMA_CLASS_3D, reinterpret_cast<uint32_t>(report_context_object_),
                    kReportContextArraySize, &report_dma_ctx_);
  pb_bind_channel(&report_dma_ctx_);
}

void ZPassPixelCountTests::Deinitialize() {
  TestSuite::Deinitialize();

  if (semaphore_context_object_) {
    MmFreeContiguousMemory(semaphore_context_object_);
    semaphore_context_object_ = nullptr;
  }

  if (report_context_object_) {
    MmFreeContiguousMemory(report_context_object_);
    report_context_object_ = nullptr;
  }
}

void ZPassPixelCountTests::Test() {
  host_.PrepareDraw(0xFF333333);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_SEMAPHORE, semaphore_dma_ctx_.ChannelID);
  // NOTE: This test assumes that SET_SEMAPHORE_OFFSET is 0.
  // TODO: Try to read back the current semaphore offset and set it to 0 if it is not, then restore at end of test.
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_REPORT, report_dma_ctx_.ChannelID);

  Pushbuffer::Push(NV097_CLEAR_REPORT_VALUE, NV097_CLEAR_REPORT_VALUE_TYPE_ZPASS_PIXEL_CNT);
  Pushbuffer::Push(NV097_SET_ZPASS_PIXEL_COUNT_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);

  Pushbuffer::Push(NV097_SET_STENCIL_MASK, false);
  Pushbuffer::End();

  static constexpr auto kQuadSize = 128.f;
  static constexpr auto kFrontX = 32.f;
  static constexpr auto kFrontY = 240.f;
  static constexpr auto kFrontZ = 0.f;
  static constexpr auto kNonOverlapX = kFrontX + kQuadSize * 1.5f;

  // Draw a quad at Z = 0 to prime the depth buffer.
  host_.SetDiffuse(0xFF773377);
  host_.DrawScreenQuad(kFrontX, kFrontY, kFrontX + kQuadSize, kFrontY + kQuadSize, kFrontZ);

  // Draw another quad behind the first one, it should be entirely culled and not affect the pixel count.
  host_.SetDiffuse(0xFFFF0000);
  host_.DrawScreenQuad(kFrontX, kFrontY, kFrontX + kQuadSize, kFrontY + kQuadSize, kFrontZ + 1.f);

  // Draw a quad that is not occluded to establish the baseline count.
  host_.SetDiffuse(0xFF777733);
  host_.DrawScreenQuad(kNonOverlapX, kFrontY, kNonOverlapX + kQuadSize, kFrontY + kQuadSize, kFrontZ);

  // Draw a quad partially offscreen.
  const auto left = host_.GetFramebufferWidthF() - kQuadSize * 0.5f;
  host_.SetDiffuse(0xFFE0E0E0);
  host_.DrawScreenQuad(left, kFrontY, left + kQuadSize, kFrontY + kQuadSize, kFrontZ);

  // The number of pixels that should be written by the test quad.
  static constexpr uint32_t kQuadPixels = kQuadSize * kQuadSize;
  static constexpr uint32_t kHalfQuadPixels = kQuadPixels >> 1;

  // Expect 2 full quads and one half.
  static constexpr uint32_t kExpectedPassedPixels = kQuadPixels * 2 + kHalfQuadPixels;

  // Disable z-pass counting and request the report.
  static constexpr auto kReportOffset = 0;  // DMA context already points directly at the buffer.
  static constexpr auto kSemaphoreReleaseValue = 0xABC;
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_GET_REPORT,
                   SET_MASK(NV097_GET_REPORT_TYPE, NV097_GET_REPORT_TYPE_ZPASS_PIXEL_CNT) | kReportOffset);
  Pushbuffer::End(true);

  // Draw an additional quad and save a second report.
  host_.SetDiffuse(0xFF33CC33);
  host_.DrawScreenQuad(left, kFrontY - kQuadSize * 0.5f, left + kQuadSize, kFrontY + kQuadSize * 0.5f, kFrontZ - 0.1f);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_ZPASS_PIXEL_COUNT_ENABLE, false);
  Pushbuffer::Push(NV097_GET_REPORT, SET_MASK(NV097_GET_REPORT_TYPE, NV097_GET_REPORT_TYPE_ZPASS_PIXEL_CNT) |
                                         kReportOffset + sizeof(ZPassReport));
  Pushbuffer::Push(NV097_BACK_END_WRITE_SEMAPHORE_RELEASE, kSemaphoreReleaseValue);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);

  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_REPORT, kDefaultReportContextChannel);
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_SEMAPHORE, kDefaultSemaphoreContextChannel);
  Pushbuffer::End(true);

  host_.WaitForGPU();
  // TODO: See if there is a better mechanism to delay until the report is fetched without spin locking.
  for (auto i = 0; i < 32 && *semaphore_context_object_ != kSemaphoreReleaseValue; ++i) {
    Sleep(1);
  }

  pb_print("%s\n", kTestName);
  pb_print("\n");
  pb_print("SEMAPHORE at 0x%X = 0x%X [%s]\n", semaphore_context_object_, *semaphore_context_object_,
           *semaphore_context_object_ == kSemaphoreReleaseValue ? "PASS" : "FAIL");

  auto print_report = [this](uint32_t report_index, uint32_t expected_pixels) {
    const auto offset = kReportOffset + report_index * sizeof(ZPassReport);
    const auto &report = report_context_object_[report_index];
    pb_print("\n");
    pb_print("GET_REPORT at 0x%X\n", offset);
    // Timestamp is a 64-bit nanosecond counter. To avoid non-determinism, it is omitted.
    //    pb_print(".timestamp: 0x%llX\n", report.timestamp);
    pb_print(".report: 0x%X (%u) [%s]\n", report.report, report.report,
             (report.report == expected_pixels) ? "PASS" : "FAIL");
    pb_print(".reserved: 0x%X\n", report.reserved);
  };

  print_report(0, kExpectedPassedPixels);
  print_report(1, kExpectedPassedPixels + kHalfQuadPixels);

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestName, true);
}
