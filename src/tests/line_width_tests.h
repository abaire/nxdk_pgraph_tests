#ifndef NXDK_PGRAPH_TESTS_LINE_WIDTH_TESTS_H
#define NXDK_PGRAPH_TESTS_LINE_WIDTH_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class TestHost;
class VertexBuffer;

// Tests behavior of 0x380 - glLineWidth
class LineWidthTests : public TestSuite {
 public:
  LineWidthTests(TestHost& host, std::string output_dir);

  void Initialize() override;
  void TearDownTest() override;

  // Line widths appear to be a 6.3 fixed point integer (so 0x08 => 1.000 is a width of 1).
  typedef uint32_t fixed_t;

 private:
  void Test(const std::string& name, bool fill, fixed_t line_width);
};

#endif  // NXDK_PGRAPH_TESTS_LINE_WIDTH_TESTS_H
