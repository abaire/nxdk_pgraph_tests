#ifndef NXDK_PGRAPH_TESTS_FOG_EXCEPTIONAL_VALUE_TESTS_H
#define NXDK_PGRAPH_TESTS_FOG_EXCEPTIONAL_VALUE_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;

/**
 * Tests the effects of exceptional values (e.g., inf, nan) on the vertex fog
 * coordinate. Fog is set to bright red.
 *
 * Each test renders a series of blueish quads with their fog coordinate set to
 * interesting test values to observe the resulting behavior.
 */
class FogExceptionalValueTests : public TestSuite {
 public:
  FogExceptionalValueTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void Test(const std::string& name, uint32_t fog_mode, uint32_t fog_gen_mode);
};

#endif  // NXDK_PGRAPH_TESTS_FOG_EXCEPTIONAL_VALUE_TESTS_H
