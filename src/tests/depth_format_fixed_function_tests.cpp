#include "depth_format_fixed_function_tests.h"

#include <pbkit/pbkit.h>

#include "../shaders/precalculated_vertex_shader.h"
#include "../test_host.h"
#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "vertex_buffer.h"

static constexpr uint32_t kF16MaxFixedRepresentation = 0x0000FFFF;
static constexpr uint32_t kF24MaxFixedRepresentation = 0x00FEFFFF;

// Keep in sync with the value used to set up the default XDK composite matrix.
static constexpr float kCameraZ = -7.0f;
static constexpr float kZNear = kCameraZ + 1.0f;
static constexpr float kZFar = kCameraZ + 200.0f;

constexpr DepthFormatFixedFunctionTests::DepthFormat kDepthFormats[] = {
    {NV097_SET_SURFACE_FORMAT_ZETA_Z16, 0x0000FFFF, false},
    {NV097_SET_SURFACE_FORMAT_ZETA_Z24S8, 0x00FFFFFF, false},
    {NV097_SET_SURFACE_FORMAT_ZETA_Z16, kF16MaxFixedRepresentation, true},
    {NV097_SET_SURFACE_FORMAT_ZETA_Z24S8, kF24MaxFixedRepresentation, true},
};

constexpr uint32_t kNumDepthTests = 4;
constexpr bool kCompressionSettings[] = {false, true};

DepthFormatFixedFunctionTests::DepthFormatFixedFunctionTests(TestHost &host, std::string output_dir,
                                                             const Config &config)
    : TestSuite(host, std::move(output_dir), "Depth buffer fixed function", config) {
  for (auto depth_format : kDepthFormats) {
    uint32_t depth_cutoff_step = depth_format.max_depth / kNumDepthTests;

    for (auto compression_enabled : kCompressionSettings) {
      uint32_t depth_cutoff = depth_format.max_depth;

      for (int i = 0; i <= kNumDepthTests; ++i, depth_cutoff -= depth_cutoff_step) {
        std::string name = MakeTestName(depth_format, compression_enabled, depth_cutoff);

        tests_[name] = [this, depth_format, compression_enabled, depth_cutoff]() {
          Test(depth_format, compression_enabled, depth_cutoff);
        };
      }
    }
  }
}

void DepthFormatFixedFunctionTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, 1);
  CreateGeometry();
}

void DepthFormatFixedFunctionTests::CreateGeometry() {
  auto fb_width = host_.GetFramebufferWidthF();
  auto fb_height = host_.GetFramebufferHeightF();

  // Place the graphic in the center of the framebuffer with some padding on all sides.
  const float kLeft = floorf(fb_width / 5.0f);
  const float kRight = fb_width - kLeft;
  const float kTop = floorf(fb_height / 10.0f);
  const float kBottom = fb_height - kTop;
  const float kBigQuadWidth = kRight - kLeft;
  const float kBigQuadHeight = kBottom - kTop;

  // Place an array of smaller quads with increasing depth inside of the big quad.
  constexpr uint32_t kSmallSize = 16;
  constexpr uint32_t kSmallSpacing = 4;
  constexpr uint32_t kStep = kSmallSize + kSmallSpacing;

  uint32_t quads_per_row = static_cast<uint32_t>(kBigQuadWidth - kSmallSize) / kStep;
  uint32_t quads_per_col = static_cast<uint32_t>(kBigQuadHeight - kSmallSize) / kStep;

  float row_size = static_cast<float>(kSmallSize) + static_cast<float>(quads_per_row) * kStep;
  float col_size = static_cast<float>(kSmallSize) + static_cast<float>(quads_per_col) * kStep;

  float x_offset = kLeft + kSmallSpacing + (kBigQuadWidth - row_size) / 2.0f;
  float y_offset = kTop + kSmallSpacing + (kBigQuadHeight - col_size) / 2.0f;

  uint32_t num_quads = 3 + quads_per_row * quads_per_col;
  auto buffer = host_.AllocateVertexBuffer(6 * num_quads);

  uint32_t idx = 0;
  //  auto z_far = static_cast<float>(format.max_depth);
  Color ul{};
  Color ll{};
  Color lr{};
  Color ur{};

  // Generate a grid of small quads going from min depth to ~max depth. The right size will alternate from being deeper
  // or less deep than the left side.
  // Quads are intentionally rendered from front to back to verify the behavior of the depth buffer.
  {
    float z_inc = (kZFar - kZNear) / (static_cast<float>(num_quads) + 1.0f);
    float z_left = kZNear;
    float right_offset = z_inc * 0.75f;
    float y = y_offset;

    for (int y_idx = 0; y_idx < quads_per_col; ++y_idx) {
      float x = x_offset;
      for (int x_idx = 0; x_idx < quads_per_row; ++x_idx) {
        float z_right = z_left + right_offset;
        float left_color = 0.25f + (z_left / kZFar * 0.75f);
        float right_color = 0.25f + (z_right / kZFar * 0.75f);
        ul.SetGrey(left_color);
        ll.SetGrey(left_color);
        lr.SetGrey(right_color);
        ur.SetGrey(right_color);

        vector_t ul_world;
        vector_t screen_point = {x, y, 0.0f, 1.0f};
        host_.UnprojectPoint(ul_world, screen_point, z_left);

        vector_t ur_world;
        screen_point[0] = x + kSmallSize;
        host_.UnprojectPoint(ur_world, screen_point, z_right);

        vector_t lr_world;
        screen_point[1] = y + kSmallSize;
        host_.UnprojectPoint(lr_world, screen_point, z_right);

        vector_t ll_world;
        screen_point[0] = x;
        host_.UnprojectPoint(ll_world, screen_point, z_left);

        buffer->DefineBiTri(idx++, ul_world, ur_world, lr_world, ll_world, ul, ll, lr, ur);

        z_left += z_inc;
        right_offset *= -1.0f;
        x += kStep;
      }

      y += kStep;
    }
  }

  // Add a small quad on the bottom going from 3/4 max depth on the left to min depth at the right.
  {
    const float z_left = kZFar * 0.75;
    const float z_right = kZNear;

    ul.SetRGB(z_left / kZFar, 0.75, z_left / kZFar);
    ll.SetRGB(z_left / kZFar, 0.75, z_left / kZFar);
    ur.SetRGB(0.75, z_right / kZFar, z_right / kZFar);
    lr.SetRGB(0.75, z_right / kZFar, z_right / kZFar);

    PrintMsg("Bottom quad: %g -> %g\n", z_left, z_right);

    vector_t ul_world;
    vector_t screen_point = {kLeft + 4, kBottom - 10, 0.0f, 1.0f};
    host_.UnprojectPoint(ul_world, screen_point, z_left);

    vector_t ur_world;
    screen_point[0] = kRight - 20;
    host_.UnprojectPoint(ur_world, screen_point, z_right);

    vector_t lr_world;
    screen_point[1] = kBottom - 5;
    host_.UnprojectPoint(lr_world, screen_point, z_right);

    vector_t ll_world;
    screen_point[0] = kLeft + 4;
    host_.UnprojectPoint(ll_world, screen_point, z_left);

    buffer->DefineBiTri(idx++, ul_world, ur_world, lr_world, ll_world, ul, ll, lr, ur);
  }

  // Add a small quad on the right going from near min depth at the top to 1/2 max depth at the bottom.
  {
    const float z_top = kZNear;
    const float z_bottom = kZFar * 0.5f;

    ul.SetRGB(z_top / kZFar, 0.5, z_top / kZFar);
    ur.SetRGB(z_top / kZFar, 0.5, z_top / kZFar);
    ll.SetRGB(0.5, z_bottom / kZFar, z_bottom / kZFar);
    lr.SetRGB(0.5, z_bottom / kZFar, z_bottom / kZFar);

    PrintMsg("Right quad: %g -> %g\n", z_top, z_bottom);

    vector_t ul_world;
    vector_t screen_point = {kRight - 10, kTop + 5, 0.0f, 1.0f};
    host_.UnprojectPoint(ul_world, screen_point, z_top);

    vector_t ur_world;
    screen_point[0] = kRight - 2;
    host_.UnprojectPoint(ur_world, screen_point, z_top);

    vector_t lr_world;
    screen_point[1] = kBottom - 5;
    host_.UnprojectPoint(lr_world, screen_point, z_bottom);

    vector_t ll_world;
    screen_point[0] = kRight - 10;
    host_.UnprojectPoint(ll_world, screen_point, z_bottom);

    buffer->DefineBiTri(idx++, ul_world, ur_world, lr_world, ll_world, ul, ll, lr, ur);
  }

  // Add a large quad going from 1/3 max depth at the top to max depth at the bottom.
  {
    ul.SetRGB(0.0, 1.0, 0.0);
    ll.SetRGB(0.0, 0.0, 1.0);
    lr.SetRGB(0.4, 0.3, 0.0);
    ur.SetRGB(1.0, 0.0, 0.0);

    float bottom_z = kZFar;
    float top_z = bottom_z * 2.0f / 3.0f;
    PrintMsg("Big quad: %g -> %g\n", top_z, bottom_z);

    vector_t ul_world;
    vector_t screen_point = {kLeft, kTop, 0.0f, 1.0f};
    host_.UnprojectPoint(ul_world, screen_point, top_z);

    vector_t ur_world;
    screen_point[0] = kRight;
    host_.UnprojectPoint(ur_world, screen_point, top_z);

    vector_t lr_world;
    screen_point[1] = kBottom;
    host_.UnprojectPoint(lr_world, screen_point, bottom_z);

    vector_t ll_world;
    screen_point[0] = kLeft;
    host_.UnprojectPoint(ll_world, screen_point, bottom_z);

    buffer->DefineBiTri(idx++, ul_world, ur_world, lr_world, ll_world, ul, ll, lr, ur);
  }
}

void DepthFormatFixedFunctionTests::Test(const DepthFormat &format, bool compress_z, uint32_t depth_cutoff) {
  host_.SetSurfaceFormat(host_.GetColorBufferFormat(), static_cast<TestHost::SurfaceZetaFormat>(format.format),
                         host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetDepthBufferFloatMode(format.floating_point);
  host_.PrepareDraw(0xFF000000, depth_cutoff);

  // Override the depth clip to ensure that max_val depths (post-projection) are never clipped.
  host_.SetDepthClip(0.0f, 16777216);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, true);
  p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LESS);
  p = pb_push1(p, NV097_SET_COMPRESS_ZBUFFER_EN, compress_z);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, false);
  pb_end(p);

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

std::string DepthFormatFixedFunctionTests::MakeTestName(const DepthFormat &format, bool compress_z,
                                                        uint32_t depth_cutoff) {
  const char *format_name = format.format == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? "z16" : "z24";
  char buf[64] = {0};
  snprintf(buf, 63, "%s_C%s_FZ%s_M%06.6x", format_name, compress_z ? "y" : "n", format.floating_point ? "y" : "n",
           depth_cutoff);
  return buf;
}
