#include "texture_render_update_in_place_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "test_host.h"
#include "texture_format.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
const uint32_t kDefaultDMAChannelA = 3;

// From pbkit.c, DMA_COLOR is set to channel 9 by default
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
const uint32_t kDefaultDMAColorChannel = 9;

static const char *kTestName = "RenderToInputTexture";

TextureRenderUpdateInPlaceTests::TextureRenderUpdateInPlaceTests(TestHost &host, std::string output_dir,
                                                                 const Config &config)
    : TestSuite(host, std::move(output_dir), "Texture render update in place", config) {
  tests_[kTestName] = [this]() { Test(); };
}

void TextureRenderUpdateInPlaceTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  auto channel = kNextContextChannel;
  pb_create_dma_ctx(channel++, DMA_CLASS_3D, 0, MAXRAM, &texture_target_ctx_);
  pb_bind_channel(&texture_target_ctx_);

  const uint32_t texture_size = host_.GetMaxTextureWidth() * 4 * host_.GetMaxTextureHeight();
  render_target_ =
      (uint8_t *)MmAllocateContiguousMemoryEx(texture_size, 0, MAXRAM, 0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE);
  ASSERT(render_target_ && "Failed to allocate target surface.");

  host_.SetCombinerControl(1, true, true);

  host_.SetTextureFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8));
}

void TextureRenderUpdateInPlaceTests::Deinitialize() {
  TestSuite::Deinitialize();
  if (render_target_) {
    MmFreeContiguousMemory(render_target_);
  }

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_A, kDefaultDMAChannelA);
  Pushbuffer::End();
}

void TextureRenderUpdateInPlaceTests::CreateGeometry() {
  render_target_vertex_buffer_ = host_.AllocateVertexBuffer(6);

  render_target_vertex_buffer_->DefineBiTri(0, 0.0f, 0.0f, host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight(),
                                            0.1f);
  render_target_vertex_buffer_->Linearize(host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight());

  framebuffer_vertex_buffer_ = host_.AllocateVertexBuffer(6);
  framebuffer_vertex_buffer_->DefineBiTri(0, -1.75, 1.75, 1.75, -1.75, 0.1f);
  framebuffer_vertex_buffer_->Linearize(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF());
}

void TextureRenderUpdateInPlaceTests::Test() {
  const uint32_t kFramebufferPitch = host_.GetFramebufferWidth() * 4;

  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetTextureDimensions(host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight());
  const uint32_t kTexturePitch = host_.GetMaxTextureWidth() * 4;

  auto shader = std::make_shared<PrecalculatedVertexShader>();
  host_.SetVertexShaderProgram(shader);
  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);

  host_.PrepareDraw(0xFE202020);

  const auto render_target_address = reinterpret_cast<uint32_t>(render_target_) & 0x03FFFFFF;
  const auto normal_texture_address = reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF;

  host_.SetVertexBuffer(render_target_vertex_buffer_);
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetMaxTextureWidth(),
                                  host_.GetMaxTextureHeight(), true);

  // Set the render target as the color output and render a pure white rectangle.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kTexturePitch) |
                                                  SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, texture_target_ctx_.ChannelID);
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, render_target_address);
    // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
    Pushbuffer::Push(NV097_NO_OPERATION, 0);
    Pushbuffer::Push(NV097_WAIT_FOR_IDLE, 0);
    Pushbuffer::End();

    host_.SetWindowClip(host_.GetMaxTextureWidth(), host_.GetMaxTextureHeight());
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);
    host_.DrawArrays();
  }

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetupTextureStages();

  // Set the render target as the input texture and remove the blue channel.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_CONTEXT_DMA_A, texture_target_ctx_.ChannelID);
    Pushbuffer::Push(NV097_SET_TEXTURE_OFFSET, render_target_address);
    Pushbuffer::End();

    host_.SetCombinerFactorC0(0, 1.0f, 1.0f, 0.0f, 1.0f);
    host_.SetCombinerFactorC1(0, 0.0f, 0.0f, 0.0f, 1.0f);
    host_.SetInputColorCombiner(0, TestHost::ColorInput(TestHost::SRC_TEX0), TestHost::ColorInput(TestHost::SRC_C0));
    host_.SetOutputColorCombiner(0, TestHost::DST_R0);
    host_.SetInputAlphaCombiner(0, TestHost::AlphaInput(TestHost::SRC_TEX0), TestHost::AlphaInput(TestHost::SRC_C1));
    host_.SetOutputAlphaCombiner(0, TestHost::DST_R0);
    host_.SetFinalCombiner0Just(TestHost::SRC_R0);
    host_.SetFinalCombiner1Just(TestHost::SRC_R0, true);
    host_.DrawArrays();
  }

  // Switch the target texture to the normal texture memory, leaving the same texture offset (input) address. Then
  // modulate the texture further, zeroing the red channel and increasing the alpha.
  {
    Pushbuffer::Begin();
    Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, normal_texture_address);
    Pushbuffer::End();

    host_.SetCombinerFactorC0(0, 0.0f, 1.0f, 1.0f, 1.0f);
    host_.SetCombinerFactorC1(0, 0.0f, 0.0f, 0.0f, 1.0f);
    host_.DrawArrays();
  }

  // Set the input texture to the normal texture address and the output to the backbuffer.
  host_.SetVertexShaderProgram(nullptr);
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  host_.SetSurfaceFormatImmediate(TestHost::SCF_A8R8G8B8, TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                                  host_.GetFramebufferHeight(), false);

  host_.SetWindowClip(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_SURFACE_PITCH, SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, kFramebufferPitch) |
                                                SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, kFramebufferPitch));
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, 0);
  Pushbuffer::Push(NV097_SET_TEXTURE_OFFSET, normal_texture_address);
  Pushbuffer::End();

  host_.SetVertexBuffer(framebuffer_vertex_buffer_);
  host_.PrepareDraw(0xFE252525);
  host_.DrawArrays();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestName);
}
