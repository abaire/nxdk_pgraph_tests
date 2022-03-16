#ifndef NXDK_PGRAPH_TESTS_TEXGEN_MATRIX_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXGEN_MATRIX_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class TexgenMatrixTests : public TestSuite {
 public:
  TexgenMatrixTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();

  void Test(TextureStage::TexGen gen_mode);

  static std::string MakeTestName(TextureStage::TexGen mode);
};

#endif  // NXDK_PGRAPH_TESTS_TEXGEN_MATRIX_TESTS_H
