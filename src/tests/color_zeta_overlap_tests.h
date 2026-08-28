#ifndef NXDK_PGRAPH_TESTS_COLOR_ZETA_OVERLAP_TESTS_H
#define NXDK_PGRAPH_TESTS_COLOR_ZETA_OVERLAP_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests behavior when the color buffer and z/stencil buffers overlap in RAM.
 *
 * WARNING: These tests are non-deterministic on Xbox hardware. The primary
 * intent of these tests is to verify that emulators do not crash.
 */
class ColorZetaOverlapTests : public TestSuite {
 public:
  ColorZetaOverlapTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  //! Tests rendering where the color buffer is placed in the depth buffer's memory region.
  void TestColorIntoDepth();

  //! Tests rendering where the depth buffer is placed in the color buffer's memory region.
  void TestDepthIntoColor();

  //! Tests swapping color and depth surface base addresses in VRAM.
  void TestSwap();

  //! Tests rendering to adjacent surfaces with clip offset configurations.
  void TestXemuAdjacentSurfaceWithClipOffset(bool swizzle);

  //! Tests rendering to adjacent surfaces with antialiasing enabled.
  void TestXemuAdjacentSurfaceWithAA();

  void SetSurfaceDMAs() const;
  void RestoreSurfaceDMAs() const;
};

#endif  // NXDK_PGRAPH_TESTS_COLOR_ZETA_OVERLAP_TESTS_H
