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
      for (const auto &test_dfactor : kBlendFactors) {
        uint32_t sfactor = test_sfactor.value;
        uint32_t dfactor = test_dfactor.value;
        std::string name = test_sfactor.name;
        name += "_";
        name += test_eqn.name;
        name += "_";
        name += test_dfactor.name;
        if (sfactor != NV097_SET_BLEND_FUNC_SFACTOR_V_ZERO || dfactor != NV097_SET_BLEND_FUNC_SFACTOR_V_ZERO) {
          tests_[name] = [this, name, &test_eqn, sfactor, dfactor]() { Test(name, test_eqn.value, sfactor, dfactor); };
        }
      }
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

void BlendTests::Test(const std::string &name, uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor) {
  host_.SetBlend(false);
  host_.PrepareDraw(0xFE312F31);

  DrawCheckerboardBackground();

  host_.SetBlendColorConstant(kBlendColorConstant);

  auto render_textured_quad = [this](float left, float top, uint32_t texture_width, uint32_t texture_height) {
    constexpr uint32_t kStage = 1;
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);
    host_.SetTextureStageEnabled(kStage, true);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);
    auto &texture_stage = host_.GetTextureStage(kStage);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    texture_stage.SetTextureDimensions(texture_width, texture_height);
    host_.SetupTextureStages();

    const float kRight = left + static_cast<float>(texture_width);
    const float kBottom = top + static_cast<float>(texture_height);

    host_.SetBlend(true);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord1(0.0f, 0.0f);
    host_.SetVertex(left, top, 0.1f, 1.0f);
    host_.SetTexCoord1(1.0f, 0.0f);
    host_.SetVertex(kRight, top, 0.1f, 1.0f);
    host_.SetTexCoord1(1.0f, 1.0f);
    host_.SetVertex(kRight, kBottom, 0.1f, 1.0f);
    host_.SetTexCoord1(0.0f, 1.0f);
    host_.SetVertex(left, kBottom, 0.1f, 1.0f);
    host_.End();

    host_.SetTextureStageEnabled(kStage, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  };

  auto prepare_blend_background = [this](uint32_t texture_width, uint32_t texture_height, uint32_t texture_pitch) {
    auto texture_memory = host_.GetTextureMemoryForStage(1);
    GenerateSwizzledRGBACheckerboard(texture_memory, 0, 0, texture_width, texture_height, texture_pitch,
                                     kColorSwatchBackground, kCheckerboardB, kBlendCheckerboardSize);
  };

  {
    RenderToTextureStart(1, kTexturePitch);

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kTextureSize, kTextureSize, true);

    prepare_blend_background(kTextureSize, kTextureSize, kTexturePitch);

    DrawAlphaStack(blend_function, src_factor, dst_factor);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());

    RenderToTextureEnd();

    const float kLeft = (host_.GetFramebufferWidthF() - kTextureSize) * 0.5f;
    const float kTop = (host_.GetFramebufferHeightF() - kTextureSize) * 0.5f;
    render_textured_quad(kLeft, kTop, kTextureSize, kTextureSize);

    while (pb_busy()) {
      /* Wait for completion... */
    }
  }

  // Draw color swatches with fixed alpha.
  {
    RenderToTextureStart(1, kColorSwatchTexturePitch);

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kColorSwatchSize,
                                    kColorSwatchTextureHeight, true);

    prepare_blend_background(kColorSwatchSize, kColorSwatchTextureHeight, kColorSwatchTexturePitch);

    DrawColorStack(blend_function, src_factor, dst_factor);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());

    RenderToTextureEnd();

    render_textured_quad(16.0f, (host_.GetFramebufferHeightF() - kColorSwatchTextureHeight) * 0.5f, kColorSwatchSize,
                         kColorSwatchTextureHeight);
    while (pb_busy()) {
      /* Wait for completion... */
    }
  }

  // Draw fully blended swatches.
  {
    RenderToTextureStart(1, kColorSwatchTexturePitch);

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kColorSwatchSize,
                                    kColorSwatchTextureHeight, true);

    prepare_blend_background(kColorSwatchSize, kColorSwatchTextureHeight, kColorSwatchTexturePitch);

    DrawColorAndAlphaStack(blend_function, src_factor, dst_factor);

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());

    RenderToTextureEnd();

    render_textured_quad(host_.GetFramebufferWidthF() - (16.0f + kColorSwatchSize),
                         (host_.GetFramebufferHeightF() - kColorSwatchTextureHeight) * 0.5f, kColorSwatchSize,
                         kColorSwatchTextureHeight);
    while (pb_busy()) {
      /* Wait for completion... */
    }
  }

  pb_printat(0, 0, (char *)"%s\n", name.c_str());
  pb_printat(1, 0, (char *)"Bg: 0x%X|Black        Constant color: 0x%X\n", kColorSwatchBackground, kBlendColorConstant);

  pb_printat(16, 9, (char *)"Center alphas: G: 0x7F R: 0xFF B: 0x80 W: 0x00\n");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void BlendTests::DrawAlphaStack(uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor) {
  static constexpr float inc = 24.0f;
  float left = 0.0f;
  float right = kTextureSize;
  float top = 0.0f;
  float bottom = kTextureSize;
  DrawQuad(left, top, right, bottom, 0x7F00CC00, blend_function, src_factor, dst_factor, false, true);

  left += inc;
  right -= inc;
  top += inc;
  bottom -= inc;
  DrawQuad(left, top, right, bottom, 0xFFCC0000, blend_function, src_factor, dst_factor, false, true);

  left += inc;
  right -= inc;
  top += inc;
  bottom -= inc;
  DrawQuad(left, top, right, bottom, 0x800000FF, blend_function, src_factor, dst_factor, false, true);

  left += inc;
  right -= inc;
  top += inc;
  bottom -= inc;
  DrawQuad(left, top, right, bottom, 0x00FFFFFF, blend_function, src_factor, dst_factor, false, true);
}

void BlendTests::DrawColorStack(uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor) {
  float left = 0.0f;
  float right = kColorSwatchSize;
  float top = 0.0f;
  float bottom = kColorSwatchSize;

  DrawQuad(left, top, right, bottom, 0xDD00DD00, blend_function, src_factor, dst_factor, true, false);

  top += kColorSwatchSize;
  bottom += kColorSwatchSize;
  DrawQuad(left, top, right, bottom, 0xDDDD0000, blend_function, src_factor, dst_factor, true, false);

  top += kColorSwatchSize;
  bottom += kColorSwatchSize;
  DrawQuad(left, top, right, bottom, 0xDD0000DD, blend_function, src_factor, dst_factor, true, false);

  top += kColorSwatchSize;
  bottom += kColorSwatchSize;
  DrawQuad(left, top, right, bottom, 0xDDFFFFFF, blend_function, src_factor, dst_factor, true, false);
}

void BlendTests::DrawColorAndAlphaStack(uint32_t blend_function, uint32_t src_factor, uint32_t dst_factor) {
  float left = 0.0f;
  float right = kColorSwatchSize;
  float top = 0.0f;
  float bottom = kColorSwatchSize;

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

  top += kColorSwatchSize;
  bottom += kColorSwatchSize;
  draw_quad(left, top, right, bottom, 0xDD0000DD, blend_function, src_factor, dst_factor);

  top += kColorSwatchSize;
  bottom += kColorSwatchSize;
  draw_quad(left, top, right, bottom, 0xDDDD0000, blend_function, src_factor, dst_factor);

  top += kColorSwatchSize;
  bottom += kColorSwatchSize;
  draw_quad(left, top, right, bottom, 0xDD00DD00, blend_function, src_factor, dst_factor);
}

void BlendTests::DrawCheckerboardBackground() const {
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  constexpr uint32_t kCheckerSize = 24;

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
  texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  auto texture_memory = host_.GetTextureMemoryForStage(0);
  GenerateSwizzledRGBACheckerboard(texture_memory, 0, 0, kTextureSize, kTextureSize, kTextureSize * 4, kCheckerboardA,
                                   kCheckerboardB, kCheckerSize);

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

  while (pb_busy()) {
    /* Wait for completion... */
  }

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
