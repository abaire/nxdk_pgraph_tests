#pragma once

#include <memory>
#include <string>
#include <vector>

#include "test_host.h"
#include "test_suite.h"
#include "vertex_buffer.h"

/**
 * Tests line smoothing and antialiasing control registers (NV097_SET_SMOOTH_CONTROL).
 */
class SmoothingTests : public TestSuite {
 public:
  enum DrawMode {
    DRAW_ARRAYS,
    DRAW_INLINE_BUFFERS,
    DRAW_INLINE_ARRAYS,
    DRAW_INLINE_ELEMENTS,
  };

 public:
  SmoothingTests(TestHost& host, std::string output_dir, const Config& config);
  void Initialize() override;

 private:
  //! Tests primitive smoothing and antialiasing with the given smooth control flags.
  void Test(const std::string& name, uint32_t smooth_control);
};
