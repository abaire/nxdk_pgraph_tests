#ifndef NXDK_PGRAPH_TESTS_MATERIAL_TESTS_H
#define NXDK_PGRAPH_TESTS_MATERIAL_TESTS_H

#include "test_suite.h"

class TestHost;

class MaterialTests : public TestSuite {
 public:
  MaterialTests(TestHost &host, std::string output_dir);

  std::string Name() override { return "Material"; }
  void Initialize() override;
  void Deinitialize() override;

 private:
  void CreateGeometry();
  void Test();

  static std::string MakeTestName();
};

#endif  // NXDK_PGRAPH_TESTS_MATERIAL_TESTS_H
