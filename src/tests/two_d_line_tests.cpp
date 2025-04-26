#include "two_d_line_tests.h"

#include <pbkit/pbkit.h>

#include "../nxdk_ext.h"
#include "../pbkit_ext.h"
#include "../test_host.h"

// 5C is the class for NV04_RENDER_SOLID_LIN (nv32.h) / NV04_SOLID_LINE (nv_objects.h)
static constexpr uint32_t SUBCH_CLASS_5C = kNextSubchannel;
// 42 is the class for NV04_CONTEXT_SURFACES_2D (nv32.h) & (nv_objects.h)
static constexpr uint32_t SUBCH_CLASS_42 = SUBCH_CLASS_5C + 1;

static std::string ColorFormatName(uint32_t format, bool ReturnShortName);

// clang-format off
static constexpr TwoDLineTests::TestCase kTests[] = {
    // values outside of target buffer bounds will result in crash
    {0xFFFFFFFF, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 100, 100, 100, 400},
    {0xFFFFFF, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 222, 222, 222, 222},
    {0x000000, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 400, 200, 400, 400},
    {0xFF0000, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 400, 200, 400, 400},
    {0x00FF00, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 400, 200, 400, 400},
    {0x0007E0, NV05C_SET_COLOR_FORMAT_LE_X16R5G6B5, 400, 200, 400, 400},
    {0x0003E0, NV05C_SET_COLOR_FORMAT_LE_X17R5G5B5, 400, 200, 400, 400},
    {0x00FF00, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 444, 222, 444, 444},
    {0xFFFFFF, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 0, 0, 639, 479},
    {0xFFFFFF, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 639, 479, 0, 0},
    {0xFFFFFF, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 400, 0, 400, 479},
    {0xFFFFFF, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 400, 300, 401, 300},
    {0xFFFFFF, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 400, 300, 400, 301},
    {0xFFFFFF, NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8, 639, 479, 0, 0},
};
// clang-format on

/**
 * Constructs the 2D Line test suite.
 *
 * @tc 2DLine-15-C000003E0-400_200-400_400 XRGB 555 color depth - green vertical line from 400, 200 - 400, 400.
 * @tc 2DLine-16-C000007E0-400_200-400_400 XRGB 565 color depth - green vertical line from 400, 200 - 400, 400.
 * @tc 2DLine-24-C00000000-400_200-400_400 XRGB 888 - black vertical line from 400, 200 - 400, 400
 * @tc 2DLine-24-C0000FF00-400_200-400_400 XRGB 888 - green vertical line from 400, 200 - 400, 400
 * @tc 2DLine-24-C0000FF00-444_222-444_444 XRGB 888 - green vertical line from 444, 222 - 444, 444
 * @tc 2DLine-24-C00FF0000-400_200-400_400 XRGB 888 - red vertical line from 400, 200 - 400, 400
 * @tc 2DLine-24-C00FFFFFF-0_0-639_479 XRGB 888 - white diagonal line from 0, 0, - 639, 479 (one pixel short of bottom
 *    right corner)
 * @tc 2DLine-24-C00FFFFFF-222_222-222_222 XRGB 888 - should display nothing, the line has no length
 * @tc 2DLine-24-C00FFFFFF-400_0-400_479 XRGB 888 - white vertical line from 400, 0 - 400, 479 (one pixel short of
 *    bottom)
 * @tc 2DLine-24-C00FFFFFF-400_300-400_301 XRGB 888 - white dot at 400, 300
 * @tc 2DLine-24-C00FFFFFF-400_300-401_300 XRGB 888 - white dot at 400, 300
 * @tc 2DLine-24-C00FFFFFF-639_479-0_0 XRGB 888 - white diagonal line from 639, 479 - 0, 0 (one pixel short of the upper
 *    left corner)
 * @tc 2DLine-24-CFFFFFFFF-100_100-100_400 XRGB 888 - white vertical line from 100, 100 - 100, 400 - alpha is ignored
 */
TwoDLineTests::TwoDLineTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), kSuiteName, config) {
  for (auto test : kTests) {
    std::string name = MakeTestName(test, false);

    auto test_method = [this, test]() { Test(test); };
    tests_[name] = test_method;
  }
}

void TwoDLineTests::Initialize() {
  TestSuite::Initialize();
  SetDefaultTextureFormat();

  auto channel = kNextContextChannel;

  pb_create_gr_ctx(channel++, NV04_SOLID_LINE, &solid_lin_ctx_);
  pb_bind_channel(&solid_lin_ctx_);
  pb_bind_subchannel(SUBCH_CLASS_5C, &solid_lin_ctx_);

  pb_create_gr_ctx(channel++, NV04_CONTEXT_SURFACES_2D, &surface_destination_ctx_);
  pb_bind_channel(&surface_destination_ctx_);
  pb_bind_subchannel(SUBCH_CLASS_42, &surface_destination_ctx_);

  host_.SetVertexShaderProgram(nullptr);
}

void TwoDLineTests::Test(const TestCase& test) {
  host_.PrepareDraw(0xFF440011);  // alpha + RRGGBB

  Pushbuffer::Begin();

  Pushbuffer::PushTo(SUBCH_CLASS_42, NV04_CONTEXT_SURFACES_2D_SET_DMA_IMAGE_DST, DMA_CHANNEL_PIXEL_RENDERER);
  Pushbuffer::PushTo(SUBCH_CLASS_42, NV042_SET_PITCH, (pb_back_buffer_pitch() << 16) | pb_back_buffer_pitch());
  Pushbuffer::PushTo(SUBCH_CLASS_42, NV042_SET_COLOR_FORMAT, NV042_SET_COLOR_FORMAT_LE_A8R8G8B8);

  Pushbuffer::PushTo(SUBCH_CLASS_5C, NV04_SOLID_LINE_OPERATION, NV09F_SET_OPERATION_SRCCOPY);
  Pushbuffer::PushTo(SUBCH_CLASS_5C, NV04_SOLID_LINE_COLOR_VALUE, test.object_color);
  Pushbuffer::PushTo(SUBCH_CLASS_5C, NV04_SOLID_LINE_SURFACE, surface_destination_ctx_.ChannelID);

  Pushbuffer::PushTo(SUBCH_CLASS_5C, NV04_SOLID_LINE_COLOR_FORMAT, test.color_format);
  Pushbuffer::PushTo(SUBCH_CLASS_5C, NV04_SOLID_LINE_START, (test.start_y << 16) | test.start_x);
  Pushbuffer::PushTo(SUBCH_CLASS_5C, NV04_SOLID_LINE_END, (test.end_y << 16) | test.end_x);

  Pushbuffer::End();

  pb_print("2D Line: (%lu, %lu) - (%lu, %lu)\n", test.start_x, test.start_y, test.end_x, test.end_y);
  std::string color_format_name = ColorFormatName(test.color_format, false);
  pb_print("Color: %s - %08X\n", color_format_name.c_str(), test.object_color);

  pb_draw_text_screen();
  std::string name = MakeTestName(test, true);

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

std::string TwoDLineTests::MakeTestName(const TestCase& test, bool ReturnShortName) {
  char buf[256] = {0};
  snprintf(buf, 255, "2DLine-%s-C%08X-%lu_%lu-%lu_%lu", ColorFormatName(test.color_format, ReturnShortName).c_str(),
           test.object_color, test.start_x, test.start_y, test.end_x, test.end_y);
  return buf;
}

static std::string ColorFormatName(uint32_t format, bool ReturnShortName) {
  switch (format) {
    case NV05C_SET_COLOR_FORMAT_LE_X16R5G6B5:
      if (ReturnShortName) return "16";
      return "R5G6B5";
    case NV05C_SET_COLOR_FORMAT_LE_X17R5G5B5:
      if (ReturnShortName) return "15";
      return "R5G5B5";
    case NV05C_SET_COLOR_FORMAT_LE_X8R8G8B8:
      if (ReturnShortName) return "24";
      return "R8G8B8";
    default:
      break;
  }

  char buf[16] = {0};
  snprintf(buf, 15, "%X", format);
  return buf;
}
