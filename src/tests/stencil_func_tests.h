#ifndef NXDK_PGRAPH_TESTS_STENCIL_FUNC_TESTS_H
#define NXDK_PGRAPH_TESTS_STENCIL_FUNC_TESTS_H

#include <cstdint>
#include <memory>
#include <string>

#include "test_suite.h"

class TestHost;

/**
 * Tests the behavior of NV097_SET_STENCIL_FUNC (0x00000364).
 *
 * Each test renders a series of red quads unconditionally, then renders an
 * invisible quad on top to set the value of the stencil buffer to various test
 * values. Finally, a green quad is rendered across all test patches using the
 * selected stencil function, causing areas that pass the function to turn
 * green, while failing pixels remain red.
 */
class StencilFuncTests : public TestSuite {
 public:
  StencilFuncTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void Test(const std::string &name, uint32_t stencil_func);
};

#endif  // NXDK_PGRAPH_TESTS_STENCIL_FUNC_TESTS_H
