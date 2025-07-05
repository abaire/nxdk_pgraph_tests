#ifndef NXDK_PGRAPH_TESTS_TEXTURE_MATRIX_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_MATRIX_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

class TextureMatrixTests : public TestSuite {
 public:
  TextureMatrixTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test(const char *test_name, const matrix4_t &matrix);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_MATRIX_TESTS_H
