#ifndef NXDK_PGRAPH_TESTS_TEXTURE_MATRIX_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_MATRIX_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class TextureMatrixTests : public TestSuite {
 public:
  TextureMatrixTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test(const char *test_name, MATRIX matrix);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_MATRIX_TESTS_H
