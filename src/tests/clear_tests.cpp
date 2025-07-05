#include "clear_tests.h"

#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "vertex_buffer.h"

// From pbkit.c, DMA_COLOR is set to channel 9 by default
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
const uint32_t kDefaultDMAColorChannel = 9;

static constexpr uint32_t kColorMasks[] = {
    0x00000000,
    NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE,
    NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE,
    NV097_SET_COLOR_MASK_RED_WRITE_ENABLE,
    NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE,
    NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE | NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
        NV097_SET_COLOR_MASK_RED_WRITE_ENABLE | NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE,
};

/**
 * Constructs the test suite and creates test cases.
 *
 * @tc C00000000_Depth_n
 *  Framebuffer result after disabling writing to all color channels and to the
 *  depth/stencil buffer.
 *
 * @tc C00000000_Depth_n_ZB
 *  Z-buffer result after disabling writing to all color channels and to the
 *  depth/stencil buffer.
 *
 * @tc C00000000_Depth_y
 *  Framebuffer result after disabling writing to all color channels and
 *  enabling writes to the depth/stencil buffer.
 *
 * @tc C00000000_Depth_y_ZB
 *  Z-buffer result after disabling writing to all color channels and
 *  enabling writes to the depth/stencil buffer.
 *
 * @tc C00000001_Depth_n
 *  Framebuffer result after disabling writing to all color channels except blue
 *  and to the depth/stencil buffer.
 *
 * @tc C00000001_Depth_n_ZB
 *  Z-buffer result after disabling writing to all color channels except blue
 *  and to the depth/stencil buffer.
 *
 * @tc C00000001_Depth_y
 *  Framebuffer result after disabling writing to all color channels except blue
 *  enabling writes to the depth/stencil buffer.
 *
 * @tc C00000001_Depth_y_ZB
 *  Z-buffer result after disabling writing to all color channels except blue
 *  enabling writes to the depth/stencil buffer.
 *
 * @tc C00000100_Depth_n
 *  Framebuffer result after disabling writing to all color channels except
 *  green and to the depth/stencil buffer.
 *
 * @tc C00000100_Depth_n_ZB
 *  Z-buffer result after disabling writing to all color channels except
 *  green and to the depth/stencil buffer.
 *
 * @tc C00000100_Depth_y
 *  Framebuffer result after disabling writing to all color channels except
 *  green and enabling writes to the depth/stencil buffer.
 *
 * @tc C00000100_Depth_y_ZB
 *  Z-buffer result after disabling writing to all color channels except
 *  green and enabling writes to the depth/stencil buffer.
 *
 * @tc C00010000_Depth_n
 *  Framebuffer result after disabling writing to all color channels except
 *  red and to the depth/stencil buffer.
 *
 * @tc C00010000_Depth_n_ZB
 *  Z-buffer result after disabling writing to all color channels except
 *  red and to the depth/stencil buffer.
 *
 * @tc C00010000_Depth_y
 *  Framebuffer result after disabling writing to all color channels except
 *  red and enabling writes to the depth/stencil buffer.
 *
 * @tc C00010000_Depth_y_ZB
 *  Z-buffer result after disabling writing to all color channels except
 *  red and enabling writes to the depth/stencil buffer.
 *
 * @tc C01000000_Depth_n
 *  Framebuffer result after disabling writing to all color channels except
 *  alpha and to the depth/stencil buffer.
 *
 * @tc C01000000_Depth_n_ZB
 *  Z-buffer result after disabling writing to all color channels except
 *  alpha and to the depth/stencil buffer.
 *
 * @tc C01000000_Depth_y
 *  Framebuffer result after disabling writing to all color channels except
 *  alpha and enabling writes to the depth/stencil buffer.
 *
 * @tc C01000000_Depth_y_ZB
 *  Z-buffer result after disabling writing to all color channels except
 *  alpha and enabling writes to the depth/stencil buffer.
 *
 * @tc C01010101_Depth_n
 *  Framebuffer result after enabling writing to all color channels but
 *  disabling writes to the depth/stencil buffer.
 *
 * @tc C01010101_Depth_n_ZB
 *  Z-buffer result after enabling writing to all color channels but
 *  disabling writes to the depth/stencil buffer.
 *
 * @tc C01010101_Depth_y
 *  Framebuffer result after enabling writing to all color channels and the
 *  depth/stencil buffer.
 *
 * @tc C01010101_Depth_y_ZB
 *  Z-buffer result after enabling writing to all color channels and the
 *  depth/stencil buffer.
 *
 */
ClearTests::ClearTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Clear", config) {
  for (auto color_write : kColorMasks) {
    for (auto depth_write : {true, false}) {
      std::string name = MakeTestName(color_write, depth_write);
      tests_[name] = [this, color_write, depth_write]() { Test(color_write, depth_write); };
    }
  }
}

void ClearTests::Initialize() {
  TestSuite::Initialize();

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  CreateGeometry();

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::End();
}

void ClearTests::CreateGeometry() {
  uint32_t num_quads = 4;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);

  Color ul{1.0, 0.0, 0.0, 1.0};
  Color ll{0.0, 1.0, 0.0, 1.0};
  Color lr{0.0, 0.0, 1.0, 1.0};
  Color ur{0.5, 0.5, 0.5, 1.0};

  auto fb_width = host_.GetFramebufferWidthF();
  auto fb_height = host_.GetFramebufferHeightF();

  float width = floorf(fb_width / (1.0f + 2.0f * static_cast<float>(num_quads)));
  float height = floorf(fb_height / 4.0f);

  float x = width;
  float y = floorf(fb_height * 0.5f) - (height * 0.5f);
  float z = 10.0f;

  for (auto i = 0; i < num_quads; ++i) {
    buffer->DefineBiTri(i, x, y, x + width, y + height, z, z, z, z, ul, ll, lr, ur);
    x += width * 2.0f;
    z -= 0.5f;
  }
}

void ClearTests::Test(uint32_t color_mask, bool depth_write_enable) {
  host_.PrepareDraw();
  host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE);

  host_.SetupControl0(false);
  Pushbuffer::Begin();
  // This is not strictly necessary, but causes xemu to flag the color surface as dirty, exercising xemu bug #730.
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  Pushbuffer::Push(NV097_SET_COLOR_MASK, color_mask);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, depth_write_enable);
  Pushbuffer::Push(NV097_SET_STENCIL_MASK, depth_write_enable);

  Pushbuffer::Push(NV097_SET_COLOR_CLEAR_VALUE, 0x7F7F7F7F);
  Pushbuffer::Push(NV097_CLEAR_SURFACE,
                   NV097_CLEAR_SURFACE_COLOR | NV097_CLEAR_SURFACE_Z | NV097_CLEAR_SURFACE_STENCIL);

  Pushbuffer::Push(NV097_SET_COLOR_MASK,
                   NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE | NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
                       NV097_SET_COLOR_MASK_RED_WRITE_ENABLE | NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_STENCIL_MASK, true);
  Pushbuffer::End();

  pb_print("C: 0x%08X\n", color_mask);
  pb_print("D: %s\n", depth_write_enable ? "Y" : "N");
  pb_printat(13, 0, (char*)"This screen should be grey, with no hint of");
  pb_printat(14, 0, (char*)"the geometry that was rendered.");
  pb_draw_text_screen();

  std::string name = MakeTestName(color_mask, depth_write_enable);
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name, true);
}

std::string ClearTests::MakeTestName(uint32_t color_mask, bool depth_write_enable) {
  char buf[32] = {0};
  snprintf(buf, 31, "C%08X_Depth_%s", color_mask, depth_write_enable ? "y" : "n");
  return buf;
}
