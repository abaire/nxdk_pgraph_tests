#ifndef NXDK_PGRAPH_TESTS_IMAGE_BLIT_TESTS_H
#define NXDK_PGRAPH_TESTS_IMAGE_BLIT_TESTS_H

#include <pbkit/pbkit.h>

#include "test_suite.h"

class TestHost;

class ImageBlitTests : public TestSuite {
 public:
  ImageBlitTests(TestHost &host, std::string output_dir);

  std::string Name() override { return "Image blit"; }
  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();
  void Test(uint32_t operation);

  static std::string MakeTestName(uint32_t front_face);

  uint32_t image_pitch_;
  uint32_t image_height_;
  uint8_t *surface_memory_;

  struct s_CtxDma null_ctx_;
  struct s_CtxDma image_src_dma_ctx_;
  struct s_CtxDma clip_rect_ctx_;
  struct s_CtxDma beta_ctx_;
};

#endif  // NXDK_PGRAPH_TESTS_IMAGE_BLIT_TESTS_H
