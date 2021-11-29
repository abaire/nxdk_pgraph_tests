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

#define SOURCE_OFFSET_X 100
#define SOURCE_OFFSET_y 16
#define SOURCE_WIDTH 126
#define SOURCE_HEIGHT 116

static std::string OperationName(uint32_t operation);
static std::string ColorFormatName(uint32_t format);

static constexpr ImageBlitTests::BlitTest kTests[] = {
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8},
    {NV09F_SET_OPERATION_SRCCOPY, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8},
    {NV09F_SET_OPERATION_BLEND_AND, NV04_SURFACE_2D_FORMAT_A8R8G8B8},
    {NV09F_SET_OPERATION_SRCCOPY, NV04_SURFACE_2D_FORMAT_A8R8G8B8},
    //    {6, NV04_SURFACE_2D_FORMAT_X8R8G8B8_X8R8G8B8},  // BLEND_PREMULT?
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

  SDL_Surface* test_image = IMG_Load("D:\\image_blit\\TestImage.png");
  assert(test_image);

  image_pitch_ = test_image->pitch;
  image_height_ = test_image->h;
  uint32_t image_bytes = image_pitch_ * image_height_;

  surface_memory_ = static_cast<uint8_t*>(MmAllocateContiguousMemory(image_bytes));
  memcpy(surface_memory_, test_image->pixels, image_bytes);
  SDL_free(test_image);

  // TODO: Provide a mechanism to find the next unused channel.
  pb_create_dma_ctx(20, DMA_CLASS_3D, 0, MAXRAM, &image_src_dma_ctx_);
  pb_bind_channel(&image_src_dma_ctx_);

  pb_create_gr_ctx(21, GR_CLASS_30, &null_ctx_);
  pb_bind_channel(&null_ctx_);

  pb_create_gr_ctx(22, GR_CLASS_19, &clip_rect_ctx_);
  pb_bind_channel(&clip_rect_ctx_);

  // TODO: Provide a mechanism to find the next unused subchannel.
  pb_bind_subchannel(SUBCH_CLASS_19, &clip_rect_ctx_);

  pb_create_gr_ctx(23, GR_CLASS_12, &beta_ctx_);
  pb_bind_channel(&beta_ctx_);
  pb_bind_subchannel(SUBCH_CLASS_12, &beta_ctx_);
}

void ImageBlitTests::Deinitialize() {
  MmFreeContiguousMemory(surface_memory_);
  surface_memory_ = nullptr;
}

static uint32_t* set_beta(uint32_t* p, float value) {
  // Sets the beta factor. The parameter is a signed fixed-point number with a sign bit and 31 fractional bits.
  // Note that negative values are clamped to 0, and only 8 fractional bits are actually implemented in hardware.
  uint32_t int_val;
  if (value < 0.0f) {
    int_val = 0;
  } else {
    if (value > 1.0f) {
      value = 1.0f;
    }

    constexpr uint32_t kMaxValue = 0x7f800000;
    int_val = static_cast<int>(value * static_cast<float>(kMaxValue));
    int_val &= kMaxValue;
  }

  p = pb_push1_to(SUBCH_CLASS_12, p, NV012_SET_BETA, int_val);
  return p;
}

void ImageBlitTests::Test(const BlitTest& test) {
  host_.PrepareDraw(0x00660033);

  uint32_t image_bytes = image_pitch_ * image_height_;
  pb_set_dma_address(&image_src_dma_ctx_, surface_memory_, image_bytes - 1);

  uint32_t in_x = SOURCE_OFFSET_X;
  uint32_t in_y = SOURCE_OFFSET_y;
  uint32_t out_x = 300;
  uint32_t out_y = 200;
  uint32_t width = SOURCE_WIDTH;
  uint32_t height = SOURCE_HEIGHT;

  uint32_t clip_x = 0;
  uint32_t clip_y = 0;
  uint32_t clip_w = host_.GetFramebufferWidth();
  uint32_t clip_h = host_.GetFramebufferHeight();

  uint32_t dest_pitch = 4 * host_.GetFramebufferWidth();

  auto p = pb_begin();
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_OPERATION, test.blit_operation);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY0, image_src_dma_ctx_.ChannelID);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY1, 11);  // DMA channel 11 - 0x1117

  p = pb_push1_to(SUBCH_CLASS_19, p, NV01_CONTEXT_CLIP_RECTANGLE_SET_POINT, (clip_y << 16) | clip_x);
  p = pb_push1_to(SUBCH_CLASS_19, p, NV01_CONTEXT_CLIP_RECTANGLE_SET_SIZE, (clip_h << 16) | clip_w);
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_CLIP_RECTANGLE, clip_rect_ctx_.ChannelID);

  p = pb_push1_to(SUBCH_CLASS_9F, p, 0x0120, 0);  // Sync read
  p = pb_push1_to(SUBCH_CLASS_9F, p, 0x0124, 1);  // Sync write
  p = pb_push1_to(SUBCH_CLASS_9F, p, 0x0128, 2);  // Modulo

  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_FORMAT, test.buffer_color_format);

  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_PITCH, (dest_pitch << 16) | image_pitch_);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_OFFSET_SRC, 0);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_OFFSET_DST, 0);

  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_COLOR_KEY, null_ctx_.ChannelID);
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_PATTERN, null_ctx_.ChannelID);
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_ROP5, null_ctx_.ChannelID);

  if (test.blit_operation == NV09F_SET_OPERATION_SRCCOPY) {
    p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SET_BETA, null_ctx_.ChannelID);
  } else {
    p = set_beta(p, 0.85f);
    p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SET_BETA, beta_ctx_.ChannelID);
  }
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SET_BETA4, null_ctx_.ChannelID);

  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_POINT_IN, in_x | (in_y << 16));     // 0x300
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_POINT_OUT, out_x | (out_y << 16));  // 0x304
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_SIZE, width | (height << 16));      // 0x308

  pb_end(p);

  std::string op_name = OperationName(test.blit_operation);
  pb_print("Op: %s\n", op_name.c_str());
  std::string color_format_name = ColorFormatName(test.buffer_color_format);
  pb_print("BufFmt: %s\n", color_format_name.c_str());
  pb_draw_text_screen();

  std::string name = MakeTestName(test);
  //  host_.FinishDrawAndSave(output_dir_.c_str(), name.c_str());

  host_.FinishDraw();
}

std::string ImageBlitTests::MakeTestName(const BlitTest& test) {
  std::string ret = "ImageBlit_";
  ret += OperationName(test.blit_operation);
  ret += "_";
  ret += ColorFormatName(test.buffer_color_format);
  return std::move(ret);
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