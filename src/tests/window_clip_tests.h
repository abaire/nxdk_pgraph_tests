#ifndef NXDK_PGRAPH_TESTS_WINDOW_CLIP_TESTS_H
#define NXDK_PGRAPH_TESTS_WINDOW_CLIP_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

namespace PBKitPlusPlus {
class VertexBuffer;
}

class WindowClipTests : public TestSuite {
 public:
  struct ClipRect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
  };

 public:
  WindowClipTests(TestHost &host, std::string output_dir, const Config &config);
  void Initialize() override;

 private:
  void Test(bool clip_exclusive, bool swap_order, const ClipRect &clip1, const ClipRect &clip2);
  void TestRenderTarget(bool clip_exclusive, bool swap_order, const ClipRect &clip1, const ClipRect &clip2);

  void Draw(bool clip_exclusive, bool swap_order, uint32_t c1_left, uint32_t c1_top, uint32_t c1_right,
            uint32_t c1_bottom, uint32_t c2_left, uint32_t c2_top, uint32_t c2_right, uint32_t c2_bottom);

 private:
  std::shared_ptr<PBKitPlusPlus::VertexBuffer> framebuffer_vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_WINDOW_CLIP_TESTS_H
