#ifndef NXDK_PGRAPH_TESTS_ZPASS_PIXEL_COUNT_TESTS_H
#define NXDK_PGRAPH_TESTS_ZPASS_PIXEL_COUNT_TESTS_H

#include <pbkit/pbkit.h>

#include <cstdint>
#include <memory>
#include <string>

#include "test_suite.h"

class TestHost;

/**
 * Tests behavior of NV097_GET_REPORT.
 */
class ZPassPixelCountTests : public TestSuite {
  // From xemu
  // https://github.com/xemu-project/xemu/blob/c67843384031e0edaab3db91ec496464b931789b/hw/xbox/nv2a/pgraph/pgraph.c#L3128
  struct ZPassReport {
    uint64_t timestamp;
    uint32_t report;
    uint32_t reserved;  // xemu calls this "done" but always sets it to zero. HW also seems to always set this to zero.
  };

 public:
  ZPassPixelCountTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void Test();

  void TestPointSize(uint32_t point_size);
  void TestPointSizeProgrammable(uint32_t point_size);

  void TestLineWidth(uint32_t line_width);

 private:
  struct s_CtxDma semaphore_dma_ctx_{};
  struct s_CtxDma report_dma_ctx_{};

  uint32_t *semaphore_context_object_{nullptr};
  ZPassReport *report_context_object_{nullptr};
};

#endif  // NXDK_PGRAPH_TESTS_ZPASS_PIXEL_COUNT_TESTS_H
