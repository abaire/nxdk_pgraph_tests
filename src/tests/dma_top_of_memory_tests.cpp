#include "dma_top_of_memory_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/passthrough_vertex_shader.h"
#include "test_host.h"

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
static constexpr uint32_t kDefaultDMAChannelA = 3;
// From pbkit.c, DMA_COLOR is set to channel 9 by default.
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
static constexpr uint32_t kDefaultDMAColorChannel = 9;

static constexpr char kTextureBeyondTopOfMemoryTestName[] = "TexBeyondTopOfMemory";
static constexpr char kClearSurfaceBeyondTopOfMemoryTestName[] = "ClearBeyondTopOfMemory";

DMATopOfMemoryTests::DMATopOfMemoryTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "DMA top of memory", config) {
  tests_[kTextureBeyondTopOfMemoryTestName] = [this]() { TestReadTextureBeyondTopOfMemory(); };
  tests_[kClearSurfaceBeyondTopOfMemoryTestName] = [this]() { TestClearSurfaceBeyondTopOfMemory(); };
}

void DMATopOfMemoryTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  auto channel = kNextContextChannel;
  pb_create_dma_ctx(channel++, DMA_CLASS_3D, 0, 0x7FFAFFF, &all_memory_ctx_);
  pb_bind_channel(&all_memory_ctx_);
}

void DMATopOfMemoryTests::TestReadTextureBeyondTopOfMemory() {
  host_.PrepareDraw(0xFF050505);
  const auto width = host_.GetFramebufferWidthF();
  const auto height = host_.GetFramebufferHeightF();

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  auto &texture_stage = host_.GetTextureStage(0);
  texture_stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
  texture_stage.SetImageDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetupTextureStages();

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_A, all_memory_ctx_.ChannelID);
  Pushbuffer::Push(NV097_SET_TEXTURE_OFFSET, 0x3F5E000);
  Pushbuffer::End();

  host_.Begin(TestHost::PRIMITIVE_QUADS);
  host_.SetTexCoord0(0.0f, 0.0f);
  host_.SetVertex(0.f, 0.f, 0.1f, 1.0f);

  host_.SetTexCoord0(width, 0.0f);
  host_.SetVertex(width, 0.f, 0.1f, 1.0f);

  host_.SetTexCoord0(width, height);
  host_.SetVertex(width, height, 0.1f, 1.0f);

  host_.SetTexCoord0(0.0f, height);
  host_.SetVertex(0.f, height, 0.1f, 1.0f);
  host_.End();

  const auto normal_texture_address = reinterpret_cast<uint32_t>(host_.GetTextureMemory()) & 0x03FFFFFF;
  Pushbuffer::Begin();
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_A, kDefaultDMAChannelA);
  Pushbuffer::Push(NV097_SET_TEXTURE_OFFSET, normal_texture_address);
  Pushbuffer::End();

  host_.SetTextureStageEnabled(0, false);
  host_.SetShaderStageProgram(TestHost::STAGE_NONE);
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  pb_print("%s\n", kTextureBeyondTopOfMemoryTestName);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTextureBeyondTopOfMemoryTestName);
}

void DMATopOfMemoryTests::TestClearSurfaceBeyondTopOfMemory() {
  host_.PrepareDraw(0xFF050505);
  const auto width = host_.GetFramebufferWidth();
  const auto height = host_.GetFramebufferHeight();

  // Force the backbuffer to pure white.
  auto *back_buffer = reinterpret_cast<uint8_t *>(pb_back_buffer());
  memset(back_buffer, 0xFF, width * height * 4);

  auto swap_register = [](uint32_t register_offset, uint32_t new_val) {
    auto reg = reinterpret_cast<uint32_t *>(PGRAPH_REGISTER_BASE + register_offset);
    auto ret = *reg;
    *reg = new_val;
    return ret;
  };

  // Value observed from ESPN NFL Football. In spongebob bit 11 (0x800) seems to control the interrupt due to a limit
  // error.
  uint32_t old_0x880 = swap_register(0x880, 0x28C3FF);

  Pushbuffer::Begin();
  // Set the DMA context such that it matches the observed value of ESPN NFL Football.
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, all_memory_ctx_.ChannelID);

  // Set the color offset to the RAM address of the backbuffer (high bit will be set, which is outside the allowed
  // range).
  auto back_buffer_addr = reinterpret_cast<uint32_t>(back_buffer);
  Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, back_buffer_addr);

  Pushbuffer::Push(NV097_SET_CLEAR_RECT_HORIZONTAL, 639 << 16);
  Pushbuffer::Push(NV097_SET_CLEAR_RECT_VERTICAL, 479 << 16);
  Pushbuffer::Push(NV097_SET_ZSTENCIL_CLEAR_VALUE, 0xFFFFFF00);
  Pushbuffer::Push(NV097_SET_COLOR_CLEAR_VALUE, 0xFFFF00FF);
  Pushbuffer::Push(NV097_CLEAR_SURFACE, 0xF3);

  Pushbuffer::Push(NV097_SET_SURFACE_COLOR_OFFSET, VRAM_ADDR(back_buffer_addr));
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  Pushbuffer::End();

  pb_print("%s\n", kClearSurfaceBeyondTopOfMemoryTestName);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kClearSurfaceBeyondTopOfMemoryTestName);

  // NOTE: 880 cannot be reset until after the clear command is fully processed.
  swap_register(0x880, old_0x880);
}
