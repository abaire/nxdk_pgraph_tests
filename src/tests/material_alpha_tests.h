#ifndef NXDK_PGRAPH_TESTS_MATERIAL_TESTS_H
#define NXDK_PGRAPH_TESTS_MATERIAL_TESTS_H

#include "test_suite.h"

class TestHost;

/**
 * Tests material alpha source and modulation settings in the fixed-function lighting pipeline.
 */
class MaterialAlphaTests : public TestSuite {
 public:
  MaterialAlphaTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();

  //! Tests material alpha calculations with the specified diffuse color source and material alpha value.
  void Test(uint32_t diffuse_source, float material_alpha);

  static std::string MakeTestName(uint32_t diffuse_source, float material_alpha);
};

#endif  // NXDK_PGRAPH_TESTS_MATERIAL_TESTS_H
