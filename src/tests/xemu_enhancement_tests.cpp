#include "xemu_enhancement_tests.h"

#include "test_host.h"

static constexpr char kXemuScaledSurfaceUploadFilterTest[] = "XemuScaledSurfaceUploadFilter";

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc XemuScaledSurfaceUploadFilter
 *   Writes 1-pixel-wide alternating R/B vertical stripes to the framebuffer via CPU, then issues a GPU draw to
 *   trigger xemu's internal surface upload (pgraph_vk_upload_surface_data). After GPU completion the framebuffer
 *   is read back via CPU, which triggers xemu's download path. At scale > 1x the NEAREST+NEAREST round-trip
 *   preserves exact R/B values, while any other filtering produces blended intermediate values that fail the
 *   channel-purity check. At scale == 1x no upscale occurs and the test passes trivially.
 */
XemuEnhancementTests::XemuEnhancementTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "xemu enhancement", config) {
  tests_[kXemuScaledSurfaceUploadFilterTest] = [this]() { TestXemuScaledSurfaceUploadFilter(); };
}

void XemuEnhancementTests::Initialize() { TestSuite::Initialize(); }

void XemuEnhancementTests::TestXemuScaledSurfaceUploadFilter() {
  static constexpr uint32_t kTestWidth = 128;
  static constexpr uint32_t kTestHeight = 128;
  static constexpr uint32_t kColorRed = 0x00FF0000;
  static constexpr uint32_t kColorBlue = 0x000000FF;

  host_.PrepareDraw(0xFF111111);

  auto* fb = static_cast<uint32_t*>(pb_agp_access(pb_back_buffer()));
  const uint32_t pitch_pixels = pb_back_buffer_pitch() / 4;
  const uint32_t start_x = (host_.GetFramebufferWidth() - kTestWidth) / 2;
  const uint32_t start_y = (host_.GetFramebufferHeight() - kTestHeight) / 2;

  uint32_t row_offset = start_y * pitch_pixels + start_x;
  for (uint32_t y = 0; y < kTestHeight; ++y, row_offset += pitch_pixels) {
    for (uint32_t x = 0; x < kTestWidth; ++x) {
      fb[row_offset + x] = 0xFF000000 | ((x % 2 == 0) ? kColorRed : kColorBlue);
    }
  }

  // Minimal GPU draw to trigger the surface upload and set draw_dirty.
  // Placed just outside the right edge of the framebuffer.
  host_.Begin(TestHost::PRIMITIVE_POINTS);
  host_.SetScreenVertex(641.f, 0.f, 0.f);
  host_.End();

  host_.PBKitBusyWait();

  uint32_t fail_count = 0;
  uint32_t first_actual = 0;
  uint32_t first_expected = 0;
  static constexpr uint32_t kMaxLoggedFailures = 16;

  row_offset = start_y * pitch_pixels + start_x;
  for (uint32_t y = 0; y < kTestHeight; ++y, row_offset += pitch_pixels) {
    for (uint32_t x = 0; x < kTestWidth; ++x) {
      const uint32_t actual = fb[row_offset + x] & 0x00FFFFFF;
      const bool expect_red = (x % 2 == 0);
      const uint32_t expected = expect_red ? kColorRed : kColorBlue;

      if (actual != expected) {
        if (fail_count < kMaxLoggedFailures) {
          PrintMsg("FAIL [%u,%u]: expected %s, got 0x%06X\n", x, y, expect_red ? "RED" : "BLUE", actual);
        }
        if (fail_count == 0) {
          first_actual = actual;
          first_expected = expected;
        }
        ++fail_count;
      }
    }
  }

  pb_print("%s\n", kXemuScaledSurfaceUploadFilterTest);
  pb_print("1px R/B stripes: CPU write -> GPU draw -> readback\n");
  pb_print("Result: %s\n", fail_count ? "FAIL" : "PASS");
  if (fail_count) {
    pb_print("%u failures, first: exp 0x%06X got 0x%06X\n", fail_count, first_expected, first_actual);
  }
  pb_draw_text_screen();

  FinishDraw(kXemuScaledSurfaceUploadFilterTest);
}
