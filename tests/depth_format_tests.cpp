#include "depth_format_tests.h"

#include <hal/xbox.h>
#include <pbkit/pbkit.h>
#include <windows.h>

#include "../shaders/precalculated_vertex_shader.h"
#include "../test_host.h"
#include "../texture_format.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "vertex_buffer.h"

constexpr DepthFormatTests::DepthFormat kDepthFormats[] = {
    {NV097_SET_SURFACE_FORMAT_ZETA_Z16, 0x0000FFFF},
    {NV097_SET_SURFACE_FORMAT_ZETA_Z24S8, 0x00FFFFFF},
};

constexpr uint32_t kNumDepthFormats = sizeof(kDepthFormats) / sizeof(kDepthFormats[0]);
constexpr uint32_t kNumDepthTests = 32;

DepthFormatTests::DepthFormatTests(TestHost &host, std::string output_dir) : TestBase(host, std::move(output_dir)) {}

void DepthFormatTests::Run() {
  // NV097_SET_TEXTURE_FORMAT_COLOR_SZ_X8R8G8B8
  const TextureFormatInfo &texture_format = kTextureFormats[3];
  host_.SetTextureFormat(texture_format);

  auto shader = std::make_shared<PrecalculatedVertexShader>(false);
  host_.SetShaderProgram(shader);

  constexpr bool kCompressionSettings[] = {false, true};
  constexpr bool kZFloatSettings[] = {true, false};

  for (auto depth_format : kDepthFormats) {
    for (auto z_float_enabled : kZFloatSettings) {
      CreateGeometry(depth_format, z_float_enabled);

      uint32_t depth_cutoff_step = depth_format.max_depth / kNumDepthTests;
      for (auto compression_enabled : kCompressionSettings) {
        uint32_t depth_cutoff = depth_format.max_depth;
        for (int i = 0; i <= kNumDepthTests; ++i, depth_cutoff -= depth_cutoff_step) {
          Test(depth_format, compression_enabled, z_float_enabled, depth_cutoff);
        }

        // This should always render black if the depth test mode is LESS.
        Test(depth_format, compression_enabled, z_float_enabled, 0);
      }
    }
  }
}

void DepthFormatTests::Test(const DepthFormat &format, bool compress_z, bool z_format_float, uint32_t depth_cutoff) {
  host_.SetDepthBufferFormat(format.format);
  host_.SetDepthBufferFloatMode(z_format_float);

  host_.PrepareDraw(0xFF000000, depth_cutoff, 0x00);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, true);
  p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  p = pb_push1(p, NV097_SET_COMPRESS_ZBUFFER_EN, compress_z);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, true);

  uint32_t min_max_control = (1 << 8);  // CULL_IGNORE_W
  min_max_control |= (1 << 4);          // EN_CLAMP
  min_max_control |= 0;                 // CULL_NEAR_FAR_EN_FALSE
  p = pb_push1(p, NV097_SET_ZMIN_MAX_CONTROL, min_max_control);
  pb_end(p);

  host_.DrawVertices();

  const char *format_name = format.format == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? "z16" : "z24";
  pb_print("DF: %s\n", format_name);
  pb_print("CompZ: %s\n", compress_z ? "y" : "n");
  pb_print("FloatZ: %s\n", z_format_float ? "y" : "n");
  pb_print("Max: %x\n", depth_cutoff);
  pb_draw_text_screen();

  char buf[128] = {0};
  snprintf(buf, 127, "DepthFmt_%s_C%s_FZ%s_M%x", format_name, compress_z ? "y" : "n", z_format_float ? "y" : "n",
           depth_cutoff);

  host_.FinishDrawAndSave(output_dir_.c_str(), buf);
}

void DepthFormatTests::CreateGeometry(const DepthFormat &format, bool z_format_float) {
  // Prepare a large quad at the maximum depth with a number of smaller quads on top of it with progressively higher
  // z-buffer values.

  auto max_depth_float = static_cast<float>(format.max_depth);

  auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  // Place the big quad in the center of the framebuffer with some padding on all sides.
  float left = floorf(fb_width / 5.0f);
  float right = left + (fb_width - left * 2.0f);
  float top = floorf(fb_height / 12.0f);
  float bottom = top + (fb_height - top * 2.0f);
  float big_quad_width = right - left;
  float big_quad_height = bottom - top;

  // Place an array of smaller quads with increasing depth inside of the big quad.
  constexpr uint32_t kSmallSize = 36;
  constexpr uint32_t kSmallSpacing = 10;
  constexpr uint32_t kStep = kSmallSize + kSmallSpacing;

  uint32_t quads_per_row = static_cast<uint32_t>(big_quad_width - kSmallSize) / kStep;
  uint32_t quads_per_col = static_cast<uint32_t>(big_quad_height - kSmallSize) / kStep;

  float row_size = static_cast<float>(kSmallSize) + static_cast<float>(quads_per_row) * kStep;
  float col_size = static_cast<float>(kSmallSize) + static_cast<float>(quads_per_col) * kStep;

  float x_offset = left + kSmallSpacing + (big_quad_width - row_size) / 2.0f;
  float y_offset = top + kSmallSpacing + (big_quad_height - col_size) / 2.0f;

  uint32_t num_quads = 3 + quads_per_row * quads_per_col;
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6 * num_quads);

  // Quads are intentionally rendered from front to back to verify the behavior of the depth buffer.
  uint32_t idx = 0;
  float z_inc = max_depth_float / (static_cast<float>(num_quads) + 1.0f);
  float z_left = 0.0f;
  float right_offset = z_inc + 1.0f;
  float y = y_offset;
  Color ul = {1.0, 1.0, 1.0, 1.0};
  Color ll = {1.0, 1.0, 1.0, 1.0};
  Color lr = {1.0, 1.0, 1.0, 1.0};
  Color ur = {1.0, 1.0, 1.0, 1.0};

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

      float lz = z_left;
      float rz = z_right;
      buffer->DefineQuad(idx++, x, y, x + kSmallSize, y + kSmallSize, lz, lz, rz, rz, ul, ll, lr, ur);

      z_left += z_inc;
      right_offset *= -1.0f;
      x += kStep;
    }

    y += kStep;
  }

  ul.SetRGB(1.0, 0.0, 1.0);
  ur.SetRGB(1.0, 0.0, 1.0);
  ll.SetRGB(0.0, 1.0, 1.0);
  lr.SetRGB(0.0, 1.0, 1.0);
  float back_depth = max_depth_float - 5.0f;
  float front_depth = 0.0f;
  buffer->DefineQuad(idx++, left + 5, top, left + 15, bottom, front_depth, back_depth, back_depth, front_depth, ul, ll,
                     lr, ur);

  ll.SetRGB(1.0, 0.0, 1.0);
  lr.SetRGB(1.0, 0.0, 1.0);
  ul.SetRGB(0.0, 1.0, 1.0);
  ur.SetRGB(0.0, 1.0, 1.0);
  back_depth = (back_depth / 2.0f);
  front_depth = 0.0f;
  buffer->DefineQuad(idx++, right - 15, top, right - 5, bottom, back_depth, front_depth, front_depth, back_depth, ul,
                     ll, lr, ur);

  ul.SetRGB(0.0, 1.0, 0.0);
  ll.SetRGB(1.0, 0.0, 0.0);
  lr.SetRGB(0.0, 0.0, 1.0);
  ur.SetRGB(0.3, 0.3, 0.3);
  back_depth = max_depth_float - 1.0f;
  front_depth = back_depth - (back_depth / 3.0f);

  buffer->DefineQuad(idx++, left, top, right, bottom, front_depth, back_depth, back_depth, front_depth, ul, ll, lr, ur);
}

float DepthFormatTests::DepthFormat::fixed_to_float(uint32_t val) const {
  if (format == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    return z16_to_float(val);
  }

  return z24_to_float(val);
}

uint32_t DepthFormatTests::DepthFormat::float_to_fixed(float val) const {
  if (format == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    return float_to_z16(val);
  }

  return float_to_z24(val);
}
