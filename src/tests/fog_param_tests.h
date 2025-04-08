#ifndef NXDK_PGRAPH_TESTS_FOG_PARAM_TESTS_H
#define NXDK_PGRAPH_TESTS_FOG_PARAM_TESTS_H

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;

/**
 * Tests the effects of NV097_SET_FOG_PARAMS.
 *
 * Note that the 3rd parameter to SET_FOG_PARAMS always appears to be set to 0.0 in practice and alternative values do
 * not appear to have any effect. Non-zero values are omitted from these tests for brevity.
 */
class FogParamTests : public TestSuite {
 public:
  FogParamTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void Test(const std::string& name, uint32_t fog_mode, float bias, float linear);

  //! Attempts to find the bias value at which the given fog_mode results in 0 fog.
  void TestBiasZeroPoint(const std::string& name, uint32_t fog_mode);
};

#endif  // NXDK_PGRAPH_TESTS_FOG_PARAM_TESTS_H
