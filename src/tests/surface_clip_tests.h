#ifndef NXDK_PGRAPH_TESTS_SURFACE_CLIP_TESTS_H
#define NXDK_PGRAPH_TESTS_SURFACE_CLIP_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

class SurfaceClipTests : public TestSuite {
 public:
  struct ClipRect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
  };

 public:
  SurfaceClipTests(TestHost &host, std::string output_dir);
  void Initialize() override;

 private:
  void Test(const ClipRect &rect);
  void TestRenderTarget(const ClipRect &rect);

  void TestXemuBug420();

  void DrawTestImage(const ClipRect &rect);
};

#endif  // NXDK_PGRAPH_TESTS_SURFACE_CLIP_TESTS_H
