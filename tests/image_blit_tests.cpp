#include "image_blit_tests.h"

#include <SDL_image.h>
#include <pbkit/pbkit.h>

#include "../nxdk_ext.h"
#include "../pbkit_ext.h"
#include "../test_host.h"
#include "debug_output.h"
#include "vertex_buffer.h"

// From pbkit.c
#define MAXRAM 0x03FFAFFF

// See pb_init in pbkit.c, where the channel contexts are set up.
// SUBCH_3 == GR_CLASS_9F, which contains the IMAGE_BLIT commands.
#define SUBCH_CLASS_9F SUBCH_3
// SUBCH_4 == GR_CLASS_62, which contains the NV10 2D surface commands.
#define SUBCH_CLASS_62 SUBCH_4

// Subchannel reserved for interaction with the class 19 channel.
#define SUBCH_CLASS_19 5

// Subchannel reserved for interaction with the class 12 channel.
#define SUBCH_CLASS_12 6

// Initial color for the compositing buffer.
#define COMPOSITING_BUFFER_COLOR 0x00CC2222

#define SOURCE_X 8
#define SOURCE_Y 8
#define SOURCE_WIDTH 128
#define SOURCE_HEIGHT 128
#define DESTINATION_X 256
#define DESTINATION_Y 176

static std::string OperationName(uint32_t operation);
static std::string ColorFormatName(uint32_t format);

static constexpr ImageBlitTests::BlitTest kTests[] = {
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0.5f, false},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_A8R8G8B8, 0.5f, false},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_A8R8G8B8, 0.5f, true},
    {NV09F_SET_OPERATION_SRCCOPY, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8, 0, false},
    {NV09F_SET_OPERATION_SRCCOPY, NV04_SURFACE_2D_FORMAT_A8R8G8B8, 0, false},
    {NV09F_SET_OPERATION_SRCCOPY, NV04_SURFACE_2D_FORMAT_A8R8G8B8, 0, true},
};

ImageBlitTests::ImageBlitTests(TestHost& host, std::string output_dir) : TestSuite(host, std::move(output_dir)) {
  for (auto test : kTests) {
    std::string name = MakeTestName(test);

    auto test_method = [this, test]() { this->Test(test); };
    tests_[name] = test_method;
  }
}

void ImageBlitTests::Initialize() {
  SetDefaultTextureFormat();

  SDL_Surface* temp = IMG_Load("D:\\image_blit\\TestImage.png");
  assert(temp);
  SDL_Surface* test_image = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_BGRA32, 0);
  SDL_free(temp);

  image_pitch_ = test_image->pitch;
  image_height_ = test_image->h;
  uint32_t image_bytes = image_pitch_ * image_height_;

  source_image_ = static_cast<uint8_t*>(MmAllocateContiguousMemory(image_bytes));
  memcpy(source_image_, test_image->pixels, image_bytes);
  SDL_free(test_image);

  uint32_t compositing_bytes = 4 * SOURCE_WIDTH * SOURCE_HEIGHT;
  compositing_image_ = static_cast<uint8_t*>(MmAllocateContiguousMemory(compositing_bytes));
  std::fill_n(reinterpret_cast<uint32_t*>(compositing_image_), compositing_bytes >> 2, COMPOSITING_BUFFER_COLOR);

  // DONOTSUBMIT
  auto foo = source_image_;
  for (int y = 0; y < test_image->h; ++y) {
    for (int x = 0; x < test_image->w; ++x) {
      foo[3] = 0x33;
      foo += 4;
    }
  }

  // TODO: Provide a mechanism to find the next unused channel.
  auto channel = 20;

  pb_create_dma_ctx(channel++, DMA_CLASS_3D, 0, MAXRAM, &image_src_dma_ctx_);
  pb_bind_channel(&image_src_dma_ctx_);

  pb_create_dma_ctx(channel++, DMA_CLASS_3D, 0, MAXRAM, &image_composite_dma_ctx_);
  pb_bind_channel(&image_composite_dma_ctx_);

  pb_create_gr_ctx(channel++, GR_CLASS_30, &null_ctx_);
  pb_bind_channel(&null_ctx_);

  pb_create_gr_ctx(channel++, GR_CLASS_19, &clip_rect_ctx_);
  pb_bind_channel(&clip_rect_ctx_);
  // TODO: Provide a mechanism to find the next unused subchannel.
  pb_bind_subchannel(SUBCH_CLASS_19, &clip_rect_ctx_);

  pb_create_gr_ctx(channel++, GR_CLASS_12, &beta_ctx_);
  pb_bind_channel(&beta_ctx_);
  pb_bind_subchannel(SUBCH_CLASS_12, &beta_ctx_);
}

void ImageBlitTests::Deinitialize() {
  MmFreeContiguousMemory(source_image_);
  source_image_ = nullptr;
  MmFreeContiguousMemory(compositing_image_);
  compositing_image_ = nullptr;
}

void ImageBlitTests::Render2D(uint32_t operation, uint32_t beta, uint32_t source_channel, uint32_t destination_channel,
                              uint32_t surface_format, uint32_t source_pitch, uint32_t destination_pitch,
                              uint32_t source_offset, uint32_t source_x, uint32_t source_y, uint32_t destination_offset,
                              uint32_t destination_x, uint32_t destination_y, uint32_t width, uint32_t height,
                              uint32_t clip_x, uint32_t clip_y, uint32_t clip_width, uint32_t clip_height) {
  auto p = pb_begin();
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_OPERATION, operation);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY0, source_channel);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY1, destination_channel);

  p = pb_push1_to(SUBCH_CLASS_19, p, NV01_CONTEXT_CLIP_RECTANGLE_SET_POINT, (clip_y << 16) | clip_x);
  p = pb_push1_to(SUBCH_CLASS_19, p, NV01_CONTEXT_CLIP_RECTANGLE_SET_SIZE, (clip_height << 16) | clip_width);
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_CLIP_RECTANGLE, clip_rect_ctx_.ChannelID);

  p = pb_push1_to(SUBCH_CLASS_9F, p, 0x0120, 0);  // Sync read
  p = pb_push1_to(SUBCH_CLASS_9F, p, 0x0124, 1);  // Sync write
  p = pb_push1_to(SUBCH_CLASS_9F, p, 0x0128, 2);  // Modulo

  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_FORMAT, surface_format);

  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_PITCH, source_pitch | (destination_pitch << 16));
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_OFFSET_SRC, source_offset);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_OFFSET_DST, destination_offset);

  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_COLOR_KEY, null_ctx_.ChannelID);
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_PATTERN, null_ctx_.ChannelID);
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_ROP5, null_ctx_.ChannelID);

  if (operation == NV09F_SET_OPERATION_SRCCOPY) {
    p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SET_BETA, null_ctx_.ChannelID);
  } else {
    p = pb_push1_to(SUBCH_CLASS_12, p, NV012_SET_BETA, beta);
    p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SET_BETA, beta_ctx_.ChannelID);
  }
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SET_BETA4, null_ctx_.ChannelID);

  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_POINT_IN, source_x | (source_y << 16));
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_POINT_OUT, destination_x | (destination_y << 16));
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SIZE, width | (height << 16));

  pb_end(p);
}

void ImageBlitTests::Test(const BlitTest& test) {
  host_.PrepareDraw(0x00660033);

  uint32_t image_bytes = image_pitch_ * image_height_;
  pb_set_dma_address(&image_src_dma_ctx_, source_image_, image_bytes - 1);
  if (test.composite_first) {
    pb_set_dma_address(&image_composite_dma_ctx_, compositing_image_, image_bytes - 1);
  }

  uint32_t clip_x = 0;
  uint32_t clip_y = 0;
  uint32_t clip_w = host_.GetFramebufferWidth();
  uint32_t clip_h = host_.GetFramebufferHeight();

  if (test.composite_first) {
    Render2D(test.blit_operation, test.GetBeta(), image_src_dma_ctx_.ChannelID, image_composite_dma_ctx_.ChannelID,
             test.buffer_color_format, image_pitch_, SOURCE_WIDTH << 2, 0, SOURCE_X, SOURCE_Y, 0, 0, 0, SOURCE_WIDTH,
             SOURCE_HEIGHT, clip_x, clip_y, clip_w, clip_h);

    while (pb_busy()) {
    }

    Render2D(NV09F_SET_OPERATION_SRCCOPY, 0, image_composite_dma_ctx_.ChannelID,
             11,  // DMA channel 11 - 0x1117
             test.buffer_color_format, SOURCE_WIDTH << 2, host_.GetFramebufferWidth() << 2, 0, 0, 0, 0, DESTINATION_X,
             DESTINATION_Y, SOURCE_WIDTH, SOURCE_HEIGHT, clip_x, clip_y, clip_w, clip_h);
  } else {
    Render2D(test.blit_operation, test.GetBeta(), image_src_dma_ctx_.ChannelID,
             11,  // DMA channel 11 - 0x1117
             test.buffer_color_format, image_pitch_, 4 * host_.GetFramebufferWidth(), 0, SOURCE_X, SOURCE_Y, 0,
             DESTINATION_X, DESTINATION_Y, SOURCE_WIDTH, SOURCE_HEIGHT, clip_x, clip_y, clip_w, clip_h);
  }

  std::string op_name = OperationName(test.blit_operation);
  pb_print("Op: %s\n", op_name.c_str());
  std::string color_format_name = ColorFormatName(test.buffer_color_format);
  pb_print("BufFmt: %s\n", color_format_name.c_str());
  if (test.blit_operation != NV09F_SET_OPERATION_SRCCOPY) {
    pb_print("Beta: %d\n", test.GetBeta());
  }
  if (test.composite_first) {
    pb_print("Composite\n");
  }
  pb_draw_text_screen();

  std::string name = MakeTestName(test);
  //  host_.FinishDrawAndSave(output_dir_.c_str(), name.c_str());

  host_.FinishDraw();
}

std::string ImageBlitTests::MakeTestName(const BlitTest& test) {
  char buf[256] = {0};
  snprintf(buf, 255, "ImageBlt_%s_%s_B%08X_C%s", OperationName(test.blit_operation).c_str(),
           ColorFormatName(test.buffer_color_format).c_str(), test.GetBeta(), test.composite_first ? "Y" : "N");
  return buf;
}

static std::string OperationName(uint32_t operation) {
  if (operation == NV09F_SET_OPERATION_BLEND_AND) {
    return "BLENDAND";
  }
  if (operation == NV09F_SET_OPERATION_SRCCOPY) {
    return "SRCCOPY";
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
  }

  char buf[16] = {0};
  snprintf(buf, 15, "%X", format);
  return buf;
}

uint32_t ImageBlitTests::BlitTest::GetBeta() const {
  uint32_t int_val;

  // Sets the beta factor. The parameter is a signed fixed-point number with a sign bit and 31 fractional bits.
  // Note that negative values are clamped to 0, and only 8 fractional bits are actually implemented in hardware.

  float beta_val = beta;
  if (beta_val < 0.0f) {
    int_val = 0;
  } else {
    if (beta_val > 1.0f) {
      beta_val = 1.0f;
    }

    constexpr uint32_t kMaxValue = 0x7f800000;
    int_val = static_cast<int>(beta_val * static_cast<float>(kMaxValue));
    int_val &= kMaxValue;
  }
  return int_val;
}
