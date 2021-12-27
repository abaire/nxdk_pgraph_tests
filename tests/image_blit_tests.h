#ifndef NXDK_PGRAPH_TESTS_IMAGE_BLIT_TESTS_H
#define NXDK_PGRAPH_TESTS_IMAGE_BLIT_TESTS_H

#include <pbkit/pbkit.h>

#include "test_suite.h"

class TestHost;

class ImageBlitTests : public TestSuite {
 public:
  struct BlitTest {
    uint32_t blit_operation;
    uint32_t buffer_color_format;
    uint32_t beta;
  };

 public:
  ImageBlitTests(TestHost& host, std::string output_dir);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void Test(const BlitTest& test);
  void ImageBlit(uint32_t operation, uint32_t beta, uint32_t source_channel, uint32_t destination_channel,
                 uint32_t surface_format, uint32_t source_pitch, uint32_t destination_pitch, uint32_t source_offset,
                 uint32_t source_x, uint32_t source_y, uint32_t destination_offset, uint32_t destination_x,
                 uint32_t destination_y, uint32_t width, uint32_t height, uint32_t clip_x, uint32_t clip_y,
                 uint32_t clip_width, uint32_t clip_height) const;

  static std::string MakeTestName(const BlitTest& test);

  uint32_t image_pitch_{0};
  uint32_t image_height_{0};
  uint8_t* source_image_{nullptr};

  struct s_CtxDma null_ctx_ {};
  struct s_CtxDma image_src_dma_ctx_ {};
  struct s_CtxDma clip_rect_ctx_ {};
  struct s_CtxDma beta_ctx_ {};
  struct s_CtxDma beta4_ctx_ {};
};

#endif  // NXDK_PGRAPH_TESTS_IMAGE_BLIT_TESTS_H
