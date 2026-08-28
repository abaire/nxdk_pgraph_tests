#ifndef NXDK_PGRAPH_TESTS_TEXTURE_MATRIX_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_MATRIX_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests texture transformation matrix operations (NV097_SET_TEXTURE_MATRIX).
 */
class TextureMatrixTests : public TestSuite {
 public:
  TextureMatrixTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void CreateGeometry();

  //! Tests texture coordinate transformations with the given 4x4 matrix.
  void Test(const char *test_name, const matrix4_t &matrix);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_MATRIX_TESTS_H
