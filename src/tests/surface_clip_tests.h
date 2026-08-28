#ifndef NXDK_PGRAPH_TESTS_SURFACE_CLIP_TESTS_H
#define NXDK_PGRAPH_TESTS_SURFACE_CLIP_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests behavior of NV097_SET_SURFACE_CLIP_HORIZONTAL and
 * NV097_SET_SURFACE_CLIP_VERTICAL.
 */
class SurfaceClipTests : public TestSuite {
 public:
  struct ClipRect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
  };

 public:
  SurfaceClipTests(TestHost &host, std::string output_dir, const Config &config);
  void Initialize() override;

 private:
  //! Tests surface clipping rectangles with the specified surface color format.
  void Test(const std::string &name, const ClipRect &rect, TestHost::SurfaceColorFormat color_format);

  //! Tests surface clipping rectangles on non-framebuffer render target surfaces.
  void TestRenderTarget(const std::string &name, const ClipRect &rect);

  //! Tests surface clip boundary calculations and offset handling.
  void TestXemuBug420();

  //! Tests that debug text rendering respects active surface clipping boundaries.
  void TestDebugTextIsClipped();

  void DrawTestImage(const ClipRect &rect);
};

#endif  // NXDK_PGRAPH_TESTS_SURFACE_CLIP_TESTS_H
