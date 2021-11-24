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

static std::string OperationName(uint32_t operation);

static constexpr uint32_t kBlitOperations[] = {
    NV09F_SET_OPERATION_BLEND_AND,
    NV09F_SET_OPERATION_SRCCOPY,
    6,  // BLEND_PREMULT?
};

ImageBlitTests::ImageBlitTests(TestHost& host, std::string output_dir) : TestSuite(host, std::move(output_dir)) {
  for (auto operation : kBlitOperations) {
    std::string name = MakeTestName(operation);

    auto test = [this, operation]() { this->Test(operation); };
    tests_[name] = test;
  }

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

void ImageBlitTests::Initialize() {
  SDL_Surface* test_image = IMG_Load("D:\\image_blit\\TestImage.png");
  assert(test_image);

  image_pitch_ = test_image->pitch;
  image_height_ = test_image->h;
  uint32_t image_bytes = image_pitch_ * image_height_;

  surface_memory_ = static_cast<uint8_t*>(MmAllocateContiguousMemory(image_bytes));
  memcpy(surface_memory_, test_image->pixels, image_bytes);

  SDL_free(test_image);
}

void ImageBlitTests::Deinitialize() {
  MmFreeContiguousMemory(surface_memory_);
  surface_memory_ = nullptr;
}

static uint32_t* set_beta(uint32_t *p, float value) {
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

void ImageBlitTests::Test(uint32_t operation) {
  host_.PrepareDraw(0x00660033);

  uint32_t image_bytes = image_pitch_ * image_height_;
  pb_set_dma_address(&image_src_dma_ctx_, surface_memory_, image_bytes - 1);

  uint32_t in_x = 200;
  uint32_t in_y = 100;
  uint32_t out_x = 300;
  uint32_t out_y = 200;
  uint32_t width = 128;
  uint32_t height = 64;

  uint32_t clip_x = 0;
  uint32_t clip_y = 0;
  uint32_t clip_w = host_.GetFramebufferWidth();
  uint32_t clip_h = host_.GetFramebufferHeight();

  uint32_t dest_pitch = 4 * host_.GetFramebufferWidth();

  auto p = pb_begin();
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_OPERATION, operation);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY0, image_src_dma_ctx_.ChannelID);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY1, 11);  // DMA channel 11 - 0x1117

  p = pb_push1_to(SUBCH_CLASS_19, p, NV01_CONTEXT_CLIP_RECTANGLE_SET_POINT, (clip_y << 16) | clip_x);
  p = pb_push1_to(SUBCH_CLASS_19, p, NV01_CONTEXT_CLIP_RECTANGLE_SET_SIZE, (clip_h << 16) | clip_w);
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_CLIP_RECTANGLE, clip_rect_ctx_.ChannelID);

  p = pb_push1_to(SUBCH_CLASS_9F, p, 0x0120, 0);  // Sync read
  p = pb_push1_to(SUBCH_CLASS_9F, p, 0x0124, 1);  // Sync write
  p = pb_push1_to(SUBCH_CLASS_9F, p, 0x0128, 2);  // Modulo

  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_FORMAT,
                  7);  // NV062_SET_COLOR_FORMAT_LE_X8R8G8B8_O8R8G8B8

  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_PITCH, (dest_pitch << 16) | image_pitch_);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_OFFSET_SRC, 0);
  p = pb_push1_to(SUBCH_CLASS_62, p, NV10_CONTEXT_SURFACES_2D_OFFSET_DST, 0);

  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_COLOR_KEY, null_ctx_.ChannelID);
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_PATTERN, null_ctx_.ChannelID);
  p = pb_push1_to(SUBCH_CLASS_9F, p, NV_IMAGE_BLIT_ROP5, null_ctx_.ChannelID);

  if (operation == NV09F_SET_OPERATION_SRCCOPY) {
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

  std::string op_name = OperationName(operation);
  pb_print("Op: %s\n", op_name.c_str());
  pb_draw_text_screen();

  std::string name = MakeTestName(operation);
  //  host_.FinishDrawAndSave(output_dir_.c_str(), name.c_str());

  host_.FinishDraw();
}

std::string ImageBlitTests::MakeTestName(uint32_t front_face) {
  std::string ret = "ImageBlit_";
  ret += OperationName(front_face);
  return std::move(ret);
}

static std::string OperationName(uint32_t operation) {
  if (operation == NV09F_SET_OPERATION_BLEND_AND) {
    return "BLEND_AND";
  }
  if (operation == NV09F_SET_OPERATION_SRCCOPY) {
    return "SRCCOPY";
  }

  char buf[16] = {0};
  snprintf(buf, 15, "%X", operation);
  return buf;
}
