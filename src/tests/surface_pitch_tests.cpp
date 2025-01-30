#include "surface_pitch_tests.h"

#include "shaders/precalculated_vertex_shader.h"
#include "texture_generator.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;

// From pbkit.c, DMA_COLOR is set to channel 9 by default
static constexpr uint32_t kDefaultDMAColorChannel = 9;

static constexpr uint32_t kTextureSize = 128;
static constexpr uint32_t kTexturePitch = kTextureSize * 4;
static constexpr uint32_t kSmallTextureSize = kTextureSize / 2;
static constexpr uint32_t kSmallTexturePitch = kSmallTextureSize * 4;

static constexpr const char kSwizzlePitchTest[] = "Swizzle";

SurfacePitchTests::SurfacePitchTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "Surface pitch", config) {
  tests_[kSwizzlePitchTest] = [this]() { TestSwizzle(); };
}

void SurfacePitchTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
}

void SurfacePitchTests::TestSwizzle() {
  host_.PrepareDraw(0xFC212111);

  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  const auto kTextureMemory = reinterpret_cast<uint32_t>(host_.GetTextureMemory());
  const uint32_t kTextureTargets[] = {
      kTextureMemory,
      kTextureMemory + kTexturePitch * kTextureSize,
      kTextureMemory + kTexturePitch * kTextureSize * 2,
      kTextureMemory + kTexturePitch * kTextureSize * 3,
  };
  const uint32_t kInnerTextureMemory = kTextureTargets[3] + kTexturePitch * kTextureSize;

  // Set up linear surface render targets with the full texture size, filling each with a solid colored quad.
  {
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kTextureSize, kTextureSize, false);

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(kTextureMemory));
    pb_end(p);

    for (auto addr : kTextureTargets) {
      auto p = pb_begin();
      p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(addr));
      pb_end(p);

      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetDiffuse(0xFF00AA00);
      host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);
      host_.SetVertex(kTextureSize, 0.0f, 0.1f, 1.0f);
      host_.SetVertex(kTextureSize, kTextureSize, 0.1f, 1.0f);
      host_.SetVertex(0.0f, kTextureSize, 0.1f, 1.0f);
      host_.End();
    }
  }

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
  texture_stage.SetImageDimensions(kSmallTextureSize, kSmallTextureSize);
  host_.SetupTextureStages();

  auto draw_inner_quad = [this, kInnerTextureMemory](uint32_t color, uint32_t color_surface_addr) {
    GenerateSwizzledRGBACheckerboard((void *)kInnerTextureMemory, 0, 0, kSmallTextureSize, kSmallTextureSize,
                                     kSmallTexturePitch, color, 0x00000000, 4);

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_TEXTURE_OFFSET, VRAM_ADDR(kInnerTextureMemory));
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(color_surface_addr));
    pb_end(p);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(0.0f, 0.0f, 0.1f, 1.0f);

    host_.SetTexCoord0(kSmallTextureSize, 0.0f);
    host_.SetVertex(kSmallTextureSize, 0.0f, 0.1f, 1.0f);

    host_.SetTexCoord0(kSmallTextureSize, kSmallTextureSize);
    host_.SetVertex(kSmallTextureSize, kSmallTextureSize, 0.1f, 1.0f);

    host_.SetTexCoord0(0.0f, kSmallTextureSize);
    host_.SetVertex(0.0f, kSmallTextureSize, 0.1f, 1.0f);
    host_.End();
  };

  // Swizzled surface smaller than the original with pitch matching the outer surface.
  {
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kSmallTextureSize, kSmallTextureSize,
                                    true);
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    pb_end(p);

    draw_inner_quad(0xFFFF2222, kTextureTargets[0]);
  }

  // Swizzled surface smaller than the original with pitch matching the smaller surface.
  {
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kSmallTextureSize, kSmallTextureSize,
                                    true);
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kSmallTexturePitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    pb_end(p);

    draw_inner_quad(0xFFFF2277, kTextureTargets[1]);
  }

  // Swizzled surface matching the original with pitch matching the outer surface.
  {
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kTextureSize, kTextureSize, true);
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    pb_end(p);

    draw_inner_quad(0xFF2222FF, kTextureTargets[2]);
  }

  // Swizzled surface matching the original with pitch matching the smaller surface.
  {
    host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kTextureSize, kTextureSize, true);
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kSmallTexturePitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    pb_end(p);

    draw_inner_quad(0xFF7722FF, kTextureTargets[3]);
  }

  DrawResults(kTextureTargets, kInnerTextureMemory);

  pb_draw_text_screen();
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kSwizzlePitchTest);
}

void SurfacePitchTests::DrawResults(const uint32_t *result_textures, const uint32_t demo_memory) const {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight());

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
  texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
  host_.SetupTextureStages();

  // Draw textured quads containing each of the results.
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    pb_end(p);

    auto draw_quad = [this](float left, float right, float top, float bottom, uint32_t texture_addr) {
      auto p = pb_begin();
      p = pb_push1(p, NV097_SET_TEXTURE_OFFSET, VRAM_ADDR(texture_addr));
      pb_end(p);

      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord0(0.0f, 0.0f);
      host_.SetVertex(left, top, 0.1f, 1.0f);

      host_.SetTexCoord0(kTextureSize, 0.0f);
      host_.SetVertex(right, top, 0.1f, 1.0f);

      host_.SetTexCoord0(kTextureSize, kTextureSize);
      host_.SetVertex(right, bottom, 0.1f, 1.0f);

      host_.SetTexCoord0(0.0f, kTextureSize);
      host_.SetVertex(left, bottom, 0.1f, 1.0f);
      host_.End();
    };

    const float start = (host_.GetFramebufferWidthF() - (kTextureSize * 2)) / 3;
    float left = start;
    float right = left + kTextureSize;
    float top = (host_.GetFramebufferHeightF() - (kTextureSize * 2)) / 3;
    float bottom = top + kTextureSize;

    draw_quad(left, right, top, bottom, result_textures[0]);

    left = (left * 2) + kTextureSize;
    right = left + kTextureSize;
    draw_quad(left, right, top, bottom, result_textures[1]);

    top = (top * 2) + kTextureSize;
    bottom = top + kTextureSize;
    left = start;
    right = left + kTextureSize;
    draw_quad(left, right, top, bottom, result_textures[2]);

    left = (left * 2) + kTextureSize;
    right = left + kTextureSize;
    draw_quad(left, right, top, bottom, result_textures[3]);
  }

  // Draw a final demo quad of the checkerboard swizzled texture in the center.
  {
    texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8B8G8R8));
    texture_stage.SetTextureDimensions(kSmallTextureSize, kSmallTextureSize);
    host_.SetupTextureStages();

    GenerateSwizzledRGBACheckerboard((void *)demo_memory, 0, 0, kSmallTextureSize, kSmallTextureSize,
                                     kSmallTexturePitch, 0xFFFFFFFF, 0xFF000000, 4);

    const float left = (host_.GetFramebufferWidthF() - kSmallTextureSize) * 0.5f;
    const float right = left + kSmallTextureSize;
    const float top = (host_.GetFramebufferHeightF() - kSmallTextureSize) * 0.5f;
    const float bottom = top + kSmallTextureSize;
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_TEXTURE_OFFSET, VRAM_ADDR(demo_memory));
    pb_end(p);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(left, top, 0.1f, 1.0f);

    host_.SetTexCoord0(1.0f, 0.0f);
    host_.SetVertex(right, top, 0.1f, 1.0f);

    host_.SetTexCoord0(1.0f, 1.0f);
    host_.SetVertex(right, bottom, 0.1f, 1.0f);

    host_.SetTexCoord0(0.0f, 1.0f);
    host_.SetVertex(left, bottom, 0.1f, 1.0f);
    host_.End();

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  }

  pb_printat(0, 22, (char *)"Pitch");
  pb_printat(1, 15, (char *)"P%d*4", kTextureSize);
  pb_printat(1, 40, (char *)"P%d*4", kSmallTextureSize);
  pb_printat(3, 0, (char *)"Size");
  pb_printat(4, 0, (char *)"%dx%d", kSmallTextureSize, kSmallTextureSize);
  pb_printat(9, 15, (char *)"P%d*4", kTextureSize);
  pb_printat(9, 40, (char *)"P%d*4", kSmallTextureSize);
  pb_printat(12, 0, (char *)"%dx%d", kTextureSize, kTextureSize);
}
