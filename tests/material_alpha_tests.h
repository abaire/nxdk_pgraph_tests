#ifndef NXDK_PGRAPH_TESTS_MATERIAL_TESTS_H
#define NXDK_PGRAPH_TESTS_MATERIAL_TESTS_H

#include "test_suite.h"

class TestHost;

class MaterialAlphaTests : public TestSuite {
 public:
  MaterialAlphaTests(TestHost &host, std::string output_dir);

  std::string Name() override { return "Material"; }
  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();
  void Test(uint32_t diffuse_source, float material_alpha);

  static std::string MakeTestName(uint32_t diffuse_source, float material_alpha);
};

#endif  // NXDK_PGRAPH_TESTS_MATERIAL_TESTS_H
