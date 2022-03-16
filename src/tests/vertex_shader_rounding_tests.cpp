#include "vertex_shader_rounding_tests.h"

#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_COLOR is set to channel 9 by default
const uint32_t kDefaultDMAColorChannel = 9;

static const uint32_t kTextureWidth = 128;
static const uint32_t kTexturePitch = kTextureWidth * 4;
static const uint32_t kTextureHeight = 128;

static constexpr const char kTestRenderTargetName[] = "RenderTarget";
static constexpr const char kTestGeometryName[] = "Geometry";

static constexpr float kGeometryTestBiases[] = {
    // Boundaries at 1/16 = 0.0625f
    0.0f, 0.001f, 0.5f, 0.5624f, 0.5625f, 0.5626f, 0.999f,
};

VertexShaderRoundingTests::VertexShaderRoundingTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Vertex shader rounding tests") {
  tests_[kTestRenderTargetName] = [this]() { TestRenderTarget(); };

  for (auto bias : kGeometryTestBiases) {
    std::string test_name = MakeGeometryTestName(bias);
    tests_[test_name] = [this, bias]() { TestGeometry(bias); };
  }
}

void VertexShaderRoundingTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  auto channel = kNextContextChannel;
  const uint32_t texture_target_dma_channel = channel++;
  pb_create_dma_ctx(texture_target_dma_channel, DMA_CLASS_3D, 0, MAXRAM, &texture_target_ctx_);
  pb_bind_channel(&texture_target_ctx_);

  const uint32_t texture_size = kTexturePitch * kTextureHeight;
  render_target_ =
      (uint8_t *)MmAllocateContiguousMemoryEx(texture_size, 0, MAXRAM, 0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE);
  ASSERT(render_target_ && "Failed to allocate target surface.");
  auto *pixel = reinterpret_cast<uint32_t *>(render_target_);
  for (uint32_t i = 0; i < kTextureWidth * kTextureHeight; ++i) {
    *pixel++ = 0x7FFF00FF;
  }
  pb_set_dma_address(&texture_target_ctx_, render_target_, texture_size - 1);

  host_.SetCombinerControl(1, true, true);
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
}

void VertexShaderRoundingTests::Deinitialize() {
  TestSuite::Deinitialize();
  if (render_target_) {
    MmFreeContiguousMemory(render_target_);
  }
}

void VertexShaderRoundingTests::CreateGeometry() {
  const auto fb_width = static_cast<float>(host_.GetFramebufferWidth());
  const auto fb_height = static_cast<float>(host_.GetFramebufferHeight());

  render_target_vertex_buffer_ = host_.AllocateVertexBuffer(6);
  render_target_vertex_buffer_->DefineBiTri(0, -1.0f, 1.0f, 1.0f, -1.0f, 0.1f);
  render_target_vertex_buffer_->Linearize(kTextureWidth, kTextureHeight);

  framebuffer_vertex_buffer_ = host_.AllocateVertexBuffer(6);
  framebuffer_vertex_buffer_->DefineBiTri(0, -1.75, 1.75, 1.75, -1.75, 0.1f);
  framebuffer_vertex_buffer_->Linearize(fb_width, fb_height);
}

void VertexShaderRoundingTests::TestRenderTarget() {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_X8R8G8B8));

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTextureDimensions(host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight());

  const uint32_t kSourceTextureSize = host_.GetMaxTextureWidth() * host_.GetMaxTextureHeight();
  auto *source_texture = new uint32_t[kSourceTextureSize];
  auto *pixel = source_texture;
  for (uint32_t i = 0; i < kSourceTextureSize; ++i) {
    *pixel++ = 0xFF00AAFF;
  }
  host_.SetRawTexture(reinterpret_cast<uint8_t *>(source_texture), host_.GetMaxTextureWidth(),
                      host_.GetMaxTextureHeight(), 1, host_.GetMaxTextureWidth() * 4, 4, false);
  delete[] source_texture;

  host_.PrepareDraw(0xFE202020);

  uint32_t *p;
  // Redirect the color output to the target texture.
  p = pb_begin();
  p = pb_push1(
      p, NV097_SET_SURFACE_PITCH,
      SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) | SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kTexturePitch));
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, texture_target_ctx_.ChannelID);
  p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
  // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
  p = pb_push1(p, NV097_NO_OPERATION, 0);
  p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
  pb_end(p);

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);

  host_.SetWindowClip(kTextureWidth - 1, kTextureHeight - 1);

  // This is what triggers the erroneous (rounding error) offset
  host_.SetViewportOffset(320.531250f, 240.531250f, 0.0f, 0.0f);
  host_.SetViewportScale(320.0f, -240.0f, 16777215.0f, 0.0f);
  host_.SetDepthClip(0.0f, 16777215.0f);

  p = pb_begin();
  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0x0);

  //  MOV(oPos,xyzw, v0);
  p = pb_push4(p, 0xB00 /*NV097_SET_TRANSFORM_PROGRAM[0]*/, 0x00000000, 0x0020001B, 0x0836106C, 0x2070F800);

  //  MUL(oPos,xyz, R12, c[58]);
  //  RCC(R1,x, R12.w);
  p = pb_push4(p, 0xB10 /*NV097_SET_TRANSFORM_PROGRAM[4]*/, 0x00000000, 0x0647401B, 0xC4361BFF, 0x1078E800);

  // MAD(oPos,xyz, R12, R1.x, c[59]);
  p = pb_push4(p, 0xB20 /*NV097_SET_TRANSFORM_PROGRAM[8]*/, 0x00000000, 0x0087601B, 0xC400286C, 0x3070E801);

  // MOV(oT0,xy, v9);
  p = pb_push4(p, 0xB30 /* NV097_SET_TRANSFORM_PROGRAM[12] */, 0x00000000, 0x0020121B, 0x0836106C, 0x2070C849);

  p = pb_push1(p, 0x1E98 /*NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN*/, 0x0);

  p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0x0);
  p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 0x62);

  p = pb_push4f(p, 0xB80 /*NV097_SET_TRANSFORM_CONSTANT[0]*/, 1.0f, 0.0f, 0.0f, 0.0f);
  p = pb_push4f(p, 0xB90 /*NV097_SET_TRANSFORM_CONSTANT[4]*/, 0.0f, 1.0f, 0.0f, 0.0f);
  p = pb_push4f(p, 0xBA0 /*NV097_SET_TRANSFORM_CONSTANT[8]*/, 0.0f, 0.0f, 0.050505f, 0.242424f);
  p = pb_push4f(p, 0xBB0 /*NV097_SET_TRANSFORM_CONSTANT[12]*/, 0.0f, 0.0f, 0.0f, 1.0f);
  pb_end(p);

  host_.SetVertexBuffer(render_target_vertex_buffer_);
  bool swizzle = true;
  host_.SetSurfaceFormat(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, kTextureWidth, kTextureHeight, swizzle);
  if (!swizzle) {
    // Linear targets should be cleared to avoid uninitialized memory in regions not explicitly drawn to.
    host_.Clear(0xFF000000, 0, 0);
  }

  host_.DrawArrays();

  texture_stage.SetTextureDimensions(kTextureWidth, kTextureHeight);
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  host_.SetWindowClip(host_.GetFramebufferWidth() - 1, host_.GetFramebufferHeight() - 1);
  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));

  p = pb_begin();
  p = pb_push1(p, NV097_SET_SURFACE_PITCH,
               SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                   SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
  pb_end(p);

  host_.SetRawTexture(render_target_, kTextureWidth, kTextureHeight, 1, kTexturePitch, 4, false);

  host_.SetVertexBuffer(framebuffer_vertex_buffer_);
  host_.PrepareDraw(0xFE202020);
  host_.DrawArrays();

  pb_print("%s\n", kTestRenderTargetName);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, kTestRenderTargetName);
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

  const float height = floorf(static_cast<float>(host_.GetFramebufferHeight()) / 4.0f) * 2.0f;

  const float left = floorf((static_cast<float>(host_.GetFramebufferWidth()) - height) / 2.0f);
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

  std::string test_name = MakeGeometryTestName(bias);
  pb_print("%s\n", test_name.c_str());
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

std::string VertexShaderRoundingTests::MakeGeometryTestName(float bias) {
  char buf[32] = {0};
  snprintf(buf, 31, "%s_%f", kTestGeometryName, bias);
  return buf;
}
