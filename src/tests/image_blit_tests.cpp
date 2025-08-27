#include "image_blit_tests.h"

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

static constexpr char kDirtyOverlappedDestSurfaceTest[] = "DirtyOverlappedDestSurf";
static constexpr char kBlitRenderBlitTest[] = "BlitRenderBlit";

#define SOURCE_X 8
#define SOURCE_Y 8
#define SOURCE_WIDTH 128
#define SOURCE_HEIGHT 128
#define DESTINATION_X 256
#define DESTINATION_Y 176

static std::string OperationName(uint32_t operation);
static std::string ColorFormatName(uint32_t format);

static constexpr ImageBlitTests::BlitTest kTests[] = {
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x80000000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x8FFFFFFF},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x007FFFFF},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00800000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x03300000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00D00000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x44400000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x444fffff},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x66800000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x7F800000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x7FFFFFFF},

    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8, 0x007FFFFF},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8, 0x00800000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8, 0x00D00000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8, 0x03300000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8, 0x44400000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8, 0x444fffff},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8, 0x66800000},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8, 0x7F800000},

    //    {NV09F_SET_OPERATION_BLEND_AND_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00000000},
    //    {NV09F_SET_OPERATION_BLEND_AND_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0xFF000000},
    //    {NV09F_SET_OPERATION_BLEND_AND_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00FF0000},
    //    {NV09F_SET_OPERATION_BLEND_AND_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x0000FF00},
    //    {NV09F_SET_OPERATION_BLEND_AND_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x000000FF},
    //    {NV09F_SET_OPERATION_BLEND_AND_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x33000000},
    //    {NV09F_SET_OPERATION_BLEND_AND_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00330000},
    //    {NV09F_SET_OPERATION_BLEND_AND_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00003300},
    //    {NV09F_SET_OPERATION_BLEND_AND_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00000033},
    //    {NV09F_SET_OPERATION_BLEND_AND_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00FFFFFF},
    //    {NV09F_SET_OPERATION_BLEND_AND_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0xFFFFFFFF},

    {NV09F_SET_OPERATION_SRCCOPY, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0},
    {NV09F_SET_OPERATION_SRCCOPY, NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8, 0},  // Zero alpha.
    {NV09F_SET_OPERATION_SRCCOPY, NV04_SURFACE_2D_FORMAT_A8R8G8B8, 0},

    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00000000},
    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0xFF000000},
    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00FF0000},
    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x0000FF00},
    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x000000FF},
    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x33000000},
    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00330000},
    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00003300},
    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00000033},
    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0x00FFFFFF},
    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0xFFFFFFFF},
    //    {NV09F_SET_OPERATION_SRCCOPY_PREMULT, NV04_SURFACE_2D_FORMAT_A8R8G8B8, 0x00000033},
};

struct ClipRegion {
  uint32_t x;
  uint32_t y;
  uint32_t width;
  uint32_t height;
};

static constexpr ClipRegion kClipRegionTests[] = {
    {0, 0, 640, 480}, {320, 240, 64, 40}, {320, 240, 640, 480}, {300, 200, 16, 24},
    {320, 240, 0, 0}, {320, 240, 1, 1},   {320, 240, 0, 10},
};

static std::string MakeTestName(const ImageBlitTests::BlitTest& test) {
  char buf[256] = {0};
  snprintf(buf, 255, "ImgBlt_%s_%s_B%08X", OperationName(test.blit_operation).c_str(),
           ColorFormatName(test.buffer_color_format).c_str(), test.beta);
  return buf;
}

static std::string MakeTestName(uint32_t clip_x, uint32_t clip_y, uint32_t clip_w, uint32_t clip_h) {
  char buf[256] = {0};
  snprintf(buf, 255, "ImgBlt_Clip_%d_%d_%d_%d", clip_x, clip_y, clip_w, clip_h);
  return buf;
}

/**
 * Initializes the test suite and creates test cases.
 *
 * @tc DirtyOverlappedDestSurf
 *   Performs a 3d render into the backbuffer, then a 2D srccopy that completely overwrites it. Finally performs another
 *   3d render, demonstrating incorrect behavior in various versions of xemu.
 *
 * @tc BlitRenderBlit
 *   Blits an image into a render surface, then performs a 3d render on top of the surface, then another blit on top of
 *   the composited two. Finally does a srccopy to blit the finished composition to the framebuffer. Demonstrates
 *   apparent incorrect ordering in various versions of xemu.
 *
 * @tc ImgBlt_Clip_0_0_640_480
 *   Demonstrates expected behavior of the contextual clip rectangle (class 0x19) during image blits.
 *
 * @tc ImgBlt_BLENDAND_XRGB_B00000000
 *   Demonstrates expected behavior of the contextual beta (class 0x12) during image blits.
 */
ImageBlitTests::ImageBlitTests(TestHost& host, std::string output_dir, const Config& config)
    : TestSuite(host, std::move(output_dir), "Image blit", config) {
  for (auto& test : kTests) {
    std::string name = MakeTestName(test);
    tests_[name] = [this, test]() { Test(test); };
  }

  {
    ImageBlitTests::BlitTest blit_setup{NV09F_SET_OPERATION_SRCCOPY, NV04_SURFACE_2D_FORMAT_A8R8G8B8, 0};
    for (auto& test : kClipRegionTests) {
      tests_[MakeTestName(test.x, test.y, test.width, test.height)] = [this, blit_setup, &test] {
        TestWithClipRectangle(blit_setup, test.x, test.y, test.width, test.height);
      };
    }
  }

  tests_[kDirtyOverlappedDestSurfaceTest] = [this]() { TestDirtyOverlappedDestinationSurface(); };
  tests_[kBlitRenderBlitTest] = [this]() { TestBlitRenderBlit(); };
}

void ImageBlitTests::Initialize() {
  TestSuite::Initialize();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();
  SetDefaultTextureFormat();

  SDL_Surface* temp = IMG_Load("D:\\image_blit\\TestImage.png");
  ASSERT(temp);
  SDL_Surface* test_image = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_BGRA32, 0);
  SDL_FreeSurface(temp);

  image_pitch_ = test_image->pitch;
  image_height_ = test_image->h;
  uint32_t image_bytes = image_pitch_ * image_height_;

  source_image_ = static_cast<uint8_t*>(
      MmAllocateContiguousMemoryEx(image_bytes, 0, MAXRAM, 0x1000, PAGE_WRITECOMBINE | PAGE_READWRITE));
  memcpy(source_image_, test_image->pixels, image_bytes);
  SDL_FreeSurface(test_image);

  // TODO: Provide a mechanism to find the next unused channel.
  auto channel = kNextContextChannel;

  pb_create_dma_ctx(channel++, DMA_CLASS_3D, 0, MAXRAM, &image_src_dma_ctx_);
  pb_bind_channel(&image_src_dma_ctx_);

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

  pb_create_dma_ctx(channel++, DMA_CLASS_3D, 0, MAXRAM, &render_target_dma_ctx_);
  pb_bind_channel(&render_target_dma_ctx_);
}

void ImageBlitTests::Deinitialize() {
  if (source_image_) {
    MmFreeContiguousMemory(source_image_);
    source_image_ = nullptr;
  }
  TestSuite::Deinitialize();
}

void ImageBlitTests::ImageBlit(uint32_t operation, uint32_t beta, uint32_t source_channel, uint32_t destination_channel,
                               uint32_t surface_format, uint32_t source_pitch, uint32_t destination_pitch,
                               uint32_t source_offset, uint32_t source_x, uint32_t source_y,
                               uint32_t destination_offset, uint32_t destination_x, uint32_t destination_y,
                               uint32_t width, uint32_t height, uint32_t clip_x, uint32_t clip_y, uint32_t clip_width,
                               uint32_t clip_height) const {
  PrintMsg("ImageBlit: %d beta: 0x%08X src: %d dest: %d\n", operation, beta, source_channel, destination_channel);
  Pushbuffer::Begin();
  Pushbuffer::PushTo(SUBCH_CLASS_19, NV01_CONTEXT_CLIP_RECTANGLE_SET_POINT, clip_x | (clip_y << 16));
  Pushbuffer::PushTo(SUBCH_CLASS_19, NV01_CONTEXT_CLIP_RECTANGLE_SET_SIZE, clip_width | (clip_height << 16));
  Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_CLIP_RECTANGLE, clip_rect_ctx_.ChannelID);

  Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_OPERATION, operation);
  Pushbuffer::PushTo(SUBCH_CLASS_62, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY0, source_channel);
  Pushbuffer::PushTo(SUBCH_CLASS_62, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY1, destination_channel);

  Pushbuffer::PushTo(SUBCH_CLASS_62, NV10_CONTEXT_SURFACES_2D_FORMAT, surface_format);

  Pushbuffer::PushTo(SUBCH_CLASS_62, NV10_CONTEXT_SURFACES_2D_PITCH, source_pitch | (destination_pitch << 16));
  Pushbuffer::PushTo(SUBCH_CLASS_62, NV10_CONTEXT_SURFACES_2D_OFFSET_SRC, source_offset);
  Pushbuffer::PushTo(SUBCH_CLASS_62, NV10_CONTEXT_SURFACES_2D_OFFSET_DST, destination_offset);

  Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_COLOR_KEY, null_ctx_.ChannelID);
  Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_PATTERN, null_ctx_.ChannelID);
  Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_ROP5, null_ctx_.ChannelID);

  if (operation != NV09F_SET_OPERATION_BLEND_AND) {
    Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_SET_BETA, null_ctx_.ChannelID);
  } else {
    Pushbuffer::PushTo(SUBCH_CLASS_12, NV012_SET_BETA, beta);
    Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_SET_BETA, beta_ctx_.ChannelID);
  }

  if (operation != NV09F_SET_OPERATION_SRCCOPY_PREMULT && operation != NV09F_SET_OPERATION_BLEND_AND_PREMULT) {
    Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_SET_BETA4, null_ctx_.ChannelID);
  } else {
    // beta is ARGB
    Pushbuffer::PushTo(SUBCH_CLASS_72, NV072_SET_BETA, beta);
    Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_SET_BETA4, beta4_ctx_.ChannelID);
  }

  Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_POINT_IN, source_x | (source_y << 16));
  Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_POINT_OUT, destination_x | (destination_y << 16));
  Pushbuffer::PushTo(SUBCH_CLASS_9F, NV_IMAGE_BLIT_SIZE, width | (height << 16));

  Pushbuffer::End();
}

void ImageBlitTests::Test(const BlitTest& test) {
  host_.PrepareDraw(0xF0440011);

  uint32_t image_bytes = image_pitch_ * image_height_;
  pb_set_dma_address(&image_src_dma_ctx_, source_image_, image_bytes - 1);

  uint32_t clip_x = 0;
  uint32_t clip_y = 0;
  uint32_t clip_w = host_.GetFramebufferWidth();
  uint32_t clip_h = host_.GetFramebufferHeight();

  ImageBlit(test.blit_operation, test.beta, image_src_dma_ctx_.ChannelID,
            DMA_CHANNEL_BITBLT_IMAGES,  // DMA channel 11 - 0x1117
            test.buffer_color_format, image_pitch_, 4 * host_.GetFramebufferWidth(), 0, SOURCE_X, SOURCE_Y, 0,
            DESTINATION_X, DESTINATION_Y, SOURCE_WIDTH, SOURCE_HEIGHT, clip_x, clip_y, clip_w, clip_h);

  std::string op_name = OperationName(test.blit_operation);
  pb_print("Op: %s\n", op_name.c_str());
  std::string color_format_name = ColorFormatName(test.buffer_color_format);
  pb_print("BufFmt: %s\n", color_format_name.c_str());
  if (test.blit_operation != NV09F_SET_OPERATION_SRCCOPY) {
    pb_print("Beta: %08X\n", test.beta);
  }
  pb_draw_text_screen();

  std::string name = MakeTestName(test);
  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void ImageBlitTests::TestWithClipRectangle(const ImageBlitTests::BlitTest& test, uint32_t clip_x, uint32_t clip_y,
                                           uint32_t clip_w, uint32_t clip_h) {
  host_.PrepareDraw(0xF0440011);

  uint32_t image_bytes = image_pitch_ * image_height_;
  pb_set_dma_address(&image_src_dma_ctx_, source_image_, image_bytes - 1);

  ImageBlit(test.blit_operation, test.beta, image_src_dma_ctx_.ChannelID,
            DMA_CHANNEL_BITBLT_IMAGES,  // DMA channel 11 - 0x1117
            test.buffer_color_format, image_pitch_, 4 * host_.GetFramebufferWidth(), 0, SOURCE_X, SOURCE_Y, 0,
            DESTINATION_X, DESTINATION_Y, SOURCE_WIDTH, SOURCE_HEIGHT, clip_x, clip_y, clip_w, clip_h);

  std::string name = MakeTestName(clip_x, clip_y, clip_w, clip_h);
  pb_print("%s\n", name.c_str());
  std::string color_format_name = ColorFormatName(test.buffer_color_format);
  pb_print("BufFmt: %s\n", color_format_name.c_str());
  if (test.blit_operation != NV09F_SET_OPERATION_SRCCOPY) {
    pb_print("Beta: %08X\n", test.beta);
  }
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, name);
}

void ImageBlitTests::TestDirtyOverlappedDestinationSurface() {
  host_.PrepareDraw(0xFF111111);

  uint32_t image_bytes = image_pitch_ * image_height_;
  pb_set_dma_address(&image_src_dma_ctx_, source_image_, image_bytes - 1);

  // Do a 3D render to a surface render target within the destination blit area.
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  auto subtexture = reinterpret_cast<uint8_t*>(pb_back_buffer()) + 64 + 64 * pb_back_buffer_pitch();
  host_.RenderToSurfaceStart(subtexture, TestHost::SCF_A8R8G8B8, host_.GetFramebufferWidth(),
                             host_.GetFramebufferHeight());
  host_.Begin(TestHost::PRIMITIVE_QUADS);

  host_.SetDiffuse(1.f, 0.f, 0.f);
  host_.SetScreenVertex(0.f, 0.f, 0.f);

  host_.SetDiffuse(0.f, 1.f, 0.f);
  host_.SetScreenVertex(128.f, 0.f, 0.f);

  host_.SetDiffuse(0.f, 0.f, 1.f);
  host_.SetScreenVertex(128.f, 128.f, 0.f);

  host_.SetDiffuse(0.75, 0.65f, 0.55f);
  host_.SetScreenVertex(0.f, 128.f, 0.f);

  host_.End();
  host_.RenderToSurfaceEnd();

  // 2) Do a 2D image srccopy that completely overwrites the destination area.
  ImageBlit(NV09F_SET_OPERATION_SRCCOPY, 0, image_src_dma_ctx_.ChannelID,
            DMA_CHANNEL_BITBLT_IMAGES,  // DMA channel 11 - 0x1117
            NV04_SURFACE_2D_FORMAT_A8R8G8B8, image_pitch_, 4 * host_.GetFramebufferWidth(),
            0,    // source_offset
            0,    // source_x
            0,    // source_y
            0,    // destination_offset
            0,    // destination_x
            0,    // destination_y
            256,  // width
            256,  // height
            0,    // clip_x
            0,    // clip_y
            256,  // clip_width
            256   // clip_height
  );

  // 3) Draw an unrelated quad to force the 3D hardware to wait on the 2D blit completion.
  host_.Begin(TestHost::PRIMITIVE_QUADS);

  host_.SetDiffuse(1.f, 0.f, 0.f);
  host_.SetScreenVertex(320.f, 240.f, 0.f);

  host_.SetDiffuse(0.f, 1.f, 0.f);
  host_.SetScreenVertex(324.f, 240.f, 0.f);

  host_.SetDiffuse(0.f, 0.f, 1.f);
  host_.SetScreenVertex(324.f, 244.f, 0.f);

  host_.SetDiffuse(0.75, 0.65f, 0.55f);
  host_.SetScreenVertex(320.f, 244.f, 0.f);

  host_.End();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kDirtyOverlappedDestSurfaceTest);
}

void ImageBlitTests::TestBlitRenderBlit() {
  host_.PrepareDraw(0xFF112233);

  uint32_t image_bytes = image_pitch_ * image_height_;
  pb_set_dma_address(&image_src_dma_ctx_, source_image_, image_bytes - 1);

  // Allocate a render target in texture memory.
  uint32_t rt_width = 256;
  uint32_t rt_height = 256;
  uint32_t rt_pitch = rt_width * 4;
  uint32_t rt_size = rt_pitch * rt_height;
  auto render_target = host_.GetTextureMemoryForStage(0);
  memset(render_target, 0, rt_size);
  pb_set_dma_address(&render_target_dma_ctx_, render_target, rt_size - 1);

  // 1) Blit an image to the render target.
  ImageBlit(NV09F_SET_OPERATION_BLEND_AND, 0x444fffff, image_src_dma_ctx_.ChannelID, render_target_dma_ctx_.ChannelID,
            NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, image_pitch_, rt_pitch,
            0,         // source_offset
            0,         // source_x
            0,         // source_y
            0,         // destination_offset
            0,         // destination_x
            0,         // destination_y
            128,       // width
            128,       // height
            0,         // clip_x
            0,         // clip_y
            rt_width,  // clip_width
            rt_height  // clip_height
  );

  // 2) Render a quad to a region of the render target.
  host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
  host_.SetFinalCombiner1Just(TestHost::SRC_ZERO, true, true);

  host_.RenderToSurfaceStart(render_target, TestHost::SCF_A8R8G8B8, rt_width, rt_height, false, 64, 64, 128, 128);
  host_.Begin(TestHost::PRIMITIVE_QUADS);

  host_.SetDiffuse(1.f, 0.f, 0.f);
  host_.SetScreenVertex(64.f, 64.f, 0.f);

  host_.SetDiffuse(0.f, 1.f, 0.f);
  host_.SetScreenVertex(192.f, 64.f, 0.f);

  host_.SetDiffuse(0.f, 0.f, 1.f);
  host_.SetScreenVertex(192.f, 192.f, 0.f);

  host_.SetDiffuse(0.75, 0.65f, 0.55f);
  host_.SetScreenVertex(64.f, 192.f, 0.f);

  host_.End();
  host_.RenderToSurfaceEnd();

  // 3) Blit again to the render target.
  ImageBlit(NV09F_SET_OPERATION_BLEND_AND, 0x66800000, image_src_dma_ctx_.ChannelID, render_target_dma_ctx_.ChannelID,
            NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, image_pitch_, rt_pitch,
            0,         // source_offset
            0,         // source_x
            0,         // source_y
            0,         // destination_offset
            128,       // destination_x
            128,       // destination_y
            128,       // width
            128,       // height
            0,         // clip_x
            0,         // clip_y
            rt_width,  // clip_width
            rt_height  // clip_height
  );

  // 4) Blit the render target to the screen to show the result.
  static constexpr auto y_offset = 72;
  ImageBlit(NV09F_SET_OPERATION_SRCCOPY, 0, render_target_dma_ctx_.ChannelID,
            DMA_CHANNEL_BITBLT_IMAGES,  // DMA channel 11 - 0x1117
            NV04_SURFACE_2D_FORMAT_A8R8G8B8, rt_pitch, 4 * host_.GetFramebufferWidth(),
            0,                                                                     // source_offset
            0,                                                                     // source_x
            0,                                                                     // source_y
            0,                                                                     // destination_offset
            (host_.GetFramebufferWidth() - rt_width) / 2,                          // destination_x
            y_offset + (host_.GetFramebufferHeight() - y_offset - rt_height) / 2,  // destination_y
            rt_width,                                                              // width
            rt_height,                                                             // height
            0,                                                                     // clip_x
            0,                                                                     // clip_y
            host_.GetFramebufferWidth(),                                           // clip_width
            host_.GetFramebufferHeight()                                           // clip_height
  );

  pb_print("%s\n", kBlitRenderBlitTest);
  pb_print("  Test image is rendered with low alpha to UL\n");
  pb_print("  Quad is rendered to LL\n");
  pb_print("  Test image is rendered with high alpha to LR\n");
  pb_print("  Surface is blit via SRCCOPY to framebuffer\n");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, suite_name_, kBlitRenderBlitTest);
}

static std::string OperationName(uint32_t operation) {
  if (operation == NV09F_SET_OPERATION_BLEND_AND) {
    return "BLENDAND";
  }
  if (operation == NV09F_SET_OPERATION_SRCCOPY) {
    return "SRCCOPY";
  }
  if (operation == NV09F_SET_OPERATION_SRCCOPY_PREMULT) {
    return "SRCCOPYPRE";
  }
  if (operation == NV09F_SET_OPERATION_BLEND_AND_PREMULT) {
    return "BLENDANDPRE";
  }

  char buf[16] = {0};
  snprintf(buf, 15, "%X", operation);
  return buf;
}

static std::string ColorFormatName(uint32_t format) {
  switch (format) {
    case NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8:
      return "XRGB";
    case NV04_SURFACE_2D_FORMAT_A8R8G8B8:
      return "ARGB";
    case NV04_SURFACE_2D_FORMAT_X8R8G8B8_Z8R8G8B8:
      return "ZRGB";
    default:
      break;
  }

  char buf[16] = {0};
  snprintf(buf, 15, "%X", format);
  return buf;
}
