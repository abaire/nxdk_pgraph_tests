#ifndef NXDK_PGRAPH_TESTS_SPECULAR_TESTS_H
#define NXDK_PGRAPH_TESTS_SPECULAR_TESTS_H

#include "test_suite.h"

class TestHost;

/**
 * Tests handling of specular color with interesting combinations of lighting
 * settings.
 */
class SpecularTests : public TestSuite {
 public:
  SpecularTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  //! Tests handling of LIGHTING_ENABLE, SPECULAR_ENABLE, and SEPARATE_SPECULAR.
  void TestControlFlags(const std::string& name, bool use_fixed_function, bool enable_lighting);
};

#endif  // NXDK_PGRAPH_TESTS_SPECULAR_TESTS_H
