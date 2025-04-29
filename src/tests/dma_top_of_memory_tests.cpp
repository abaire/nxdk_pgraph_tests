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

static constexpr char kTestName[] = "DMATopOfMemory";
static constexpr uint32_t kTextureSize = 128;

static constexpr uint32_t kCheckerSize = 8;
static constexpr uint32_t kCheckerboardB = 0xFF3333C0;

DMATopOfMemoryTests::DMATopOfMemoryTests(TestHost &host, std::string output_dir, const Config &config)
    : TestSuite(host, std::move(output_dir), "DMA top of memory", config) {
  tests_[kTestName] = [this]() { Test(); };
}

void DMATopOfMemoryTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PassthroughVertexShader>();
  host_.SetVertexShaderProgram(shader);

  auto channel = kNextContextChannel;
  pb_create_dma_ctx(channel++, DMA_CLASS_3D, 0, 0x7FFAFFF, &texture_top_of_memory_ctx_);
  pb_bind_channel(&texture_top_of_memory_ctx_);
}

void DMATopOfMemoryTests::Test() {
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
  Pushbuffer::Push(NV097_SET_CONTEXT_DMA_A, texture_top_of_memory_ctx_.ChannelID);
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

  pb_print("%s\n", kTestName);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kTestName);
}
