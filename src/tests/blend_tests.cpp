#include "blend_tests.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"
#include "texture_generator.h"

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;
// From pbkit.c, DMA_B is set to channel 11 by default
// NV097_SET_CONTEXT_DMA_B == NV20_TCL_PRIMITIVE_3D_SET_OBJECT2
// static constexpr uint32_t kDefaultDMAChannelB = 11;
// From pbkit.c, DMA_COLOR is set to channel 9 by default.
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
static constexpr uint32_t kDefaultDMAColorChannel = 9;

static constexpr uint32_t kCheckerboardA = 0xFF202020;
static constexpr uint32_t kCheckerboardB = 0xFF000000;

static constexpr uint32_t kBlendColorConstant = 0x55555555;
static constexpr uint32_t kColorSwatchBackground = 0x33333333;
static constexpr uint32_t kBlendCheckerboardSize = 16;

struct BlendFunc {
  const char *name;
  uint32_t value;
};

static constexpr struct BlendFunc kBlendEqns[] = {
    {"ADD", NV097_SET_BLEND_EQUATION_V_FUNC_ADD},
    {"SUB", NV097_SET_BLEND_EQUATION_V_FUNC_SUBTRACT},
    {"REVSUB", NV097_SET_BLEND_EQUATION_V_FUNC_REVERSE_SUBTRACT},
    {"MIN", NV097_SET_BLEND_EQUATION_V_MIN},
    {"MAX", NV097_SET_BLEND_EQUATION_V_MAX},
    {"SADD", NV097_SET_BLEND_EQUATION_V_FUNC_ADD_SIGNED},
    {"SREVSUB", NV097_SET_BLEND_EQUATION_V_FUNC_REVERSE_SUBTRACT_SIGNED},
};

// SFACTORs and DFACTORS are all identical
static constexpr struct BlendFunc kBlendFactors[] = {
    {"0", NV097_SET_BLEND_FUNC_SFACTOR_V_ZERO},
    {"1", NV097_SET_BLEND_FUNC_SFACTOR_V_ONE},
    {"srcRGB", NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_COLOR},
    {"1-srcRGB", NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_SRC_COLOR},
    {"srcA", NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA},
    {"1-srcA", NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_SRC_ALPHA},
    {"dstA", NV097_SET_BLEND_FUNC_SFACTOR_V_DST_ALPHA},
    {"1-dstA", NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_DST_ALPHA},
    {"dstRGB", NV097_SET_BLEND_FUNC_SFACTOR_V_DST_COLOR},
    {"1-dstRGB", NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_DST_COLOR},
    {"srcAsat", NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA_SATURATE},
    {"cRGB", NV097_SET_BLEND_FUNC_SFACTOR_V_CONSTANT_COLOR},
    {"1-cRGB", NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_CONSTANT_COLOR},
    {"cA", NV097_SET_BLEND_FUNC_SFACTOR_V_CONSTANT_ALPHA},
    {"1-cA", NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_CONSTANT_ALPHA},
};

static constexpr auto kNumBlendFactors = sizeof(kBlendFactors) / sizeof(kBlendFactors[0]);

static constexpr uint32_t kTextureSize = 256;
static constexpr uint32_t kTexturePitch = kTextureSize * 4;

static constexpr uint32_t kColorSwatchSize = kTextureSize / 4;
static constexpr uint32_t kColorSwatchTextureHeight = kColorSwatchSize * 4;
static constexpr uint32_t kColorSwatchTexturePitch = kColorSwatchSize * 4;

/**
 * Initializes the test suite and creates test cases.
 */
BlendTests::BlendTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Blend tests", config) {
  for (const auto &test_eqn : kBlendEqns) {
    for (const auto &test_sfactor : kBlendFactors) {
      uint32_t sfactor = test_sfactor.value;

      for (const auto &test_dfactor : kBlendFactors) {
        uint32_t dfactor = test_dfactor.value;
        std::string name = test_sfactor.name;
        name += "_";
        name += test_eqn.name;
        name += "_";
        name += test_dfactor.name;
        if (sfactor != NV097_SET_BLEND_FUNC_SFACTOR_V_ZERO || dfactor != NV097_SET_BLEND_FUNC_SFACTOR_V_ZERO) {
          tests_[name] = [this, name, &test_eqn, sfactor, dfactor]() {
            TestDetailed(name, test_eqn.value, sfactor, dfactor);
          };
          interactive_only_tests_.insert(name);
        }
      }

      std::string name = "#spot_";
      name += test_sfactor.name;
      name += "_";
      name += test_eqn.name;

      tests_[name] = [this, name, &test_eqn, sfactor] { TestSpot(name, test_eqn.value, sfactor); };
    }
  }
}

void BlendTests::Initialize() {
  TestSuite::Initialize();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetCombinerControl(1);

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_ALPHA_TEST_ENABLE, false);
  Pushbuffer::Push(NV097_SET_DEPTH_TEST_ENABLE, false);
  Pushbuffer::End();
}

namespace {
void RenderTexturedQuad(TestHost &host, float left, float top, uint32_t texture_width, uint32_t texture_height) {
  constexpr uint32_t kStage = 1;
  host.SetFinalCombiner0Just(TestHost::SRC_TEX1);
  host.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);
  host.SetTextureStageEnabled(kStage, true);
  host.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);
  auto &texture_stage = host.GetTextureStage(kStage);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  texture_stage.SetTextureDimensions(texture_width, texture_height);
  host.SetupTextureStages();

  const float kRight = left + static_cast<float>(texture_width);
  const float kBottom = top + static_cast<float>(texture_height);

  host.SetBlend(true);

  host.Begin(TestHost::PRIMITIVE_QUADS);
  host.SetTexCoord1(0.0f, 0.0f);
  host.SetVertex(left, top, 0.1f, 1.0f);
  host.SetTexCoord1(1.0f, 0.0f);
  host.SetVertex(kRight, top, 0.1f, 1.0f);
  host.SetTexCoord1(1.0f, 1.0f);
  host.SetVertex(kRight, kBottom, 0.1f, 1.0f);
  host.SetTexCoord1(0.0f, 1.0f);
  host.SetVertex(left, kBottom, 0.1f, 1.0f);
  host.End();

  host.SetTextureStageEnabled(kStage, false);
  host.SetShaderStageProgram(TestHost::STAGE_NONE);
}

void PrepareBlendBackground(TestHost &host, uint32_t texture_width, uint32_t texture_height, uint32_t texture_pitch,
                            uint32_t checkerboard_size = kBlendCheckerboardSize) {
  auto texture_memory = host.GetTextureMemoryForStage(1);
  GenerateSwizzledRGBACheckerboard(texture_memory, 0, 0, texture_width, texture_height, texture_pitch,
                                   kColorSwatchBackground, kCheckerboardB, checkerboard_size);
}

}  // namespace

void BlendTests::TestSpot(const std::string &name, uint32_t blend_function, uint32_t src_factor) {
  host_.SetBlend(false);
  host_.PrepareDraw(0xFE312F31);

  DrawCheckerboardBackground(true);

  host_.SetBlendColorConstant(kBlendColorConstant);

  static constexpr auto kRenderTargetTextureSize = 512;
  static constexpr auto kRenderTargetTexturePitch = kRenderTargetTextureSize * 4;

  RenderToTextureStart(1, kRenderTargetTexturePitch);
  PrepareBlendBackground(host_, kRenderTargetTextureSize, kRenderTargetTextureSize, kRenderTargetTexturePitch, 8);
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kRenderTargetTextureSize,
                                  kRenderTargetTextureSize, true);

  static constexpr float kTopMargin = 64.f;
  float left = 0.f;
  float top = 0.f;
  static constexpr auto kTestsPerRow = 5;
  static constexpr auto kNumRows = (kNumBlendFactors + (kTestsPerRow - 1)) / kTestsPerRow;

  static constexpr auto kSpacing = 4.f;
  static constexpr auto kRowSpace = kSpacing * (kTestsPerRow - 1);
  float test_width = floorf((kRenderTargetTextureSize - kRowSpace) / kTestsPerRow);

  static constexpr auto kColumnSpace = kSpacing * (kNumRows - 1);
  float test_height = floorf((host_.GetFramebufferHeightF() - (kTopMargin + kColumnSpace)) / kNumRows);
  float color_swatch_size = floorf(test_width / 4);
  auto x_index = 0;

  for (auto dst_factor : kBlendFactors) {
    // Draw color swatches with fixed alpha.
    {
      host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
      host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

      DrawColorStack(blend_function, src_factor, dst_factor.value, left, top, color_swatch_size);
      DrawColorAndAlphaStack(blend_function, src_factor, dst_factor.value, left + color_swatch_size, top,
                             color_swatch_size);
      DrawAlphaStack(blend_function, src_factor, dst_factor.value, left + color_swatch_size * 2, top,
                     test_width - (color_swatch_size * 2), 4);
    }

    if (++x_index >= kTestsPerRow) {
      left = 0.f;
      top += test_height;
      x_index = 0;
    } else {
      left += test_width + kSpacing;
    }
  }

  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());

  RenderToTextureEnd();

  RenderTexturedQuad(host_, host_.CenterX(kRenderTargetTextureSize), 64.f, kRenderTargetTextureSize,
                     kRenderTargetTextureSize);

  pb_printat(0, 0, (char *)"%s\n", name.c_str());
  pb_draw_text_screen();

  FinishDraw(name);
}

void BlendTests::TestDetailed(const std::string &name, uint32_t blend_function, uint32_t src_factor,
                              uint32_t dst_factor) {
  host_.SetBlend(false);
  host_.PrepareDraw(0xFE312F31);

  DrawCheckerboardBackground();

  host_.SetBlendColorConstant(kBlendColorConstant);

  {
    RenderToTextureStart(1, kTexturePitch);

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kTextureSize, kTextureSize, true);

    PrepareBlendBackground(host_, kTextureSize, kTextureSize, kTexturePitch);

    DrawAlphaStack(blend_function, src_factor, dst_factor);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());

    RenderToTextureEnd();

    const float kLeft = (host_.GetFramebufferWidthF() - kTextureSize) * 0.5f;
    const float kTop = (host_.GetFramebufferHeightF() - kTextureSize) * 0.5f;
    RenderTexturedQuad(host_, kLeft, kTop, kTextureSize, kTextureSize);
  }

  // Draw color swatches with fixed alpha.
  {
    RenderToTextureStart(1, kColorSwatchTexturePitch);

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kColorSwatchSize,
                                    kColorSwatchTextureHeight, true);

    PrepareBlendBackground(host_, kColorSwatchSize, kColorSwatchTextureHeight, kColorSwatchTexturePitch);

    DrawColorStack(blend_function, src_factor, dst_factor);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());

    RenderToTextureEnd();

    RenderTexturedQuad(host_, 16.0f, (host_.GetFramebufferHeightF() - kColorSwatchTextureHeight) * 0.5f,
                       kColorSwatchSize, kColorSwatchTextureHeight);
  }

  // Draw fully blended swatches.
  {
    RenderToTextureStart(1, kColorSwatchTexturePitch);

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kColorSwatchSize,
                                    kColorSwatchTextureHeight, true);

    PrepareBlendBackground(host_, kColorSwatchSize, kColorSwatchTextureHeight, kColorSwatchTexturePitch);

    DrawColorAndAlphaStack(blend_function, src_factor, dst_factor);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());

    RenderToTextureEnd();

    RenderTexturedQuad(host_, host_.GetFramebufferWidthF() - (16.0f + kColorSwatchSize),
                       (host_.GetFramebufferHeightF() - kColorSwatchTextureHeight) * 0.5f, kColorSwatchSize,
                       kColorSwatchTextureHeight);
  }

  pb_printat(0, 0, (char *)"%s\n", name.c_str());
  pb_printat(1, 0, (char *)"Bg: 0x%X|Black        Constant color: 0x%X\n", kColorSwatchBackground, kBlendColorConstant);

  pb_printat(16, 9, (char *)"Center alphas: G: 0x7F R: 0xFF B: 0x80 W: 0x00\n");
  pb_draw_text_screen();

  FinishDraw(name);
}

void BlendTests::DrawAlphaStack(uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor, float left,
                                float top, float quad_size, float increment) {
  float right = left + quad_size;
  float bottom = top + quad_size;
  DrawQuad(left, top, right, bottom, 0x7F00CC00, blend_function, src_factor, dst_factor, false, true);

  left += increment;
  right -= increment;
  top += increment;
  bottom -= increment;
  DrawQuad(left, top, right, bottom, 0xFFCC0000, blend_function, src_factor, dst_factor, false, true);

  left += increment;
  right -= increment;
  top += increment;
  bottom -= increment;
  DrawQuad(left, top, right, bottom, 0x800000FF, blend_function, src_factor, dst_factor, false, true);

  left += increment;
  right -= increment;
  top += increment;
  bottom -= increment;
  DrawQuad(left, top, right, bottom, 0x00FFFFFF, blend_function, src_factor, dst_factor, false, true);
}

void BlendTests::DrawColorStack(uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor, float left,
                                float top, float color_swatch_size) {
  float right = left + color_swatch_size;
  float bottom = top + color_swatch_size;

  DrawQuad(left, top, right, bottom, 0xDD00DD00, blend_function, src_factor, dst_factor, true, false);

  top += color_swatch_size;
  bottom += color_swatch_size;
  DrawQuad(left, top, right, bottom, 0xDDDD0000, blend_function, src_factor, dst_factor, true, false);

  top += color_swatch_size;
  bottom += color_swatch_size;
  DrawQuad(left, top, right, bottom, 0xDD0000DD, blend_function, src_factor, dst_factor, true, false);

  top += color_swatch_size;
  bottom += color_swatch_size;
  DrawQuad(left, top, right, bottom, 0xDDFFFFFF, blend_function, src_factor, dst_factor, true, false);
}

void BlendTests::DrawColorAndAlphaStack(uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor, float left,
                                        float top, float color_swatch_size) {
  float right = left + color_swatch_size;
  float bottom = top + color_swatch_size;

  auto draw_quad = [this](float left, float top, float right, float bottom, uint32_t color, uint32_t func,
                          uint32_t sfactor, uint32_t dfactor) {
    host_.SetBlend(true, func, sfactor, dfactor);
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(color);
    host_.SetVertex(left, top, 0.1f, 1.0f);
    host_.SetVertex(right, top, 0.1f, 1.0f);
    host_.SetVertex(right, bottom, 0.1f, 1.0f);
    host_.SetVertex(left, bottom, 0.1f, 1.0f);
    host_.End();
  };

  draw_quad(left, top, right, bottom, 0xDDFFFFFF, blend_function, src_factor, dst_factor);

  top += color_swatch_size;
  bottom += color_swatch_size;
  draw_quad(left, top, right, bottom, 0xDD0000DD, blend_function, src_factor, dst_factor);

  top += color_swatch_size;
  bottom += color_swatch_size;
  draw_quad(left, top, right, bottom, 0xDDDD0000, blend_function, src_factor, dst_factor);

  top += color_swatch_size;
  bottom += color_swatch_size;
  draw_quad(left, top, right, bottom, 0xDD00DD00, blend_function, src_factor, dst_factor);
}

void BlendTests::DrawCheckerboardBackground(bool use_small_checkers) const {
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  constexpr uint32_t kCheckerSize = 24;
  constexpr uint32_t kSmallCheckerSize = 16;

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  auto texture_memory = host_.GetTextureMemoryForStage(0);
  GenerateSwizzledRGBACheckerboard(texture_memory, 0, 0, kTextureSize, kTextureSize, kTextureSize * 4, kCheckerboardA,
                                   kCheckerboardB, use_small_checkers ? kSmallCheckerSize : kCheckerSize);

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetTexCoord0(0.0f, 0.0f);
  host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);
  host_.SetTexCoord0(1.0f, 0.0f);
  host_.SetVertex(host_.GetFramebufferWidthF(), 0.0f, 0.1f, 1.0f);
  host_.SetTexCoord0(1.0f, 1.0f);
  host_.SetVertex(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF(), 0.1f, 1.0f);
  host_.SetTexCoord0(0.0f, 1.0f);
  host_.SetVertex(0.0f, host_.GetFramebufferHeightF(), 0.1f, 1.0f);
  host_.End();

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
}

void BlendTests::RenderToTextureStart(uint32_t stage, uint32_t texture_pitch) const {
  const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(stage));
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
  Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, texture_pitch) |
                                                SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
  Pushbuffer::End();
}

void BlendTests::RenderToTextureEnd() const {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
  Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  Pushbuffer::End();
}

void BlendTests::DrawQuad(float left, float top, float right, float bottom, uint32_t color, uint32_t func,
                          uint32_t sfactor, uint32_t dfactor, bool blend_rgb, bool blend_alpha) const {
  host_.SetColorMask(NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE | NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
                     NV097_SET_COLOR_MASK_RED_WRITE_ENABLE);
  if (blend_rgb) {
    host_.SetBlend(true, func, sfactor, dfactor);
  } else {
    host_.SetBlend(false);
  }
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  host_.SetVertex(left, top, 0.1f, 1.0f);
  host_.SetVertex(right, top, 0.1f, 1.0f);
  host_.SetVertex(right, bottom, 0.1f, 1.0f);
  host_.SetVertex(left, bottom, 0.1f, 1.0f);
  host_.End();

  // Set the alpha following the given blend mode.
  host_.SetColorMask(NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE);
  if (blend_alpha) {
    host_.SetBlend(true, func, sfactor, dfactor);
  } else {
    host_.SetBlend(false);
  }
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  host_.SetVertex(left, top, 0.1f, 1.0f);
  host_.SetVertex(right, top, 0.1f, 1.0f);
  host_.SetVertex(right, bottom, 0.1f, 1.0f);
  host_.SetVertex(left, bottom, 0.1f, 1.0f);
  host_.End();

  host_.SetColorMask();
  host_.SetBlend(false);
}
