#ifndef NXDK_PGRAPH_TESTS_TEXTURE_CPU_UPDATE_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_CPU_UPDATE_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

class TextureCPUUpdateTests : public TestSuite {
 public:
  TextureCPUUpdateTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void CreateGeometry();

  void TestRGBA();
  void TestPalettized();

  //! Tests reuse of a texture across multiple draws in a single frame with CPU modification performed between each
  //! draw.
  void TestMultipleSwatches();

 private:
  struct s_CtxDma semaphore_dma_ctx_ {};
  struct s_CtxDma monochrome_ctx_ {};

  uint32_t* semaphore_context_object_{nullptr};
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_CPU_UPDATE_TESTS_H
