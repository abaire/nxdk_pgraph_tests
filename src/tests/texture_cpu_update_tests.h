#ifndef NXDK_PGRAPH_TESTS_TEXTURE_CPU_UPDATE_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_CPU_UPDATE_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests dynamic in-place CPU modifications to active texture memory.
 */
class TextureCPUUpdateTests : public TestSuite {
 public:
  TextureCPUUpdateTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void CreateGeometry();

  //! Tests updating RGBA texture memory directly from the CPU between draw calls.
  void TestRGBA();

  //! Tests updating palettized texture memory and palette tables directly from the CPU.
  void TestPalettized();

  //! Tests reuse of a texture across multiple draws in a single frame with CPU modification performed between each
  //! draw.
  void TestMultipleSwatches();

 private:
  struct s_CtxDma semaphore_dma_ctx_{};
  struct s_CtxDma monochrome_ctx_{};

  uint32_t* semaphore_context_object_{nullptr};
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_CPU_UPDATE_TESTS_H
