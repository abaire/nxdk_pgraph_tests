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

static std::string MakeMaskTestName(uint32_t color_mask, bool depth_write_enable) {
  char buf[32] = {0};
  snprintf(buf, 31, "C%08X_Depth_%s", color_mask, depth_write_enable ? "y" : "n");
  return buf;
}

struct SurfaceFormatTest {
  const char* test_name;
  TestHost::SurfaceColorFormat surface_format;
};
static constexpr SurfaceFormatTest kSurfaceFormatTests[] = {
    {"SFC_A8R8G8B8", TestHost::SurfaceColorFormat::SCF_A8R8G8B8},
    {"SFC_X1R5G5B5_Z1R5G5B5", TestHost::SurfaceColorFormat::SCF_X1R5G5B5_Z1R5G5B5},
    {"SCF_X1R5G5B5_O1R5G5B5", TestHost::SurfaceColorFormat::SCF_X1R5G5B5_O1R5G5B5},
    {"SCF_R5G6B5", TestHost::SurfaceColorFormat::SCF_R5G6B5},
    {"SCF_X8R8G8B8_Z8R8G8B8", TestHost::SurfaceColorFormat::SCF_X8R8G8B8_Z8R8G8B8},
    {"SCF_X8R8G8B8_O8R8G8B8", TestHost::SurfaceColorFormat::SCF_X8R8G8B8_O8R8G8B8},
    {"SCF_X1A7R8G8B8_Z1A7R8G8B8", TestHost::SurfaceColorFormat::SCF_X1A7R8G8B8_Z1A7R8G8B8},
    {"SCF_X1A7R8G8B8_O1A7R8G8B8", TestHost::SurfaceColorFormat::SCF_X1A7R8G8B8_O1A7R8G8B8},
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
 * @tc SFC_A8R8G8B8
 *   Tests the effect of clear color on LE_A8R8G8B8 surface format.
 *
 * @tc SFC_X1R5G5B5_Z1R5G5B5
 *   Tests the effect of clear color on LE_X1R5G5B5_Z1R5G5B5 surface format.
 *   The cleared surface will have its most significant bit set to 0.
 *   (Note that the surface is rendered as an ARGB8 texture)
 *
 * @tc SFC_X1R5G5B5_O1R5G5B5
 *   Tests the effect of clear color on LE_X1R5G5B5_O1R5G5B5 surface format.
 *   The cleared surface will have its most significant bit set to 1.
 *   (Note that the surface is rendered as an ARGB8 texture)
 *
 * @tc SCF_R5G6B5
 *   Tests the effect of clear color on LE_R5G6B5 surface format.
 *   (Note that the surface is rendered as an ARGB8 texture)
 *
 * @tc SCF_X8R8G8B8_Z8R8G8B8
 *   Tests the effect of clear color on LE_X8R8G8B8_Z8R8G8B8 surface format.
 *   The cleared surface will have its most significant byte set to 0.
 *
 * @tc SCF_X8R8G8B8_O8R8G8B8
 *   Tests the effect of clear color on LE_X8R8G8B8_O8R8G8B8 surface format.
 *   The cleared surface will have its most significant byte set to 1.
 *
 * @tc SCF_X1A7R8G8B8_Z1A7R8G8B8
 *   Tests the effect of clear color on LE_X1A7R8G8B8_Z1A7R8G8B8 surface format.
 *   The cleared surface will have its most significant bit set to 0.
 *
 * @tc SCF_X1A7R8G8B8_O1A7R8G8B8
 *   Tests the effect of clear color on LE_X1A7R8G8B8_O1A7R8G8B8 surface format.
 *   The cleared surface will have its most significant bit set to 1.
 */
ClearTests::ClearTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Clear", config) {
  for (auto color_write : kColorMasks) {
    for (auto depth_write : {true, false}) {
      std::string name = MakeMaskTestName(color_write, depth_write);
      tests_[name] = [this, color_write, depth_write]() { TestColorMask(color_write, depth_write); };
    }
  }

  for (auto& test_info : kSurfaceFormatTests) {
    tests_[test_info.test_name] = [this, &test_info]() {
      TestSurfaceFmt(test_info.surface_format, test_info.test_name);
    };
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

void ClearTests::TestColorMask(uint32_t color_mask, bool depth_write_enable) {
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

  std::string name = MakeMaskTestName(color_mask, depth_write_enable);
  FinishDraw(name, true);
}

void ClearTests::TestSurfaceFmt(TestHost::SurfaceColorFormat surface_format, const std::string& test_name) {
  static constexpr uint32_t kTextureSize = 128;
  static constexpr auto kBlackCenterMarkSize = 2.f;

  uint32_t quad_height = kTextureSize;
  auto pitch = TestHost::GetSurfaceColorPitch(surface_format, 1);
  if (pitch == 1) {
    quad_height /= 4;
  } else if (pitch == 2) {
    quad_height /= 2;
  }

  auto clear_texture = [this, surface_format](uint32_t clear_color) {
    auto target_surface = host_.GetTextureMemoryForStage(0);
    host_.RenderToSurfaceStart(target_surface, surface_format, kTextureSize, kTextureSize);

    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_COLOR_CLEAR_VALUE, clear_color);
    Pushbuffer::Push(NV097_CLEAR_SURFACE, NV097_CLEAR_SURFACE_COLOR);
    Pushbuffer::End(true);

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0x00000000);
    const auto half_texture_size = kTextureSize * 0.5f;
    host_.SetVertex(half_texture_size - kBlackCenterMarkSize, half_texture_size - kBlackCenterMarkSize, 1.f);
    host_.SetVertex(half_texture_size + kBlackCenterMarkSize, half_texture_size - kBlackCenterMarkSize, 1.f);
    host_.SetVertex(half_texture_size + kBlackCenterMarkSize, half_texture_size + kBlackCenterMarkSize, 1.f);
    host_.SetVertex(half_texture_size - kBlackCenterMarkSize, half_texture_size + kBlackCenterMarkSize, 1.f);
    host_.End();

    host_.RenderToSurfaceEnd();
  };

  auto draw_quad = [this, quad_height](float left, float top) {
    host_.SetBlend(false);
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);
    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

    auto& texture_stage = host_.GetTextureStage(0);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8B8G8R8));
    texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();

    const float right = left + kTextureSize;
    const float bottom = top + static_cast<float>(quad_height);
    static constexpr float kZ = 0.f;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(left, top, kZ);

    host_.SetTexCoord0(kTextureSize, 0.0f);
    host_.SetVertex(right, top, kZ);

    host_.SetTexCoord0(kTextureSize, static_cast<float>(quad_height));
    host_.SetVertex(right, bottom, kZ);

    host_.SetTexCoord0(0.0f, static_cast<float>(quad_height));
    host_.SetVertex(left, bottom, kZ);
    host_.End();

    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetBlend(true);
  };

  static constexpr uint32_t kClearColors[] = {
      0x00DACABA, 0x011B2B3B, 0x7F3C2C1C, 0x804D5D6D, 0xFE0ECE3E, 0xFF8F9FAF,
  };

  host_.PrepareDraw(0xFF220022);

  static constexpr float kLeftStart = 48.f;
  static constexpr float kQuadSpacing = 16.f;
  auto left = kLeftStart;
  auto top = 128.f;

  pb_print("%s\n", test_name.c_str());

  for (auto clear_color : kClearColors) {
    pb_print("0x%08X, ", clear_color);
    clear_texture(clear_color);
    draw_quad(left, top);

    left += kTextureSize + kQuadSpacing;
    if (left + kTextureSize >= host_.GetFramebufferWidthF()) {
      pb_print("\n");
      left = kLeftStart;
      top += kTextureSize + kQuadSpacing;
    }
  }

  pb_draw_text_screen();
  FinishDraw(test_name);
}
