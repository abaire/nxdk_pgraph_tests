#ifndef NXDK_PGRAPH_TESTS_BLEND_SURFACE_TESTS_H
#define NXDK_PGRAPH_TESTS_BLEND_SURFACE_TESTS_H

#include "test_host.h"
#include "test_suite.h"

//! Tests interactions of alpha blending with various surface formats.
class BlendSurfaceTests : public TestSuite {
 public:
  BlendSurfaceTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void Test(const std::string &name, TestHost::SurfaceColorFormat surface_format, uint32_t blend_func, uint32_t sfactor,
            uint32_t dfactor);
};

#endif  // NXDK_PGRAPH_TESTS_BLEND_SURFACE_TESTS_H
