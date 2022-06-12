#include "vertex_shader_rounding_tests.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/perspective_vertex_shader.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"
#include "texture_generator.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
const uint32_t kDefaultDMAChannelA = 3;

// From pbkit.c, DMA_COLOR is set to channel 9 by default
const uint32_t kDefaultDMAColorChannel = 9;

static const uint32_t kTextureWidth = 128;
static const uint32_t kTexturePitch = kTextureWidth * 4;
static const uint32_t kTextureHeight = 128;

static constexpr const char kTestRenderTargetName[] = "RenderTarget";
static constexpr const char kTestCompositingRenderTargetName[] = "Compositing";
static constexpr const char kTestGeometryName[] = "Geometry";
static constexpr const char kTestGeometrySubscreenName[] = "GeometrySubscreen";
static constexpr const char kTestGeometrySuperscreenName[] = "GeometrySuperscreen";
static constexpr const char kTestAdjacentGeometryName[] = "AdjacentGeometry";
static constexpr const char kTestProjectedAdjacentGeometryName[] = "ProjAdjacentGeometry";

static constexpr float kGeometryTestBiases[] = {
    // Boundaries at 1/16 = 0.0625f
    0.0f, 0.001f, 0.49999f, 0.5f, 0.5624f, 0.5625f, 0.5626f, 0.999f, 1.0f,
};

static std::string MakeCompositingRenderTargetTestName(int z);
static std::string MakeGeometryTestName(const char *prefix, float bias);

VertexShaderRoundingTests::VertexShaderRoundingTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Vertex shader rounding tests") {
  tests_[kTestRenderTargetName] = [this]() { TestRenderTarget(); };

  for (auto z : {-4, -2, 2}) {
    tests_[MakeCompositingRenderTargetTestName(z)] = [this, z]() { TestCompositingRenderTarget(z); };
  }

  for (auto bias : kGeometryTestBiases) {
    std::string test_name = MakeGeometryTestName(kTestGeometryName, bias);
    tests_[test_name] = [this, bias]() { TestGeometry(bias); };

    test_name = MakeGeometryTestName(kTestGeometrySubscreenName, bias);
    tests_[test_name] = [this, bias]() { TestGeometrySubscreen(bias); };

    test_name = MakeGeometryTestName(kTestGeometrySuperscreenName, bias);
    tests_[test_name] = [this, bias]() { TestGeometrySuperscreen(bias); };

    test_name = MakeGeometryTestName(kTestAdjacentGeometryName, bias);
    tests_[test_name] = [this, bias]() { TestAdjacentGeometry(bias); };

    test_name = MakeGeometryTestName(kTestProjectedAdjacentGeometryName, bias);
    tests_[test_name] = [this, bias]() { TestProjectedAdjacentGeometry(bias); };
  }
}

void VertexShaderRoundingTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  auto channel = kNextContextChannel;
  const uint32_t texture_target_dma_channel = channel++;

  const uint32_t texture_size = kTexturePitch * kTextureHeight;
  render_target_ =
      (uint8_t *)MmAllocateContiguousMemoryEx(texture_size, 0, MAXRAM, 0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE);
  ASSERT(render_target_ && "Failed to allocate target surface.");
}

void VertexShaderRoundingTests::Deinitialize() {
  TestSuite::Deinitialize();
  if (render_target_) {
    MmFreeContiguousMemory(render_target_);
  }
}

void VertexShaderRoundingTests::CreateGeometry() {
  framebuffer_vertex_buffer_ = host_.AllocateVertexBuffer(6);
  framebuffer_vertex_buffer_->DefineBiTri(0, -1.75, 1.75, 1.75, -1.75, 0.1f);
  framebuffer_vertex_buffer_->Linearize(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF());
}

void VertexShaderRoundingTests::TestGeometry(float bias) {
  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.PrepareDraw(0xFE404040);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  const float height = floorf(host_.GetFramebufferHeightF() / 4.0f) * 2.0f;

  const float left = floorf((host_.GetFramebufferWidthF() - height) / 2.0f);
  const float top = floorf(height / 2.0f);
  const float right = left + height;
  const float bottom = top + height;
  const float z = 1;

  // Draw a background.
  uint32_t color = 0xFFFF00FF;
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  host_.SetVertex(left, top, z, 1.0f);
  host_.SetVertex(right, top, z, 1.0f);
  host_.SetVertex(right, bottom, z, 1.0f);
  host_.SetVertex(left, bottom, z, 1.0f);
  host_.End();

  // Draw a subpixel offset green square.
  color = 0xFF009900;
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  host_.SetVertex(left + bias, top + bias, z, 1.0f);
  host_.SetVertex(right, top + bias, z, 1.0f);
  host_.SetVertex(right, bottom, z, 1.0f);
  host_.SetVertex(left + bias, bottom, z, 1.0f);
  host_.End();

  std::string test_name = MakeGeometryTestName(kTestGeometryName, bias);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

void VertexShaderRoundingTests::TestGeometrySubscreen(float bias) {
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.PrepareDraw(0xFE414041);

  auto draw_texture = [this, bias](uint32_t texture_width, uint32_t texture_height) {
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetupTextureStages();

    const uint32_t kTexturePitch = texture_width * 4;
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, texture_width, texture_height);
    host_.CommitSurfaceFormat();
    host_.SetWindowClip(texture_width - 1, texture_height - 1);

    // Point the color buffer at the texture and mix the left hand side with itself multiple times.
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(host_.GetTextureMemory()));
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);

    // Initialize the texture to magenta.
    auto pixel = reinterpret_cast<uint32_t *>(host_.GetTextureMemory());
    for (uint32_t y = 0; y < texture_height; ++y) {
      for (uint32_t x = 0; x < texture_width; ++x, ++pixel) {
        *pixel = 0xFFFF00FF;
      }
    }

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

    // Draw a subpixel offset green square.
    static constexpr uint32_t kColor = 0xFF009900;
    static constexpr float kZ = 0.0f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(kColor);
    host_.SetVertex(bias, bias, kZ, 1.0f);
    host_.SetVertex(static_cast<float>(texture_width) + bias, bias, kZ, 1.0f);
    host_.SetVertex(static_cast<float>(texture_width) + bias, static_cast<float>(texture_height) + bias, kZ, 1.0f);
    host_.SetVertex(bias, static_cast<float>(texture_height) + bias, kZ, 1.0f);
    host_.End();

    host_.SetWindowClip(host_.GetFramebufferWidth() - 1, host_.GetFramebufferHeight() - 1);
    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                           host_.GetFramebufferHeight());
    host_.CommitSurfaceFormat();

    p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);
  };

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8));

  auto draw_quad = [this, draw_texture](uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    draw_texture(width, height);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetImageDimensions(width, height);
    host_.SetupTextureStages();

    host_.SetDiffuse(0xFFFFFF00);

    const auto kWidth = static_cast<float>(width);
    const auto kHeight = static_cast<float>(height);
    const auto kLeft = static_cast<float>(x);
    const auto kTop = static_cast<float>(y);
    const auto kRight = kLeft + kWidth;
    const auto kBottom = kTop + kHeight;

    static constexpr float kZ = 0.0f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(kLeft, kTop, kZ, 1.0f);
    host_.SetTexCoord0(kWidth, 0.0f);
    host_.SetVertex(kRight, kTop, kZ, 1.0f);
    host_.SetTexCoord0(kWidth, kHeight);
    host_.SetVertex(kRight, kBottom, kZ, 1.0f);
    host_.SetTexCoord0(0.0f, kHeight);
    host_.SetVertex(kLeft, kBottom, kZ, 1.0f);
    host_.End();
  };

  const float kQuadRegion = 128.0f;
  const float kSpacing = 16.0f;
  const float kHorizontalStart = floorf((host_.GetFramebufferWidthF() - (kQuadRegion + kSpacing)) * 0.5f);
  const float kHorizontalEnd = kHorizontalStart * 2;
  float left = kHorizontalStart;
  float top = floorf((host_.GetFramebufferHeightF() - (kQuadRegion + kSpacing)) * 0.5f);

  static constexpr uint32_t kGeometries[][2] = {
      {128, 128},
      {128, 32},
      {32, 128},
      {16, 64},
  };

  for (auto geometry : kGeometries) {
    draw_quad(static_cast<uint32_t>(left), static_cast<uint32_t>(top), geometry[0], geometry[1]);
    left += kQuadRegion + kSpacing;
    if (left >= kHorizontalEnd) {
      left = kHorizontalStart;
      top += kQuadRegion + kSpacing;
    }
  }

  std::string test_name = MakeGeometryTestName(kTestGeometrySubscreenName, bias);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

void VertexShaderRoundingTests::TestGeometrySuperscreen(float bias) {
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.PrepareDraw(0xFE414041);

  static constexpr uint32_t kTextureSize = 1024;

  auto draw_texture = [this, bias](uint32_t draw_width, uint32_t draw_height) {
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetupTextureStages();

    const uint32_t kTexturePitch = kTextureSize * 4;
    const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, kTextureSize, kTextureSize);
    host_.CommitSurfaceFormat();
    host_.SetWindowClip(kTextureSize - 1, kTextureSize - 1);

    // Point the color buffer at the texture and mix the left hand side with itself multiple times.
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(host_.GetTextureMemory()));
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);

    // Initialize the texture to magenta.
    auto pixel = reinterpret_cast<uint32_t *>(host_.GetTextureMemory());
    for (uint32_t y = 0; y < draw_height; ++y) {
      for (uint32_t x = 0; x < draw_width; ++x, ++pixel) {
        *pixel = 0xFFFF00FF;
      }
    }

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

    // Draw a subpixel offset green square.
    static constexpr uint32_t kColor = 0xFF009900;
    static constexpr float kZ = 0.0f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(kColor);
    host_.SetVertex(bias, bias, kZ, 1.0f);
    host_.SetVertex(static_cast<float>(draw_width) + bias, bias, kZ, 1.0f);
    host_.SetVertex(static_cast<float>(draw_width) + bias, static_cast<float>(draw_height) + bias, kZ, 1.0f);
    host_.SetVertex(bias, static_cast<float>(draw_height) + bias, kZ, 1.0f);
    host_.End();

    host_.SetWindowClip(host_.GetFramebufferWidth() - 1, host_.GetFramebufferHeight() - 1);
    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z16, host_.GetFramebufferWidth(),
                           host_.GetFramebufferHeight());
    host_.CommitSurfaceFormat();

    p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);
  };

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8));

  auto draw_quad = [this, draw_texture](uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    draw_texture(width, height);

    host_.SetTextureStageEnabled(0, true);
    host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
    host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
    host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetImageDimensions(kTextureSize, kTextureSize);
    host_.SetupTextureStages();

    host_.SetDiffuse(0xFFFFFF00);

    const auto kWidth = static_cast<float>(width);
    const auto kHeight = static_cast<float>(height);
    const auto kLeft = static_cast<float>(x);
    const auto kTop = static_cast<float>(y);
    const auto kRight = kLeft + kWidth;
    const auto kBottom = kTop + kHeight;

    static constexpr float kZ = 0.0f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(kLeft, kTop, kZ, 1.0f);
    host_.SetTexCoord0(kWidth, 0.0f);
    host_.SetVertex(kRight, kTop, kZ, 1.0f);
    host_.SetTexCoord0(kWidth, kHeight);
    host_.SetVertex(kRight, kBottom, kZ, 1.0f);
    host_.SetTexCoord0(0.0f, kHeight);
    host_.SetVertex(kLeft, kBottom, kZ, 1.0f);
    host_.End();
  };

  const float kQuadRegion = 128.0f;
  const float kSpacing = 16.0f;
  const float kHorizontalStart = floorf((host_.GetFramebufferWidthF() - (kQuadRegion + kSpacing)) * 0.5f);
  const float kHorizontalEnd = kHorizontalStart * 2;
  float left = kHorizontalStart;
  float top = floorf((host_.GetFramebufferHeightF() - (kQuadRegion + kSpacing)) * 0.5f);

  static constexpr uint32_t kGeometries[][2] = {
      {128, 128},
      {128, 32},
      {32, 128},
      {16, 64},
  };

  for (auto geometry : kGeometries) {
    draw_quad(static_cast<uint32_t>(left), static_cast<uint32_t>(top), geometry[0], geometry[1]);
    left += kQuadRegion + kSpacing;
    if (left >= kHorizontalEnd) {
      left = kHorizontalStart;
      top += kQuadRegion + kSpacing;
    }
  }

  std::string test_name = MakeGeometryTestName(kTestGeometrySuperscreenName, bias);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

void VertexShaderRoundingTests::TestRenderTarget() {
  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;
  {
    auto *pixel = reinterpret_cast<uint32_t *>(render_target_);
    for (uint32_t i = 0; i < kTextureWidth * kTextureHeight; ++i) {
      *pixel++ = 0x7FFF00FF;
    }
  }

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.PrepareDraw(0xFE202020);

  // Redirect the color output to the target texture.
  {
    auto p = pb_begin();
    p = pb_push1(
        p, NV097_SET_SURFACE_PITCH,
        SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) | SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kTexturePitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(render_target_));
    // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);

    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kTextureWidth, kTextureHeight, true);
    host_.CommitSurfaceFormat();

    host_.SetWindowClip(kTextureWidth - 1, kTextureHeight - 1);

    // This is what triggers the erroneous (rounding error) offset
    host_.SetViewportOffset(320.531250f, 240.531250f, 0.0f, 0.0f);
    host_.SetViewportScale(320.0f, -240.0f, 16777215.0f, 0.0f);
    host_.SetDepthClip(0.0f, 16777215.0f);
  }

  // Manually load the vertex shader.
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0x0);
    p = pb_push1(
        p, NV097_SET_TRANSFORM_EXECUTION_MODE,
        MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
            MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);

    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0x0);

    //  MOV(oD0,xyzw, v3);
    p = pb_push4(p, NV097_SET_TRANSFORM_PROGRAM, 0x00000000, 0x0020061B, 0x0836106C, 0x2070F818);

    //  MOV(oPos,xyzw, v0);
    p = pb_push4(p, NV097_SET_TRANSFORM_PROGRAM, 0x00000000, 0x0020001B, 0x0836106C, 0x2070F800);

    //  MUL(oPos,xyz, R12, c[58]);
    //  RCC(R1,x, R12.w);
    p = pb_push4(p, NV097_SET_TRANSFORM_PROGRAM, 0x00000000, 0x0647401B, 0xC4361BFF, 0x1078E800);

    // MAD(oPos,xyz, R12, R1.x, c[59]);
    p = pb_push4(p, NV097_SET_TRANSFORM_PROGRAM, 0x00000000, 0x0087601B, 0xC400286C, 0x3070E801);
    pb_end(p);
  }

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  // Render a full surface quad into the render target.
  {
    const float kLeft = -1.0f;
    const float kRight = 1.0f;
    const float kTop = 1.0f;
    const float kBottom = -1.0f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFF887733);
    host_.SetVertex(kLeft, kTop, 0.1f, 1.0f);

    host_.SetVertex(kRight, kTop, 0.1f, 1.0f);

    host_.SetVertex(kRight, kBottom, 0.1f, 1.0f);

    host_.SetVertex(kLeft, kBottom, 0.1f, 1.0f);
    host_.End();
  }

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);

  // Render the render buffer from the previous draw call onto the screen.
  {
    host_.SetVertexShaderProgram(nullptr);
    host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

    host_.SetWindowClip(host_.GetFramebufferWidth() - 1, host_.GetFramebufferHeight() - 1);
    host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8));
    auto &texture_stage = host_.GetTextureStage(0);
    texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
    texture_stage.SetImageDimensions(kTextureWidth, kTextureHeight);
    host_.SetupTextureStages();

    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    pb_end(p);
    host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                           host_.GetFramebufferHeight());

    host_.SetRawTexture(render_target_, kTextureWidth, kTextureHeight, 1, kTexturePitch, 4, false);

    host_.SetVertexBuffer(framebuffer_vertex_buffer_);

    host_.PrepareDraw(0xFE202020);
    host_.DrawArrays();
  }

  pb_print("%s\n", kTestRenderTargetName);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kTestRenderTargetName);
}

void VertexShaderRoundingTests::TestCompositingRenderTarget(int z) {
  static constexpr uint32_t kNumCompositingIterations = 4;
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  {
    memset(host_.GetTextureMemory(), 0, host_.GetTextureMemorySize());
    static constexpr uint32_t kCheckerboardWidth = 256;
    static constexpr uint32_t kCheckerboardHeight = 256;
    uint32_t x = (host_.GetFramebufferWidth() - kCheckerboardWidth) / 2;
    uint32_t y = (host_.GetFramebufferHeight() - kCheckerboardHeight) / 2;
    GenerateRGBACheckerboard(host_.GetTextureMemory(), x, y, kCheckerboardWidth, kCheckerboardHeight,
                             kFramebufferPitch);
  }

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());
  host_.PrepareDraw(0xFE202020);

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTextureDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  texture_stage.SetImageDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetupTextureStages();

  host_.SetWindowClip(host_.GetFramebufferWidth() - 1, host_.GetFramebufferHeight() - 1);

  // Point the color buffer at the texture and mix the left hand side with itself multiple times.
  {
    auto p = pb_begin();
    p = pb_push1(p, NV097_SET_SURFACE_PITCH,
                 SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                     SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAChannelA);
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF);

    // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
    p = pb_push1(p, NV097_NO_OPERATION, 0);
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    pb_end(p);

    host_.SetCombinerFactorC1(0, 0.0f, 0.0f, 0.0f, 0.5f);
    host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_TEX0), TestHost::OneInput());
    host_.SetOutputColorCombiner(0, TestHost::DST_R0);
    host_.SetInputAlphaCombiner(0, TestHost::AlphaInput(TestHost::SRC_TEX0), TestHost::AlphaInput(TestHost::SRC_C1));
    host_.SetOutputAlphaCombiner(0, TestHost::DST_R0);
    host_.SetFinalCombiner0Just(TestHost::SRC_R0);
    host_.SetFinalCombiner1Just(TestHost::SRC_R0, true);

    const float kLeft = 0.0f;
    const float kRight = floorf(host_.GetFramebufferWidthF() * 0.5f);
    const float kTop = 0.0f;
    const float kBottom = host_.GetFramebufferHeightF();

    for (uint32_t i = 0; i < kNumCompositingIterations; ++i) {
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetTexCoord0(kLeft, kTop);
      host_.SetVertex(kLeft, kTop, 0, 1.0f);

      host_.SetTexCoord0(kRight, kTop);
      host_.SetVertex(kRight, kTop, 0, 1.0f);

      host_.SetTexCoord0(kRight, kBottom);
      host_.SetVertex(kRight, kBottom, 0, 1.0f);

      host_.SetTexCoord0(kLeft, kBottom);
      host_.SetVertex(kLeft, kBottom, 0, 1.0f);
      host_.End();
    }
  }

  // Restore the color buffer and render one last time to the screen.
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
  // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
  p = pb_push1(p, NV097_NO_OPERATION, 0);
  p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
  pb_end(p);

  {
    const float left = -1.75;
    const float top = 1.75;
    const float right = 1.75;
    const float bottom = -1.75;
    const auto depth = static_cast<float>(z);

    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetTexCoord0(0.0f, 0.0f);
    host_.SetVertex(left, top, depth, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF(), 0.0f);
    host_.SetVertex(right, top, depth, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF());
    host_.SetVertex(right, bottom, depth, 1.0f);

    host_.SetTexCoord0(0.0f, host_.GetFramebufferHeightF());
    host_.SetVertex(left, bottom, depth, 1.0f);
    host_.End();
  }

  std::string name = MakeCompositingRenderTargetTestName(z);
  pb_print("%s\n", name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}

void VertexShaderRoundingTests::TestAdjacentGeometry(float bias) {
  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.PrepareDraw(0xFE404040);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  static constexpr float kQuadSize = 100.0f;

  const float kMidX = floorf(host_.GetFramebufferWidthF() * 0.5f);
  const float kMidY = floor(host_.GetFramebufferHeightF() * 0.5f);
  const float kRowStart = kMidX - (kQuadSize * 2);
  float left = kRowStart;
  float top = kMidY - kQuadSize;
  const float kZ = 1;
  const float kBackgroundZ = kZ + 0.0001f;

  // Draw a background.
  uint32_t color = 0xFFE0E0E0;
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  host_.SetVertex(left, top, kBackgroundZ, 1.0f);
  host_.SetVertex(left + kQuadSize * 4.0f, top, kBackgroundZ, 1.0f);
  host_.SetVertex(left + kQuadSize * 4.0f, top + kQuadSize * 2.0f, kBackgroundZ, 1.0f);
  host_.SetVertex(left, top + kQuadSize * 2.0f, kBackgroundZ, 1.0f);
  host_.End();

  // Draw a subpixel offset green square.
  color = 0xFF009900;
  for (uint32_t y = 0; y < 2; ++y) {
    float bottom = top + kQuadSize;
    for (uint32_t x = 0; x < 4; ++x) {
      float right = left + kQuadSize;
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetDiffuse(color);
      host_.SetVertex(left + bias, top + bias, kZ, 1.0f);
      host_.SetVertex(right + bias, top + bias, kZ, 1.0f);
      host_.SetVertex(right + bias, bottom + bias, kZ, 1.0f);
      host_.SetVertex(left + bias, bottom + bias, kZ, 1.0f);
      host_.End();
      left += kQuadSize;
    }
    left = kRowStart;
    top = bottom;
  }

  std::string test_name = MakeGeometryTestName(kTestAdjacentGeometryName, bias);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

void VertexShaderRoundingTests::TestProjectedAdjacentGeometry(float bias) {
  float depth_buffer_max_value = host_.GetMaxDepthBufferValue();
  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                                          0.0f, depth_buffer_max_value, M_PI * 0.25f, -1.0f, 1.0f, 1.0f,
                                                          -1.0f, 1.0f, 200.0f);
  {
    shader->SetLightingEnabled(false);
    shader->SetUse4ComponentTexcoords();
    shader->SetUseD3DStyleViewport();
    VECTOR camera_position = {0.0f, 0.0f, -7.0f, 1.0f};
    VECTOR camera_look_at = {0.0f, 0.0f, 0.0f, 1.0f};
    shader->LookAt(camera_position, camera_look_at);
  }
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetVertexShaderProgram(shader);

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.PrepareDraw(0xFE404040);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  static constexpr float kQuadSize = 100.0f;

  const float kMidX = floorf(host_.GetFramebufferWidthF() * 0.5f);
  const float kMidY = floor(host_.GetFramebufferHeightF() * 0.5f);
  const float kRowStart = kMidX - (kQuadSize * 2);
  float left = kRowStart;
  float top = kMidY - kQuadSize;
  const float kZLeft = 0.0f;
  const float kZRight = 10.0f;
  const float kBackgroundZLeft = kZLeft + 0.0001f;
  const float kBackgroundZRight = kZRight + 0.0001f;

  auto set_vertex = [this](float x, float y, float z) {
    VECTOR world;
    VECTOR screen_point = {x, y, z, 1.0f};
    host_.UnprojectPoint(world, screen_point, z);
    host_.SetVertex(world[0], world[1], z, 1.0f);
  };

  // Draw a background.
  uint32_t color = 0xFFE0E0E0;
  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetDiffuse(color);
  set_vertex(left, top, kBackgroundZLeft);
  set_vertex(left + kQuadSize * 4.0f, top, kBackgroundZRight);
  set_vertex(left + kQuadSize * 4.0f, top + kQuadSize * 2.0f, kBackgroundZRight);
  set_vertex(left, top + kQuadSize * 2.0f, kBackgroundZLeft);
  host_.End();

  float z_inc = kZRight - kZLeft * 0.25f;
  float left_z = kZLeft;

  // Draw a subpixel offset green square.
  color = 0xFF009900;
  for (uint32_t y = 0; y < 2; ++y) {
    float bottom = top + kQuadSize;
    for (uint32_t x = 0; x < 4; ++x) {
      float right = left + kQuadSize;
      float right_z = kZLeft + z_inc;
      host_.Begin(TestHost::PRIMITIVE_QUADS);
      host_.SetDiffuse(color);
      set_vertex(left + bias, top + bias, left_z);
      set_vertex(right + bias, top + bias, right_z);
      set_vertex(right + bias, bottom + bias, right_z);
      set_vertex(left + bias, bottom + bias, left_z);
      host_.End();
      left += kQuadSize;
      left_z = right_z;
    }
    left = kRowStart;
    left_z = kZLeft;
    top = bottom;
  }

  std::string test_name = MakeGeometryTestName(kTestProjectedAdjacentGeometryName, bias);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

static std::string MakeGeometryTestName(const char *prefix, float bias) {
  char buf[32] = {0};
  snprintf(buf, sizeof(buf), "%s_%.04f", prefix, bias);
  return buf;
}

static std::string MakeCompositingRenderTargetTestName(int z) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%s_%d", kTestCompositingRenderTargetName, z);
  return buf;
}
