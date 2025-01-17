#ifndef NXDK_PGRAPH_TESTS_NULL_SURFACE_TESTS_H
#define NXDK_PGRAPH_TESTS_NULL_SURFACE_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

class NullSurfaceTests : public TestSuite {
 public:
  NullSurfaceTests(TestHost& host, std::string output_dir, const Config& config);
  void Initialize() override;

 private:
  void TestNullColor();
  void TestNullZeta();

  void TestXemuBug893();

  // Makes the color and zeta surfaces use fully addressable VRAM.
  void SetSurfaceDMAs() const;
  // Restores color and zeta surface DMA settings.
  void RestoreSurfaceDMAs() const;

  void DrawTestQuad(bool swizzle) const;
};

#endif  // NXDK_PGRAPH_TESTS_NULL_SURFACE_TESTS_H
