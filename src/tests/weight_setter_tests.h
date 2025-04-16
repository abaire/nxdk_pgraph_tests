#ifndef NXDK_PGRAPH_TESTS_WEIGHT_SETTER_TESTS_H
#define NXDK_PGRAPH_TESTS_WEIGHT_SETTER_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;

/**
 * Tests the effects of NV097_SET_WEIGHTxF.
 */
class WeightSetterTests : public TestSuite {
 public:
  WeightSetterTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void Test();
};

#endif  // NXDK_PGRAPH_TESTS_WEIGHT_SETTER_TESTS_H
