#ifndef NXDK_PGRAPH_TESTS_SURFACE_FORMAT_TESTS_H
#define NXDK_PGRAPH_TESTS_SURFACE_FORMAT_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests behavior of NV097_SET_SURFACE_FORMAT_COLOR. Tests render two quads into a render target with the surface
 * color format under test. The left quad consists of 4 color gradient patches (red, yellow, cyan, blue) with alpha
 * forced to fully opaque. The second quad uses the same color gradients but has alpha set in a radial pattern such that
 * it is 0 at the corners and fully opaque in the center.
 *
 * The resultant surface is then rendered to an RGBA framebuffer twice, once at the top of the
 * screen with the alpha channel preserved, and again at the bottom of the screen with alpha forced to fully opaque.
 */
class SurfaceFormatTests : public TestSuite {
 public:
  struct ClipRect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
  };

 public:
  SurfaceFormatTests(TestHost &host, std::string output_dir, const Config &config);
  void Initialize() override;

 private:
  void Test(const std::string &name, TestHost::SurfaceColorFormat color_format);

  void RenderToTextureStart(TestHost::SurfaceColorFormat color_format) const;
  void RenderToTextureEnd() const;
};

#endif  // NXDK_PGRAPH_TESTS_SURFACE_FORMAT_TESTS_H
