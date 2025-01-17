#include "texture_framebuffer_blit_tests.h"

#include <SDL_image.h>
#include <pbkit/pbkit.h>

#include "debug_output.h"
#include "nxdk_ext.h"
#include "pbkit_ext.h"
#include "test_host.h"
#include "vertex_buffer.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// See pb_init in pbkit.c, where the channel contexts are set up.
// SUBCH_3 == GR_CLASS_9F, which contains the IMAGE_BLIT commands.
#define SUBCH_CLASS_9F SUBCH_3
// SUBCH_4 == GR_CLASS_62, which contains the NV10 2D surface commands.
#define SUBCH_CLASS_62 SUBCH_4

// From pbkit.c, DMA_COLOR is set to channel 9 by default
// NV097_SET_CONTEXT_DMA_COLOR == NV20_TCL_PRIMITIVE_3D_SET_OBJECT3
const uint32_t kDefaultDMAColorChannel = 9;

// NV097_SET_CONTEXT_DMA_ZETA is set to channel 10 by default.
const uint32_t kDefaultDMAZetaChannel = 10;

// From pbkit.c, DMA_A is set to channel 3 by default
// NV097_SET_CONTEXT_DMA_A == NV20_TCL_PRIMITIVE_3D_SET_OBJECT1
const uint32_t kDefaultDMAChannelA = 3;

// Subchannel reserved for interaction with the class 19 channel.
static constexpr uint32_t SUBCH_CLASS_19 = kNextSubchannel;

// Subchannel reserved for interaction with the class 12 channel.
static constexpr uint32_t SUBCH_CLASS_12 = SUBCH_CLASS_19 + 1;

// Subchannel reserved for interaction with the class 72 channel.
static constexpr uint32_t SUBCH_CLASS_72 = SUBCH_CLASS_12 + 1;

static constexpr char kTextureTarget[] = "FBToTexture";
static constexpr char kZetaTarget[] = "FBToZetaAsTex";
static constexpr char kRenderTextureTarget[] = "FBToOldRenderTarget";

TextureFramebufferBlitTests::TextureFramebufferBlitTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Texture Framebuffer Blit", config) {
  tests_[kTextureTarget] = [this]() {
    auto offset = reinterpret_cast<uint32_t>(host_.GetTextureMemory());
    Test(offset, kTextureTarget);
  };
  tests_[kZetaTarget] = [this]() {
    auto offset = reinterpret_cast<uint32_t>(pb_depth_stencil_buffer());
    Test(offset, kZetaTarget);
  };
  tests_[kRenderTextureTarget] = [this]() { TestRenderTarget(kRenderTextureTarget); };
}

void TextureFramebufferBlitTests::Initialize() {
  TestSuite::Initialize();
  SetDefaultTextureFormat();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  CreateGeometry();

  // TODO: Provide a mechanism to find the next unused channel.
  auto channel = kNextContextChannel;

  pb_create_dma_ctx(channel++, DMA_CLASS_3D, 0, MAXRAM, &texture_target_ctx_);
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

  host_.SetSurfaceFormat(host_.GetColorBufferFormat(), TestHost::SZF_Z24S8, host_.GetFramebufferWidth(),
                         host_.GetFramebufferHeight());

  // The texture DMA channel is overridden to use a context that can address arbitrary RAM.
  // (pbkit sets up a channel that can only address a subset of video RAM)
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_A, texture_target_ctx_.ChannelID);
  pb_end(p);
}

void TextureFramebufferBlitTests::Deinitialize() {
  TestSuite::Deinitialize();
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAZetaChannel);
  p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
  p = pb_push1(p, NV097_SET_SURFACE_ZETA_OFFSET, 0);
  // Note: Leaving arbitrary offsets for these values will lead to a color buffer limit error in the two_d_line_tests.
  p = pb_push1_to(SUBCH_CLASS_62, p, NV062_SET_OFFSET_SOURCE, 0);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV062_SET_OFFSET_DESTIN, 0);
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_A, kDefaultDMAChannelA);
  p = pb_push1(p, NV097_NO_OPERATION, 0);
  p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
  p = pb_push1(p, NV097_NO_OPERATION, 0);
  p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
  pb_end(p);
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

void TextureFramebufferBlitTests::TestRenderTarget(const char* test_name) {
  const auto pitch = host_.GetFramebufferWidth() * 4;
  const uint32_t texture_size = pitch * host_.GetFramebufferHeight();
  auto target =
      (uint8_t*)MmAllocateContiguousMemoryEx(texture_size, 0, MAXRAM, 0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE);
  ASSERT(target && "Failed to allocate target surface.");

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SURFACE_PITCH,
               SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, pitch) | SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, pitch));
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, texture_target_ctx_.ChannelID);
  p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, reinterpret_cast<uint32_t>(target) & 0x03FFFFFF);
  // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
  p = pb_push1(p, NV097_NO_OPERATION, 0);
  p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
  pb_end(p);

  // Render something into the texture. The expectation is that this will not actually be displayed since the test will
  // completely blit over it.
  host_.PrepareDraw(0xF0AA00AA);

  p = pb_begin();
  p = pb_push1(p, NV097_SET_SURFACE_PITCH,
               SET_MASK(NV097_SET_SURFACE_PITCH_COLOR, pitch) | SET_MASK(NV097_SET_SURFACE_PITCH_ZETA, pitch));
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, kDefaultDMAColorChannel);
  p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
  // TODO: Investigate if this is actually necessary. Morrowind does this after changing offsets.
  p = pb_push1(p, NV097_NO_OPERATION, 0);
  p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
  pb_end(p);

  Test(reinterpret_cast<uint32_t>(target), test_name);

  MmFreeContiguousMemory(target);
}

void TextureFramebufferBlitTests::Test(uint32_t texture_destination, const char* test_name) {
  host_.PrepareDraw(0xF0441111);

  texture_destination &= 0x03FFFFFF;

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_STENCIL_TEST_ENABLE, false);
  p = pb_push1(p, NV097_SET_DEPTH_MASK, true);

  // Trigger https://github.com/mborgerson/xemu/issues/788 by forcing xemu to consider the zeta buffer as dirty.
  p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, kDefaultDMAZetaChannel);
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
              NV04_SURFACE_2D_FORMAT_A8R8G8B8, fb_pitch, fb_pitch,
              reinterpret_cast<uint32_t>(pb_back_buffer()) & 0x03FFFFFF, 0, 0, texture_destination, 0, 0, clip_w,
              clip_h, clip_x, clip_y, clip_w, clip_h);
  }

  // Render a quad with the generated texture.
  auto& stage = host_.GetTextureStage(0);
  stage.SetFormat(GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8));
  stage.SetTextureDimensions(1, 1, 1);
  stage.SetBorderColor(0xFF7F007F);
  stage.SetImageDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);
  host_.SetupTextureStages();

  // The texture stage utilities always operate off of the texture memory managed by the test_host. Because this test
  // needs to reference arbitrary texture destinations, the offset must be applied manually after the rest of the params
  // are committed.
  p = pb_begin();
  p = pb_push1(p, NV097_SET_TEXTURE_OFFSET, texture_destination);
  pb_end(p);

  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_TEX0, true);

  host_.SetVertexBuffer(target_vertex_buffer_);
  host_.DrawArrays();

  pb_print("%s\n", test_name);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}
