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

static constexpr const char kSignedAddName[] = "SignedAdd";
static constexpr const char kSignedRevSubtractName[] = "SignedRevSubtract";

static constexpr uint32_t kTextureSize = 256;
static constexpr uint32_t kTexturePitch = kTextureSize * 4;

BlendTests::BlendTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Blend tests", config) {
  tests_[kSignedAddName] = [this]() {
    Test(kSignedAddName, [this]() { Body(NV097_SET_BLEND_EQUATION_V_FUNC_ADD_SIGNED); });
  };
  tests_[kSignedRevSubtractName] = [this]() {
    Test(kSignedRevSubtractName, [this]() { Body(NV097_SET_BLEND_EQUATION_V_FUNC_REVERSE_SUBTRACT_SIGNED); });
  };
}

void BlendTests::Initialize() {
  TestSuite::Initialize();

  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetCombinerControl(1);

  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_ALPHA_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  pb_end(p);
}

void BlendTests::Test(const std::string &name, const std::function<void()> &body) {
  host_.SetBlend(false);
  host_.PrepareDraw(0xFE312F31);

  DrawCheckerboardBackground();

  {
    RenderToTextureStart(1);

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, 256, 256, true);

    // Initialize texture to fully opaque white.
    auto texture_memory = host_.GetTextureMemoryForStage(1);
    memset(texture_memory, 0xFF, kTexturePitch * kTextureSize);

    body();

    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                    host_.GetFramebufferHeight());

    RenderToTextureEnd();
  }

  {
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX1);
    host_.SetFinalCombiner1Just(TestHost::SRC_TEX1, true);
    host_.SetTextureStageEnabled(1, true);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE, TestHost::STAGE_2D_PROJECTIVE);
    auto &texture_stage = host_.GetTextureStage(1);
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    texture_stage.SetTextureDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();

    const float kLeft = (host_.GetFramebufferWidthF() - kTextureSize) * 0.5f;
    const float kRight = kLeft + kTextureSize;
    const float kTop = (host_.GetFramebufferHeightF() - kTextureSize) * 0.5f;
    const float kBottom = kTop + kTextureSize;

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord1(0.0f, 0.0f);
    host_.SetVertex(kLeft, kTop, 0.1f, 1.0f);
    host_.SetTexCoord1(1.0f, 0.0f);
    host_.SetVertex(kRight, kTop, 0.1f, 1.0f);
    host_.SetTexCoord1(1.0f, 1.0f);
    host_.SetVertex(kRight, kBottom, 0.1f, 1.0f);
    host_.SetTexCoord1(0.0f, 1.0f);
    host_.SetVertex(kLeft, kBottom, 0.1f, 1.0f);
    host_.End();

    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  }

  pb_print("%s\n", name.c_str());
  pb_print("G: 0x7F R: 0xFF B: 0x80 W: 0x00\n");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

void BlendTests::Body(uint32_t blend_function) {
  static constexpr float inc = 24.0f;
  float left = 0.0f;
  float right = kTextureSize;
  float top = 0.0f;
  float bottom = kTextureSize;
  Draw(left, top, right, bottom, 0x7F00CC00, blend_function, NV097_SET_BLEND_FUNC_SFACTOR_V_ONE,
       NV097_SET_BLEND_FUNC_DFACTOR_V_ONE);

  left += inc;
  right -= inc;
  top += inc;
  bottom -= inc;
  Draw(left, top, right, bottom, 0xFFCC0000, blend_function, NV097_SET_BLEND_FUNC_SFACTOR_V_ONE,
       NV097_SET_BLEND_FUNC_DFACTOR_V_ONE);

  left += inc;
  right -= inc;
  top += inc;
  bottom -= inc;
  Draw(left, top, right, bottom, 0x800000FF, blend_function, NV097_SET_BLEND_FUNC_SFACTOR_V_ONE,
       NV097_SET_BLEND_FUNC_DFACTOR_V_ONE);

  left += inc;
  right -= inc;
  top += inc;
  bottom -= inc;
  Draw(left, top, right, bottom, 0x00FFFFFF, blend_function, NV097_SET_BLEND_FUNC_SFACTOR_V_ONE,
       NV097_SET_BLEND_FUNC_DFACTOR_V_ONE);
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

void BlendTests::RenderToTextureStart(uint32_t stage) const {
  const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemoryForStage(stage));
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
  p = pb_push1(p, NV097_SET_SURFACE_PITCH,
               SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                   SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
  pb_end(p);
}

void BlendTests::RenderToTextureEnd() const {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
  p = pb_push1(p, NV097_SET_SURFACE_PITCH,
               SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                   SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  pb_end(p);
}

void BlendTests::Draw(float left, float top, float right, float bottom, uint32_t color, uint32_t func, uint32_t sfactor,
                      uint32_t dfactor) const {
  // Draw just the colors with blending disabled so they are predictable.
  host_.SetColorMask(NV097_SET_COLOR_MASK_BLUE_WRITE_ENABLE | NV097_SET_COLOR_MASK_GREEN_WRITE_ENABLE |
                     NV097_SET_COLOR_MASK_RED_WRITE_ENABLE);
  host_.SetBlend(false);
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  host_.SetVertex(left, top, 0.1f, 1.0f);
  host_.SetVertex(right, top, 0.1f, 1.0f);
  host_.SetVertex(right, bottom, 0.1f, 1.0f);
  host_.SetVertex(left, bottom, 0.1f, 1.0f);
  host_.End();

  // Set the alpha following the given blend mode.
  host_.SetColorMask(NV097_SET_COLOR_MASK_ALPHA_WRITE_ENABLE);
  host_.SetBlend(true, func, sfactor, dfactor);
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  host_.SetVertex(left, top, 0.1f, 1.0f);
  host_.SetVertex(right, top, 0.1f, 1.0f);
  host_.SetVertex(right, bottom, 0.1f, 1.0f);
  host_.SetVertex(left, bottom, 0.1f, 1.0f);
  host_.End();

  host_.SetColorMask();
}
