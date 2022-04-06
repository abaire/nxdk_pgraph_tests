#include "texture_framebuffer_blit_tests.h"

#include <SDL_image.h>
#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "vertex_buffer.h"

// See pb_init in pbkit.c, where the channel contexts are set up.
// SUBCH_3 == GR_CLASS_9F, which contains the IMAGE_BLIT commands.
#define SUBCH_CLASS_9F SUBCH_3
// SUBCH_4 == GR_CLASS_62, which contains the NV10 2D surface commands.
#define SUBCH_CLASS_62 SUBCH_4

// Subchannel reserved for interaction with the class 19 channel.
static constexpr uint32_t SUBCH_CLASS_19 = kNextSubchannel;

// Subchannel reserved for interaction with the class 12 channel.
static constexpr uint32_t SUBCH_CLASS_12 = SUBCH_CLASS_19 + 1;

// Subchannel reserved for interaction with the class 72 channel.
static constexpr uint32_t SUBCH_CLASS_72 = SUBCH_CLASS_12 + 1;

static constexpr char kTestName[] = "BlitFBToTextureAndRender";

TextureFramebufferBlitTests::TextureFramebufferBlitTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture Framebuffer Blit") {
  tests_[kTestName] = [this]() { Test(); };
}

void TextureFramebufferBlitTests::Initialize() {
  TestSuite::Initialize();
  SetDefaultTextureFormat();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  CreateGeometry();

  // TODO: Provide a mechanism to find the next unused channel.
  auto channel = kNextContextChannel;

  const uint32_t texture_target_dma_channel = channel++;
  pb_create_dma_ctx(texture_target_dma_channel, DMA_CLASS_3D, 0, MAXRAM, &texture_target_ctx_);
  pb_bind_channel(&texture_target_ctx_);

  pb_create_gr_ctx(channel++, GR_CLASS_30, &null_ctx_);
  pb_bind_channel(&null_ctx_);

  pb_create_gr_ctx(channel++, GR_CLASS_19, &clip_rect_ctx_);
  pb_bind_channel(&clip_rect_ctx_);
  // TODO: Provide a mechanism to find the next unused subchannel.
  pb_bind_subchannel(SUBCH_CLASS_19, &clip_rect_ctx_);

  pb_create_gr_ctx(channel++, GR_CLASS_12, &beta_ctx_);
  pb_bind_channel(&beta_ctx_);
  pb_bind_subchannel(SUBCH_CLASS_12, &beta_ctx_);

  pb_create_gr_ctx(channel++, GR_CLASS_72, &beta4_ctx_);
  pb_bind_channel(&beta4_ctx_);
  pb_bind_subchannel(SUBCH_CLASS_72, &beta4_ctx_);
}

void TextureFramebufferBlitTests::ImageBlit(uint32_t operation, uint32_t beta, uint32_t source_channel,
                                            uint32_t destination_channel, uint32_t surface_format,
                                            uint32_t source_pitch, uint32_t destination_pitch, uint32_t source_offset,
                                            uint32_t source_x, uint32_t source_y, uint32_t destination_offset,
                                            uint32_t destination_x, uint32_t destination_y, uint32_t width,
                                            uint32_t height, uint32_t clip_x, uint32_t clip_y, uint32_t clip_width,
                                            uint32_t clip_height) const {
  auto p = pb_begin();
  p = pb_push1_to(SUBCH_CLASS_19, p, NV01_CONTEXT_CLIP_RECTANGLE_SET_POINT, clip_x | (clip_y << 16));
  p = pb_push1_to(SUBCH_CLASS_19, p, NV01_CONTEXT_CLIP_RECTANGLE_SET_SIZE, clip_width | (clip_height << 16));
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_CLIP_RECTANGLE, clip_rect_ctx_.ChannelID);

  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_OPERATION, operation);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY0, source_channel);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY1, destination_channel);

  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_FORMAT, surface_format);

  p = pb_push1_to(SUBCH_CLASS_62, p, NV062_SET_PITCH, source_pitch | (destination_pitch << 16));
  p = pb_push1_to(SUBCH_CLASS_62, p, NV062_SET_OFFSET_SOURCE, source_offset);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV062_SET_OFFSET_DESTIN, destination_offset);

  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_COLOR_KEY, null_ctx_.ChannelID);
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_PATTERN, null_ctx_.ChannelID);
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_ROP5, null_ctx_.ChannelID);

  if (operation != NV09F_SET_OPERATION_BLEND_AND) {
    p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SET_BETA, null_ctx_.ChannelID);
  } else {
    p = pb_push1_to(SUBCH_CLASS_12, p, NV012_SET_BETA, beta);
    p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SET_BETA, beta_ctx_.ChannelID);
  }

  if (operation != NV09F_SET_OPERATION_SRCCOPY_PREMULT && operation != NV09F_SET_OPERATION_BLEND_AND_PREMULT) {
    p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SET_BETA4, null_ctx_.ChannelID);
  } else {
    // beta is ARGB
    p = pb_push1_to(SUBCH_CLASS_72, p, NV072_SET_BETA, beta);
    p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SET_BETA4, beta4_ctx_.ChannelID);
  }

  p = pb_push1_to(SUBCH_CLASS_9F, p, NV09F_CONTROL_POINT_IN, source_x | (source_y << 16));
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV09F_CONTROL_POINT_OUT, destination_x | (destination_y << 16));
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV09F_SIZE, width | (height << 16));

  pb_end(p);
}

void TextureFramebufferBlitTests::CreateGeometry() {
  static constexpr float kLeft = -2.75f;
  static constexpr float kRight = 2.75f;
  static constexpr float kTop = 1.75f;
  static constexpr float kBottom = -1.75f;

  {
    constexpr int kNumTriangles = 4;
    texture_source_vertex_buffer_ = host_.AllocateVertexBuffer(kNumTriangles * 3);

    static constexpr float z = 1.0f;

    int index = 0;
    Color color_one, color_two, color_three;
    {
      color_one.SetGrey(0.25f);
      color_two.SetGrey(0.55f);
      color_three.SetGrey(0.75f);

      float one[] = {kLeft, kTop, z};
      float two[] = {0.0f, kTop, z};
      float three[] = {kLeft, 0.0f, z};
      texture_source_vertex_buffer_->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
    }

    {
      color_one.SetRGB(0.8f, 0.25f, 0.10f);
      color_two.SetRGB(0.1f, 0.85f, 0.10f);
      color_three.SetRGB(0.1f, 0.25f, 0.90f);

      float one[] = {kRight, kTop, z};
      float two[] = {kRight, 0.0f, z};
      float three[] = {0.0f, 0.0f, z};
      texture_source_vertex_buffer_->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
    }

    {
      color_one.SetRGB(0.8f, 0.25f, 0.90f);
      color_two.SetRGB(0.1f, 0.85f, 0.90f);
      color_three.SetRGB(0.85f, 0.95f, 0.10f);

      float one[] = {kRight, kBottom, z};
      float two[] = {0.0f, kBottom, z};
      float three[] = {kRight, 0.0f, z};
      texture_source_vertex_buffer_->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
    }

    {
      color_one.SetRGB(0.2f, 0.4f, 0.8f);
      color_two.SetRGB(0.3f, 0.5f, 0.9f);
      color_three.SetRGB(0.4f, 0.3f, 0.7f);

      float one[] = {kLeft, kBottom, z};
      float two[] = {0.0f, 0.0f, z};
      float three[] = {0.0f, kBottom, z};
      texture_source_vertex_buffer_->DefineTriangle(index++, one, two, three, color_one, color_two, color_three);
    }
  }

  target_vertex_buffer_ = host_.AllocateVertexBuffer(6);
  target_vertex_buffer_->DefineBiTri(0, kLeft, kTop, kRight, kBottom, 0.1);
  target_vertex_buffer_->Linearize(static_cast<float>(host_.GetFramebufferWidth()),
                                   static_cast<float>(host_.GetFramebufferHeight()));
}

void TextureFramebufferBlitTests::Test() {
  host_.PrepareDraw(0xF0441111);

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  pb_end(p);

  // Render test content to the framebuffer, then blit it into texture memory.
  {
    host_.SetTextureStageEnabled(0, false);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetupTextureStages();

    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

    host_.SetVertexBuffer(texture_source_vertex_buffer_);
    host_.DrawArrays(TestHost::POSITION | TestHost::DIFFUSE);

    uint32_t clip_x = 0;
    uint32_t clip_y = 0;
    uint32_t clip_w = host_.GetFramebufferWidth();
    uint32_t clip_h = host_.GetFramebufferHeight();
    uint32_t fb_pitch = clip_w * 4;

    ImageBlit(NV09F_SET_OPERATION_SRCCOPY, 0, DMA_CHANNEL_3D_3, texture_target_ctx_.ChannelID,
              NV04_SURFACE_2D_FORMAT_A8R8G8B8, fb_pitch, fb_pitch, reinterpret_cast<uint32_t>(pb_back_buffer()), 0, 0,
              reinterpret_cast<uint32_t>(host_.GetTextureMemory()), 0, 0, clip_w, clip_h, clip_x, clip_y, clip_w,
              clip_h);
  }

  host_.Clear(0xF0111111);

  // Render a quad with the generated texture.
  auto& stage = host_.GetTextureStage(0);
  stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
  stage.SetBorderColor(0xFF7F007F);
  stage.SetImageDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());

  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetupTextureStages();

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  host_.SetVertexBuffer(target_vertex_buffer_);
  host_.DrawArrays();

  host_.FinishDraw(allow_saving_, output_dir_, kTestName);
}
