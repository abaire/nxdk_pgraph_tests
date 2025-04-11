#ifndef NXDK_PGRAPH_TESTS_SURFACE_FORMAT_TESTS_H
#define NXDK_PGRAPH_TESTS_SURFACE_FORMAT_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests behavior of NV097_SET_SURFACE_CLIP_HORIZONTAL and
 * NV097_SET_SURFACE_CLIP_VERTICAL.
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
