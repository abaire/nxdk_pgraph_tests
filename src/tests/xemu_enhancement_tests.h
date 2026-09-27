#ifndef NXDK_PGRAPH_TESTS_XEMU_ENHANCEMENT_TESTS_H
#define NXDK_PGRAPH_TESTS_XEMU_ENHANCEMENT_TESTS_H

#include "test_suite.h"

/**
 * Tests various enhancements (changes that intentionally depart from hardware) specific to the xemu emulator
 * (https://semu.app).
 */
class XemuEnhancementTests : public TestSuite {
 public:
  XemuEnhancementTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  //! Tests the round-trip fidelity of CPU-written surface data through xemu's internal render scale path.
  void TestXemuScaledSurfaceUploadFilter();
};

#endif  // NXDK_PGRAPH_TESTS_XEMU_ENHANCEMENT_TESTS_H
