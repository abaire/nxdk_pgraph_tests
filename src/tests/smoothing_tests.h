#pragma once

#include <memory>
#include <string>
#include <vector>

#include "test_host.h"
#include "test_suite.h"
#include "vertex_buffer.h"

class VertexBuffer;

// Tests behavior when lighting is enabled and color components are requested from various sources.
class SmoothingTests : public TestSuite {
 public:
  enum DrawMode {
    DRAW_ARRAYS,
    DRAW_INLINE_BUFFERS,
    DRAW_INLINE_ARRAYS,
    DRAW_INLINE_ELEMENTS,
  };

 public:
  SmoothingTests(TestHost& host, std::string output_dir);
  void Initialize() override;

 private:
  void Test(const std::string &name, uint32_t smooth_control);
};
