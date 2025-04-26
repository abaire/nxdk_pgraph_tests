#include "depth_format_tests.h"

#include <pbkit/pbkit.h>

#include "../shaders/passthrough_vertex_shader.h"
#include "../test_host.h"
#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "vertex_buffer.h"

static constexpr uint32_t kF16MaxFixedRepresentation = 0x0000FFFF;
static constexpr uint32_t kF24MaxFixedRepresentation = 0x00FEFFFF;

constexpr DepthFormatTests::DepthFormat kDepthFormats[] = {
    {NV097_SET_SURFACE_FORMAT_ZETA_Z16, 0x0000FFFF, false},
    {NV097_SET_SURFACE_FORMAT_ZETA_Z24S8, 0x00FFFFFF, false},
    {NV097_SET_SURFACE_FORMAT_ZETA_Z16, kF16MaxFixedRepresentation, true},
    {NV097_SET_SURFACE_FORMAT_ZETA_Z24S8, kF24MaxFixedRepresentation, true},
};

constexpr uint32_t kNumDepthTests = 48;
constexpr bool kCompressionSettings[] = {false, true};

DepthFormatTests::DepthFormatTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Depth buffer", config) {
  for (auto depth_format : kDepthFormats) {
    uint32_t depth_cutoff_step = depth_format.max_depth / kNumDepthTests;

    for (auto compression_enabled : kCompressionSettings) {
      uint32_t depth_cutoff = depth_format.max_depth;

      for (int i = 0; i <= kNumDepthTests; ++i, depth_cutoff -= depth_cutoff_step) {
        AddTestEntry(depth_format, compression_enabled, depth_cutoff);
      }
    }
  }
}

void DepthFormatTests::Initialize() {
  TestSuite::Initialize();
  SetDefaultTextureFormat();

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);
}

void DepthFormatTests::Test(const DepthFormat &format, bool compress_z, uint32_t depth_cutoff) {
  host_.SetSurfaceFormat(host_.GetColorBufferFormat(), static_cast<TestHost::SurfaceZetaFormat>(format.format),
                         host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetDepthBufferFloatMode(format.floating_point);
  host_.PrepareDraw(0xFF000000, depth_cutoff, 0x00);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, true);
  Pushbuffer::Push(NV097_SET_DEPTH_MASK, true);
  Pushbuffer::Push(NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  Pushbuffer::Push(NV097_SET_COMPRESS_ZBUFFER_EN, compress_z);
  Pushbuffer::Push(NV097_SET_STENCIL_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_STENCIL_MASK, false);
  Pushbuffer::End();

  host_.DrawArrays();

  const char *format_name = format.format == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? "z16" : "z24";
  pb_print("DF: %s\n", format_name);
  pb_print("CompZ: %s\n", compress_z ? "y" : "n");
  pb_print("FloatZ: %s\n", format.floating_point ? "y" : "n");
  pb_print("Max: %x\n", depth_cutoff);

  pb_draw_text_screen();

  std::string name = MakeTestName(format, compress_z, depth_cutoff);
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name, true);
}

void DepthFormatTests::CreateGeometry(const DepthFormat &format) {
  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  // Place the graphic in the center of the framebuffer with some padding on all sides.
  float left = floorf(fb_width / 5.0f);
  float right = left + (fb_width - left * 2.0f);
  float top = floorf(fb_height / 12.0f);
  float bottom = top + (fb_height - top * 2.0f);
  float big_quad_width = right - left;
  float big_quad_height = bottom - top;

  // Place an array of smaller quads with increasing depth inside of the big quad.
  constexpr uint32_t kSmallSize = 8;
  constexpr uint32_t kSmallSpacing = 4;
  constexpr uint32_t kStep = kSmallSize + kSmallSpacing;

  uint32_t quads_per_row = static_cast<uint32_t>(big_quad_width - kSmallSize) / kStep;
  uint32_t quads_per_col = static_cast<uint32_t>(big_quad_height - kSmallSize) / kStep;

  float row_size = static_cast<float>(kSmallSize) + static_cast<float>(quads_per_row) * kStep;
  float col_size = static_cast<float>(kSmallSize) + static_cast<float>(quads_per_col) * kStep;

  float x_offset = left + kSmallSpacing + (big_quad_width - row_size) / 2.0f;
  float y_offset = top + kSmallSpacing + (big_quad_height - col_size) / 2.0f;

  uint32_t num_quads = 3 + quads_per_row * quads_per_col;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);

  uint32_t idx = 0;
  auto max_depth_float = static_cast<float>(format.max_depth);
  Color ul{};
  Color ll{};
  Color lr{};
  Color ur{};

  // Generate a grid of small quads going from min depth to ~max depth. The right size will alternate from being deeper
  // or less deep than the left side.
  // Quads are intentionally rendered from front to back to verify the behavior of the depth buffer.
  float z_inc = max_depth_float / (static_cast<float>(num_quads) + 1.0f);
  float z_left = 0.0f;
  float right_offset = z_inc * 0.75f;
  float y = y_offset;

  for (int y_idx = 0; y_idx < quads_per_col; ++y_idx) {
    float x = x_offset;
    for (int x_idx = 0; x_idx < quads_per_row; ++x_idx) {
      float z_right = z_left + right_offset;
      float left_color = 0.25f + (z_left / max_depth_float * 0.75f);
      float right_color = 0.25f + (z_right / max_depth_float * 0.75f);
      ul.SetGrey(left_color);
      ll.SetGrey(left_color);
      lr.SetGrey(right_color);
      ur.SetGrey(right_color);

      float lz = format.fixed_to_float(static_cast<uint32_t>(z_left));
      float rz = format.fixed_to_float(static_cast<uint32_t>(std::max(0.0f, z_right)));
      buffer->DefineBiTri(idx++, x, y, x + kSmallSize, y + kSmallSize, lz, lz, rz, rz, ul, ll, lr, ur);

      z_left += z_inc;
      right_offset *= -1.0f;
      x += kStep;
    }

    y += kStep;
  }

  // Add a small quad on the bottom going from 3/4 max depth on the left to min depth at the right.
  {
    uint32_t back_z = format.max_depth - 1;
    float left_z = format.fixed_to_float(back_z * 3 / 4);
    float right_z = format.fixed_to_float(0);

    ul.SetRGB(left_z / max_depth_float, 0.75, left_z / max_depth_float);
    ll.SetRGB(left_z / max_depth_float, 0.75, left_z / max_depth_float);
    ur.SetRGB(0.75, right_z / max_depth_float, right_z / max_depth_float);
    lr.SetRGB(0.75, right_z / max_depth_float, right_z / max_depth_float);

    PrintMsg("Bottom quad: %g -> %g\n", left_z, right_z);
    buffer->DefineBiTri(idx++, left + 4, bottom - 10, right - 20, bottom - 5, left_z, left_z, right_z, right_z, ul, ll,
                        lr, ur);
  }

  // Add a small quad on the right going from near min depth at the top to 1/2 max depth at the bottom.
  {
    float top_z = format.fixed_to_float(1);
    float bottom_z = format.fixed_to_float(format.max_depth / 2);

    ul.SetRGB(top_z / max_depth_float, 0.5, top_z / max_depth_float);
    ur.SetRGB(top_z / max_depth_float, 0.5, top_z / max_depth_float);
    ll.SetRGB(0.5, bottom_z / max_depth_float, bottom_z / max_depth_float);
    lr.SetRGB(0.5, bottom_z / max_depth_float, bottom_z / max_depth_float);

    PrintMsg("Right quad: %g -> %g\n", top_z, bottom_z);
    buffer->DefineBiTri(idx++, right - 10, top + 5, right - 2, bottom - 5, top_z, bottom_z, bottom_z, top_z, ul, ll, lr,
                        ur);
  }

  // Add a large quad going from 1/3 max depth at the top to max depth at the bottom.
  {
    ul.SetRGB(0.0, 1.0, 0.0);
    ll.SetRGB(0.0, 0.0, 1.0);
    lr.SetRGB(0.4, 0.3, 0.0);
    ur.SetRGB(1.0, 0.0, 0.0);

    uint32_t back_z = format.max_depth - 1;
    float bottom_z = format.fixed_to_float(back_z);
    float top_z = format.fixed_to_float(back_z * 2 / 3);
    PrintMsg("Big quad: %g -> %g\n", top_z, bottom_z);
    buffer->DefineBiTri(idx++, left, top, right, bottom, top_z, bottom_z, bottom_z, top_z, ul, ll, lr, ur);
  }
}

std::string DepthFormatTests::MakeTestName(const DepthFormat &format, bool compress_z, uint32_t depth_cutoff) {
  const char *format_name = format.format == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? "z16" : "z24";
  char buf[128] = {0};
  snprintf(buf, 127, "DepthFmt_%s_C%s_FZ%s_M%06.6x", format_name, compress_z ? "y" : "n",
           format.floating_point ? "y" : "n", depth_cutoff);
  return buf;
}

void DepthFormatTests::AddTestEntry(const DepthFormatTests::DepthFormat &format, bool compress_z,
                                    uint32_t depth_cutoff) {
  std::string name = MakeTestName(format, compress_z, depth_cutoff);

  auto test = [this, format, compress_z, depth_cutoff]() {
    CreateGeometry(format);
    Test(format, compress_z, depth_cutoff);
  };

  tests_[name] = test;
}

float DepthFormatTests::DepthFormat::fixed_to_float(uint32_t val) const {
  if (!floating_point) {
    return static_cast<float>(val);
  }

  if (format == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    return z16_to_float(val);
  }

  return z24_to_float(val);
}
