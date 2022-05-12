#include "texture_shadow_comparator_tests.h"

#include <pbkit/pbkit.h>

#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/perspective_vertex_shader.h"
#include "shaders/precalculated_vertex_shader.h"
#include "swizzle.h"
#include "test_host.h"

// Uncomment to save the depth texture as an additional artifact.
//#define DEBUG_DUMP_DEPTH_TEXTURE

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// NV097_SET_CONTEXT_DMA_ZETA is set to channel 10 by default.
const uint32_t kDefaultDMAZetaChannel = 10;

// Number of small box values to set in the depth texture.
static constexpr int kHorizontal = 15;
static constexpr int kVertical = 15;

// Keep in sync with the value used to set up the default XDK composite matrix.
static constexpr float kCameraZ = -7.0f;
static constexpr float kZNear = kCameraZ + 1.0f;
static constexpr float kZFar = kCameraZ + 200.0f;

// Used to determine the tests around the end and mid/quarter points in projection tests.
static constexpr float kEpsilon = 0.0125f;

// clang format off
static constexpr uint32_t kCompareFuncs[] = {
    NV097_SET_SHADOW_COMPARE_FUNC_NEVER,  NV097_SET_SHADOW_COMPARE_FUNC_GREATER, NV097_SET_SHADOW_COMPARE_FUNC_EQUAL,
    NV097_SET_SHADOW_COMPARE_FUNC_GEQUAL, NV097_SET_SHADOW_COMPARE_FUNC_LESS,    NV097_SET_SHADOW_COMPARE_FUNC_NOTEQUAL,
    NV097_SET_SHADOW_COMPARE_FUNC_LEQUAL, NV097_SET_SHADOW_COMPARE_FUNC_ALWAYS,
};
// clang format on

struct BoxLayoutInfo {
  uint32_t box_width;
  uint32_t box_height;
  uint32_t spacing;
  uint32_t first_box_left;
  uint32_t top;
};

static std::string ShortDepthName(const TextureFormatInfo &format, uint32_t depth_format, bool float_depth) {
  switch (depth_format) {
    case NV097_SET_SURFACE_FORMAT_ZETA_Z16:
      return float_depth ? "16f" : "16";

    case NV097_SET_SURFACE_FORMAT_ZETA_Z24S8:
      return float_depth ? "24f" : "24";

    default:
      ASSERT(!"Unhandled depth format");
  }
  return "<<INVALID>>";
}

static std::string CompareFunctionName(uint32_t comp_func) {
  switch (comp_func) {
    case NV097_SET_SHADOW_COMPARE_FUNC_NEVER:
      return "N";

    case NV097_SET_SHADOW_COMPARE_FUNC_GREATER:
      return "GT";

    case NV097_SET_SHADOW_COMPARE_FUNC_EQUAL:
      return "EQ";

    case NV097_SET_SHADOW_COMPARE_FUNC_GEQUAL:
      return "GE";

    case NV097_SET_SHADOW_COMPARE_FUNC_LESS:
      return "LT";

    case NV097_SET_SHADOW_COMPARE_FUNC_NOTEQUAL:
      return "NE";

    case NV097_SET_SHADOW_COMPARE_FUNC_LEQUAL:
      return "LE";

    case NV097_SET_SHADOW_COMPARE_FUNC_ALWAYS:
      return "A";

    default:
      ASSERT(!"Invalid compare function");
  }

  return "<<INVALID>>";
}

static std::string MakeRawValueTestName(const TextureFormatInfo &format, uint32_t depth_format, uint32_t comp_func,
                                        uint32_t min_val, uint32_t max_val, uint32_t ref) {
  std::string ret = "R";
  ret += ShortDepthName(format, depth_format, false);

  char buf[32] = {0};
  sprintf(buf, "_%X-%X_%X_", min_val, max_val, ref);
  ret += buf;

  ret += CompareFunctionName(comp_func);
  return std::move(ret);
}

static std::string MakeFixedFunctionTestName(const TextureFormatInfo &format, uint32_t depth_format, bool float_depth,
                                             float min_val, float max_val, float ref, uint32_t comp_func) {
  std::string ret = "F";
  ret += ShortDepthName(format, depth_format, float_depth);

  char buf[32] = {0};
  sprintf(buf, "_%0.02f-%0.02f_%0.04f_", min_val, max_val, ref);
  ret += buf;

  ret += CompareFunctionName(comp_func);
  return std::move(ret);
}

static std::string MakeProgrammableTestName(const TextureFormatInfo &format, uint32_t depth_format, bool float_depth,
                                            float min_val, float max_val, float ref, uint32_t comp_func) {
  std::string ret = "P";
  ret += ShortDepthName(format, depth_format, float_depth);

  char buf[32] = {0};
  sprintf(buf, "_%0.02f-%0.02f_%0.04f_", min_val, max_val, ref);
  ret += buf;

  ret += CompareFunctionName(comp_func);
  return std::move(ret);
}

TextureShadowComparatorTests::TextureShadowComparatorTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture shadow comparator") {
  auto add_test = [this](uint32_t texture_format, uint32_t surface_format, uint32_t comp_func, uint32_t min_val,
                         uint32_t max_val, uint32_t ref) {
    const TextureFormatInfo &texture_format_info = GetTextureFormatInfo(texture_format);
    std::string name = MakeRawValueTestName(texture_format_info, surface_format, comp_func, min_val, max_val, ref);
    tests_[name] = [this, surface_format, texture_format, comp_func, name, min_val, max_val, ref]() {
      TestRawValues(surface_format, texture_format, comp_func, min_val, max_val, ref, name);
    };
  };

  auto add_perspective_tests = [this](uint32_t texture_format, uint32_t surface_format, bool float_depth,
                                      uint32_t comp_func, float min_val, float max_val, float ref_val) {
    const TextureFormatInfo &texture_format_info = GetTextureFormatInfo(texture_format);
    std::string ff = MakeFixedFunctionTestName(texture_format_info, surface_format, float_depth, min_val, max_val,
                                               ref_val, comp_func);
    std::string prog = MakeProgrammableTestName(texture_format_info, surface_format, float_depth, min_val, max_val,
                                                ref_val, comp_func);
    tests_[ff] = [this, surface_format, float_depth, texture_format, comp_func, min_val, max_val, ref_val, ff]() {
      TestFixedFunction(surface_format, float_depth, texture_format, comp_func, min_val, max_val, ref_val, ff);
    };
    tests_[prog] = [this, surface_format, float_depth, texture_format, comp_func, min_val, max_val, ref_val, prog]() {
      TestProgrammable(surface_format, float_depth, texture_format, comp_func, min_val, max_val, ref_val, prog);
    };
  };

  // TODO: Test floating point zeta buffers.
  for (auto comp_func : kCompareFuncs) {
    add_test(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED, NV097_SET_SURFACE_FORMAT_ZETA_Z16, comp_func, 0,
             0xFFFF, 0x7FFF);
    add_test(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED, NV097_SET_SURFACE_FORMAT_ZETA_Z16, comp_func, 0,
             256, 128);
    add_test(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED, NV097_SET_SURFACE_FORMAT_ZETA_Z16, comp_func, 256,
             512, 384);

    add_test(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_X8_Y24_FIXED, NV097_SET_SURFACE_FORMAT_ZETA_Z24S8, comp_func,
             0, 0xFFFFFF, 0x7FFFFF);
    add_test(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_X8_Y24_FIXED, NV097_SET_SURFACE_FORMAT_ZETA_Z24S8, comp_func,
             0, 256, 128);
    add_test(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_X8_Y24_FIXED, NV097_SET_SURFACE_FORMAT_ZETA_Z24S8, comp_func,
             256, 512, 384);

    {
      const float kZRef = kZNear + (kZFar - kZNear) * 0.06f;
      add_perspective_tests(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED, NV097_SET_SURFACE_FORMAT_ZETA_Z16,
                            false, comp_func, kZNear, kZFar, kZRef);
      add_perspective_tests(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED, NV097_SET_SURFACE_FORMAT_ZETA_Z16,
                            false, comp_func, 10.0f, 20.0f, 15.0f);
    }

    {
      const float kZRef = kZNear + (kZFar - kZNear) * 0.0531f;
      add_perspective_tests(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FLOAT, NV097_SET_SURFACE_FORMAT_ZETA_Z16,
                            true, comp_func, kZNear, kZFar, kZRef);
      add_perspective_tests(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FLOAT, NV097_SET_SURFACE_FORMAT_ZETA_Z16,
                            true, comp_func, 10.0f, 20.0f, 14.04f);
    }

    add_perspective_tests(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_X8_Y24_FIXED,
                          NV097_SET_SURFACE_FORMAT_ZETA_Z24S8, false, comp_func, kZNear, kZFar, 10.65f);

    {
      // TODO: Unify these in an add_perspective_tests call.
      // There is some subtle difference between the fixed function pipeline and the programmable shader written to
      // attempt to duplicate it. The depth values at 24 bit end up being ~1 off, making it impossible to choose a
      // reference value that will be matched by both pipelines.
      constexpr uint32_t texture_format = NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_X8_Y24_FIXED;
      constexpr uint32_t surface = NV097_SET_SURFACE_FORMAT_ZETA_Z24S8;
      const TextureFormatInfo &texture_format_info = GetTextureFormatInfo(texture_format);
      std::string ff = MakeFixedFunctionTestName(texture_format_info, surface, false, 10.0f, 20.0f, 11.47f, comp_func);
      tests_[ff] = [this, comp_func, ff]() {
        TestFixedFunction(surface, false, texture_format, comp_func, 10.0f, 20.0f, 11.47f, ff);
      };

      std::string prog = MakeProgrammableTestName(texture_format_info, surface, false, 10.0f, 20.0f, 11.45f, comp_func);
      tests_[prog] = [this, comp_func, prog]() {
        TestProgrammable(surface, false, texture_format, comp_func, 10.0f, 20.0f, 11.45f, prog);
      };
    }
  }
}

void TextureShadowComparatorTests::Initialize() {
  TestSuite::Initialize();

  auto channel = kNextContextChannel;
  pb_create_dma_ctx(channel++, DMA_CLASS_3D, 0, MAXRAM, &texture_target_ctx_);
  pb_bind_channel(&texture_target_ctx_);

  raw_value_shader_ = std::make_shared<PrecalculatedVertexShader>(true);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_MASK, false);
  pb_end(p);
}

void TextureShadowComparatorTests::Deinitialize() {
  TestSuite::Deinitialize();
  raw_value_shader_.reset();
}

static BoxLayoutInfo GetExplicitBoxLayout(uint32_t buffer_width, uint32_t buffer_height) {
  const uint32_t box_width = buffer_width / kHorizontal;
  const uint32_t box_height = buffer_height / kVertical;

  BoxLayoutInfo info;
  info.box_width = box_width / 2;
  info.box_height = box_height * 2;
  info.spacing = (box_width - info.box_width) * 2;

  info.first_box_left = (buffer_width / 2) - (info.box_width + info.spacing) * 3 + (info.box_width / 2);
  info.top = buffer_height / 2 - (info.box_width / 2);

  return info;
}

template <typename T>
static void PrepareRawValueTestTexture(uint8_t *memory, uint32_t width, uint32_t height, T min_val, T max_val,
                                       T default_val, uint32_t left_shift = 0) {
  static constexpr int kTotal = kHorizontal * kVertical;

  const uint32_t box_width = width / kHorizontal;
  const uint32_t box_height = height / kVertical;

  const uint32_t x_indent = width - (box_width * kHorizontal);
  const uint32_t y_indent = height - (box_height * kVertical);

  const uint32_t pitch = width * sizeof(T);
  auto buffer = reinterpret_cast<T *>(memory);
  for (auto i = 0; i < width * height; ++i) {
    buffer[i] = default_val;
  }

  auto row = buffer;

  auto value = static_cast<float>(min_val);
  float val_inc = static_cast<float>(max_val) / static_cast<float>(kTotal);
  auto row_data = new T[width];
  for (auto i = 0; i < width; ++i) {
    row_data[i] = default_val << left_shift;
  }

  for (auto y = y_indent >> 1; y < kVertical; ++y) {
    auto box_pixel = row_data + (x_indent >> 1);
    for (auto x = 0; x < kHorizontal; ++x) {
      for (auto i = 0; i < box_width; ++i, ++box_pixel) {
        *box_pixel = static_cast<T>(value);
      }
      value += val_inc;
    }

    for (auto y_row = 0; y_row < box_height; ++y_row, row += width) {
      memcpy(row, row_data, pitch);
    }
  }

  delete[] row_data;

  // Place a few small boxes of known values near the middle.
  // Values:
  // {0, 1, default - 1, default, default + 1, max - 1, max}
  auto layout = GetExplicitBoxLayout(width, height);

  uint32_t left = layout.first_box_left;
  uint32_t top = layout.top;
  row = buffer + top * width;

  auto set_box = [&layout, left_shift](T *row, uint32_t left, T value) {
    T *pixel = row + left;
    value <<= left_shift;
    for (auto x = 0; x < layout.box_width; ++x) {
      *pixel++ = value;
    }
  };

  for (uint32_t y = top; y < top + layout.box_height; ++y, row += width) {
    auto x = left;
    set_box(row, x, 0);
    x += layout.box_width + layout.spacing;

    set_box(row, x, 1);
    x += layout.box_width + layout.spacing;

    set_box(row, x, default_val - 1);
    x += layout.box_width + layout.spacing;

    set_box(row, x, default_val);
    x += layout.box_width + layout.spacing;

    set_box(row, x, default_val + 1);
    x += layout.box_width + layout.spacing;

    set_box(row, x, static_cast<T>(max_val) - 1);
    x += layout.box_width + layout.spacing;

    set_box(row, x, static_cast<T>(max_val));
  }
}

void TextureShadowComparatorTests::TestRawValues(uint32_t depth_format, uint32_t texture_format,
                                                 uint32_t shadow_comp_function, uint32_t min_val, uint32_t max_val,
                                                 uint32_t ref, const std::string &name) {
  host_.SetVertexShaderProgram(raw_value_shader_);

  host_.PrepareDraw(0xFE112233);

  switch (depth_format) {
    case NV097_SET_SURFACE_FORMAT_ZETA_Z16:
      PrepareRawValueTestTexture<uint16_t>(host_.GetTextureMemory(), host_.GetFramebufferWidth(),
                                           host_.GetFramebufferHeight(), min_val, max_val, ref);
      break;

    case NV097_SET_SURFACE_FORMAT_ZETA_Z24S8:
      PrepareRawValueTestTexture<uint32_t>(host_.GetTextureMemory(), host_.GetFramebufferWidth(),
                                           host_.GetFramebufferHeight(), min_val, max_val, ref, 8);
      break;
  }

#ifdef DEBUG_DUMP_DEPTH_TEXTURE
  if (allow_saving_) {
    std::string z_buffer_name = name + "_DT";
    const uint32_t bpp = depth_format == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? 16 : 32;
    const uint32_t texture_pitch = host_.GetFramebufferWidth() * (bpp >> 3);
    host_.SaveRawTexture(output_dir_, z_buffer_name, host_.GetTextureMemory(), host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight(), texture_pitch, bpp);
  }
#endif

  // Render a quad using the zeta buffer as a shadow map applied to the diffuse color.

  // The texture map is used as a color source and will either be 0xFFFFFFFF or 0x00000000 for any given texel.
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  auto &stage = host_.GetTextureStage(0);
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SHADOW_COMPARE_FUNC, shadow_comp_function);
  pb_end(p);

  stage.SetFormat(GetTextureFormatInfo(texture_format));
  host_.SetShaderStageProgram(TestHost::STAGE_3D_PROJECTIVE);
  stage.SetFormat(GetTextureFormatInfo(texture_format));
  stage.SetTextureDimensions(1, 1, 1);
  stage.SetImageDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetTextureStageEnabled(0, true);
  host_.SetupTextureStages();

  {
    const float kLeft = 0.0f;
    const float kRight = host_.GetFramebufferWidthF() - kLeft;
    const float kTop = 100.0f;
    const float kBottom = host_.GetFramebufferHeightF() - kTop;

    const auto tex_depth = static_cast<float>(ref);
    const float z = 1.5f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFF2277FF);
    host_.SetTexCoord0(0.0f, 0.0f, tex_depth, 1.0f);
    host_.SetVertex(kLeft, kTop, z, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF(), 0.0f, tex_depth, 1.0f);
    host_.SetVertex(kRight, kTop, z, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF(), tex_depth, 1.0f);
    host_.SetVertex(kRight, kBottom, z, 1.0f);

    host_.SetTexCoord0(0.0, host_.GetFramebufferHeightF(), tex_depth, 1.0f);
    host_.SetVertex(kLeft, kBottom, z, 1.0f);
    host_.End();
  }

  // Render tiny triangles in the center of each explicit value box to make them visible regardless of the comparison.
  {
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetTextureStageEnabled(0, false);
    host_.SetupTextureStages();

    auto layout = GetExplicitBoxLayout(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    static constexpr uint32_t kMarkerColors[] = {
        0xFFFFAA00, 0xFFFF7700, 0xFFCC5500, 0xFF00FF00, 0xFF00AACC, 0xFF005577, 0xFF002266,
    };
    const float left_offset = static_cast<float>(layout.box_width) / 4.0f;
    const float mid_offset = left_offset * 2.0f;
    const float right_offset = mid_offset + left_offset;
    auto left = static_cast<float>(layout.first_box_left);

    const float top_offset = static_cast<float>(layout.box_height) / 4.0f;
    const float bottom_offset = top_offset * 2.0f;
    const float top = static_cast<float>(layout.top) + top_offset;
    const float bottom = static_cast<float>(layout.top) + bottom_offset;

    for (auto color : kMarkerColors) {
      host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
      host_.SetDiffuse(color);
      host_.SetVertex(left + left_offset, bottom, 0.0f, 1.0f);

      host_.SetVertex(left + mid_offset, top, 0.0f, 1.0f);

      host_.SetVertex(left + right_offset, bottom, 0.0f, 1.0f);

      left += static_cast<float>(layout.spacing + layout.box_width);
      host_.End();
    }
  }

  pb_print("%s\n", name.c_str());
  pb_print("Rng 0x%X-0x%x\n", min_val, max_val);
  pb_print("Ref, edges, center: 0x%X\n", ref);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

void TextureShadowComparatorTests::TestFixedFunction(uint32_t depth_format, bool float_depth, uint32_t texture_format,
                                                     uint32_t shadow_comp_function, float min_val, float max_val,
                                                     float ref_val, const std::string &name) {
  host_.SetVertexShaderProgram(nullptr);
  host_.SetSurfaceFormat(host_.GetColorBufferFormat(), static_cast<TestHost::SurfaceZetaFormat>(depth_format),
                         host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetDepthBufferFloatMode(float_depth);

  auto project_point = [this](VECTOR out, const VECTOR in) { host_.ProjectPoint(out, in); };
  auto unproject_point = [this](VECTOR out, const VECTOR in, float z) { host_.UnprojectPoint(out, in, z); };
  TestProjected(depth_format, texture_format, shadow_comp_function, min_val, max_val, ref_val, project_point,
                unproject_point, name);
}

void TextureShadowComparatorTests::TestProgrammable(uint32_t depth_format, bool float_depth, uint32_t texture_format,
                                                    uint32_t shadow_comp_function, float min_val, float max_val,
                                                    float ref_val, const std::string &name) {
  float depth_buffer_max_value = host_.MaxDepthBufferValue(depth_format, float_depth);
  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                          0.0f, depth_buffer_max_value, M_PI * 0.25f, -1.0f, 1.0f, 1.0f,
                                                          -1.0f, 1.0f, 200.0f);
  {
    shader->SetLightingEnabled(false);
    shader->SetUse4ComponentTexcoords();
    shader->SetUseD3DStyleViewport();
    VECTOR camera_position = {0.0f, 0.0f, kCameraZ, 1.0f};
    VECTOR camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);
  }
  host_.SetVertexShaderProgram(shader);
  host_.SetSurfaceFormat(host_.GetColorBufferFormat(), static_cast<TestHost::SurfaceZetaFormat>(depth_format),
                         host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetDepthBufferFloatMode(float_depth);

  auto project_point = [shader](VECTOR out, const VECTOR in) { shader->ProjectPoint(out, in); };
  auto unproject_point = [shader](VECTOR out, const VECTOR in, float z) { shader->UnprojectPoint(out, in, z); };

  TestProjected(depth_format, texture_format, shadow_comp_function, min_val, max_val, ref_val, project_point,
                unproject_point, name);
}

void TextureShadowComparatorTests::TestProjected(uint32_t depth_format, uint32_t texture_format,
                                                 uint32_t shadow_comp_function, float min_val, float max_val,
                                                 float ref_val, std::function<void(VECTOR, const VECTOR)> project_point,
                                                 std::function<void(VECTOR, const VECTOR, float)> unproject_point,
                                                 const std::string &name) {
  auto p = pb_begin();
  // Depth test must be enabled or nothing will be written to the depth target.
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, true);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, true);
  p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_ALWAYS);

  // Point the depth buffer at the base of texture memory.
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, texture_target_ctx_.ChannelID);
  p = pb_push1(p, NV097_SET_SURFACE_ZETA_OFFSET, reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF);
  pb_end(p);

  host_.PrepareDraw(0xFE332211);

  // Override the depth clip to ensure that max_val depths (post-projection) are never clipped.
  host_.SetDepthClip(0.0f, 16777216);

  // Render test quads into the depth buffer.
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  // Coordinates are unprojected from screen space to allow a sloped Z that still appears rectangular.
  const float sLeft = host_.GetFramebufferWidthF() * 0.00125f;
  float sTop = host_.GetFramebufferHeightF() * 0.0125f;
  const float sRight = host_.GetFramebufferWidthF() - sLeft;
  float sBottom = host_.GetFramebufferHeightF() - sTop;

  {
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    // Render a background quad from min depth at the top to max depth along the bottom.
    host_.SetDiffuse(0xFF440044);

    float z_top = min_val;
    float z_bottom = max_val;

    VECTOR ul;
    VECTOR screen_point = {sLeft, sTop, 0.0f, 1.0f};
    unproject_point(ul, screen_point, z_top);

    VECTOR ur;
    screen_point[_X] = sRight;
    unproject_point(ur, screen_point, z_top);

    VECTOR lr;
    screen_point[_Y] = sBottom;
    unproject_point(lr, screen_point, z_bottom);

    VECTOR ll;
    screen_point[_X] = sLeft;
    unproject_point(ll, screen_point, z_bottom);

    host_.SetVertex(ul[_X], ul[_Y], z_top, 1.0f);
    host_.SetVertex(ur[_X], ur[_Y], z_top, 1.0f);
    host_.SetVertex(lr[_X], lr[_Y], z_bottom, 1.0f);
    host_.SetVertex(ll[_X], ll[_Y], z_bottom, 1.0f);
    host_.End();
  }

  auto layout = GetExplicitBoxLayout(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  // Render quads at various depths along the center of the screen.
  {
    auto box = [this, &unproject_point](float left, float top, float right, float bottom, float z) {
      host_.SetDiffuse(0xFFAA11AA);
      VECTOR ul;
      VECTOR screen_point = {left, top, 0.0f, 1.0f};
      unproject_point(ul, screen_point, z);

      VECTOR ur;
      screen_point[_X] = right;
      unproject_point(ur, screen_point, z);

      VECTOR lr;
      screen_point[_Y] = bottom;
      unproject_point(lr, screen_point, z);

      VECTOR ll;
      screen_point[_X] = left;
      unproject_point(ll, screen_point, z);

      host_.SetVertex(ul[_X], ul[_Y], z, 1.0f);
      host_.SetVertex(ur[_X], ur[_Y], z, 1.0f);
      host_.SetVertex(lr[_X], lr[_Y], z, 1.0f);
      host_.SetVertex(ll[_X], ll[_Y], z, 1.0f);
    };
    auto left = static_cast<float>(layout.first_box_left);
    const auto top = static_cast<float>(layout.top);
    const auto box_width = static_cast<float>(layout.box_width);
    const auto box_height = static_cast<float>(layout.box_height);

    const float kZValues[] = {
        min_val, min_val + kEpsilon, ref_val - kEpsilon, ref_val, ref_val + kEpsilon, max_val - kEpsilon, max_val,
    };

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    for (auto z : kZValues) {
      box(left, top, left + box_width, top + box_height, z);
      left += static_cast<float>(layout.spacing + layout.box_width);
    }
    host_.End();
  }

  // The zeta buffer is configured by pbkit to be 32-bpp. This is independent of the actual surface format being used,
  // and is tied to the tile assignment. Rather than modify tiling, a 32-bpp pitch is used.
  const uint32_t texture_pitch = host_.GetFramebufferWidth() * 4;

#ifdef DEBUG_DUMP_DEPTH_TEXTURE
  if (allow_saving_) {
    std::string z_buffer_name = name + "_DT";
    const uint32_t bpp = depth_format == NV097_SET_SURFACE_FORMAT_ZETA_Z16 ? 16 : 32;

    host_.SaveRawTexture(output_dir_, z_buffer_name, host_.GetTextureMemory(), host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight(), texture_pitch, bpp);
  }
#endif

  p = pb_begin();
  // Restore the depth buffer.
  p = pb_push1(p, NV097_SET_SURFACE_ZETA_OFFSET, 0);
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAZetaChannel);
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  pb_end(p);

  // Clear the visible part.
  host_.ClearColorRegion(0xFE443333);

  // Render a quad using the zeta buffer as a shadow map applied to the diffuse color.
  // The texture map is used as a color source and will either be 0xFFFFFFFF or 0x00000000 for any given texel.
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  auto &stage = host_.GetTextureStage(0);
  p = pb_begin();
  p = pb_push1(p, NV097_SET_SHADOW_COMPARE_FUNC, shadow_comp_function);
  pb_end(p);

  host_.SetShaderStageProgram(TestHost::STAGE_3D_PROJECTIVE);
  stage.SetFormat(GetTextureFormatInfo(texture_format));
  stage.SetTextureDimensions(1, 1, 1);
  stage.SetImageDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetTextureStageEnabled(0, true);
  host_.SetupTextureStages();

  // Override the texture-format dependent pitch since it is governed by the zeta buffer tiling strategy which has not
  // been updated.
  p = pb_begin();
  p = pb_push1(p, NV097_SET_TEXTURE_CONTROL1, texture_pitch << 16);
  pb_end(p);

  float projected_ref_val = ref_val;
  {
    // The comparison value needs to go through the same projection as the depth values themselves.
    VECTOR projected_point;
    VECTOR world_point = {0.0f, 0.0f, projected_ref_val, 1.0f};
    project_point(projected_point, world_point);

    // There is a half texel offset difference between the shadow lookup and the actual location.
    projected_ref_val = projected_point[_Z];
  }

  {
    // The left/right coordinates match the depth buffer creation and the quad is centered on the screen so that the
    // rough centers of the known-value boxes can be tagged with triangles below.
    sTop = host_.GetFramebufferHeightF() * 0.2f;
    sBottom = host_.GetFramebufferHeightF() - sTop;

    const float z = 1.5f;
    VECTOR ul;
    VECTOR screen_point = {sLeft, sTop, 0.0f, 1.0f};
    unproject_point(ul, screen_point, z);

    VECTOR lr;
    screen_point[_X] = sRight;
    screen_point[_Y] = sBottom;
    unproject_point(lr, screen_point, z);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFF2277FF);
    host_.SetTexCoord0(0.0f, 0.0f, projected_ref_val, 1.0f);
    host_.SetVertex(ul[_X], ul[_Y], z, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF(), 0.0f, projected_ref_val, 1.0f);
    host_.SetVertex(lr[_X], ul[_Y], z, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF(), projected_ref_val, 1.0f);
    host_.SetVertex(lr[_X], lr[_Y], z, 1.0f);

    host_.SetTexCoord0(0.0, host_.GetFramebufferHeightF(), projected_ref_val, 1.0f);
    host_.SetVertex(ul[_X], lr[_Y], z, 1.0f);
    host_.End();
  }

  // Render tiny triangles in the center of each explicit value box to make them visible regardless of the comparison.
  {
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetTextureStageEnabled(0, false);
    host_.SetupTextureStages();

    static constexpr uint32_t kMarkerColors[] = {
        0xFFFFAA00, 0xFFFF7700, 0xFFCC5500, 0xFF00FF00, 0xFF00AACC, 0xFF005577, 0xFF002266,
    };

    const float left_offset = static_cast<float>(layout.box_width) / 4.0f;
    const float mid_offset = left_offset * 2.0f;
    const float right_offset = mid_offset + left_offset;
    auto left = static_cast<float>(layout.first_box_left);

    const float top_offset = static_cast<float>(layout.box_height) / 4.0f;
    const float bottom_offset = top_offset * 2.0f;
    const float top = static_cast<float>(layout.top) + top_offset;
    const float bottom = static_cast<float>(layout.top) + bottom_offset;

    for (auto color : kMarkerColors) {
      host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
      host_.SetDiffuse(color);

      VECTOR pt;

      VECTOR screen_point = {left + left_offset, bottom, 0.0f, 1.0f};
      unproject_point(pt, screen_point, 0.0f);
      host_.SetVertex(pt);

      screen_point[_X] = left + mid_offset;
      screen_point[_Y] = top;
      unproject_point(pt, screen_point, 0.0f);
      host_.SetVertex(pt);

      screen_point[_X] = left + right_offset;
      screen_point[_Y] = bottom;
      unproject_point(pt, screen_point, 0.0f);
      host_.SetVertex(pt);

      left += static_cast<float>(layout.spacing + layout.box_width);
      host_.End();
    }
  }

  pb_print("%s\n", name.c_str());
  pb_print("Rng %.02f-%.02f\n", min_val, max_val);
  pb_print("Ref: %.02f (0x%X)\n", ref_val, static_cast<uint32_t>(projected_ref_val));

  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}
