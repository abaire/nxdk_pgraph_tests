#ifndef NXDK_PGRAPH_TESTS_FOG_CARRYOVER_TESTS_H
#define NXDK_PGRAPH_TESTS_FOG_CARRYOVER_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class TestHost;

/**
 * Tests the behavior when fog is enabled but the fog coordinate is not specified by the shader.
 *
 * This test reproduces behavior observed in "Blade 2" https://github.com/xemu-project/xemu/issues/1852
 */
class FogCarryoverTests : public TestSuite {
 public:
  FogCarryoverTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  //! Tests fog coordinate carryover across consecutive draw calls.
  void Test();

  //! Tests fog coordinate carryover for a specific primitive type.
  void TestPrimitive(const std::string& name, TestHost::DrawPrimitive primitive);
};

#endif  // NXDK_PGRAPH_TESTS_FOG_CARRYOVER_TESTS_H
