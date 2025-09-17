#ifndef NXDK_PGRAPH_TESTS_CLEAR_TESTS_H
#define NXDK_PGRAPH_TESTS_CLEAR_TESTS_H

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests the behavior of various write masks on NV097_CLEAR_SURFACE.
 *
 * Color mask (C*) tests:
 *   Each test draws some geometry, then modifies the NV097_SET_COLOR_MASK,
 *   NV097_SET_DEPTH_MASK, and NV097_SET_STENCIL_MASK settings, then invokes
 *   NV097_CLEAR_SURFACE.
 *
 *   In all cases, the clear color is set to 0x7F7F7F7F and the depth value is
 *   set to 0xFFFFFFFF.
 *
 * Surface format color (SFC*) tests:
 *   Each test clears a render target surface and then renders it as a texture
 *   with alpha blending disabled in order to test the interaction of surface
 *   format with NV097_CLEAR_SURFACE_COLOR.
 */
class ClearTests : public TestSuite {
 public:
  ClearTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void CreateGeometry();

  void TestColorMask(uint32_t color_mask, bool depth_write_enable);
  void TestSurfaceFmt(TestHost::SurfaceColorFormat surface_format, const std::string& name);
};
#endif  // NXDK_PGRAPH_TESTS_CLEAR_TESTS_H
