#ifndef NXDK_PGRAPH_TESTS_TEXGEN_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXGEN_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

class TexgenTests : public TestSuite {
 public:
  TexgenTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void CreateGeometry();

  void Test(TextureStage::TexGen gen_mode);

  static std::string MakeTestName(TextureStage::TexGen mode);
};

#endif  // NXDK_PGRAPH_TESTS_TEXGEN_TESTS_H
