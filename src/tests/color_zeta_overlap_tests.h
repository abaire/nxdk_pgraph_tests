#ifndef NXDK_PGRAPH_TESTS_COLOR_ZETA_OVERLAP_TESTS_H
#define NXDK_PGRAPH_TESTS_COLOR_ZETA_OVERLAP_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

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
  void TestColorIntoDepth();
  void TestDepthIntoColor();
  void TestSwap();

  // Tests the behavior identified in xemu #405
  void TestXemuAdjacentSurfaceWithClipOffset(bool swizzle);
  void TestXemuAdjacentSurfaceWithAA();

  void SetSurfaceDMAs() const;
  void RestoreSurfaceDMAs() const;
};

#endif  // NXDK_PGRAPH_TESTS_COLOR_ZETA_OVERLAP_TESTS_H
