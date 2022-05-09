#ifndef NXDK_PGRAPH_TESTS_WINDOW_CLIP_TESTS_H
#define NXDK_PGRAPH_TESTS_WINDOW_CLIP_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class WindowClipTests : public TestSuite {
 public:
  struct ClipRect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
  };

 public:
  WindowClipTests(TestHost &host, std::string output_dir);

 private:
  void Test(bool clip_exclusive, bool swap_order, const ClipRect &clip1, const ClipRect &clip2);
};

#endif  // NXDK_PGRAPH_TESTS_WINDOW_CLIP_TESTS_H
