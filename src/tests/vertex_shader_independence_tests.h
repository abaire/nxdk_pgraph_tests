#ifndef NXDK_PGRAPH_TESTS_VERTEX_SHADER_INDEPENDENCE_TESTS_H
#define NXDK_PGRAPH_TESTS_VERTEX_SHADER_INDEPENDENCE_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class VertexShaderIndependenceTests : public TestSuite {
 public:
  VertexShaderIndependenceTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();

  void Test();
};

#endif  // NXDK_PGRAPH_TESTS_VERTEX_SHADER_INDEPENDENCE_TESTS_H
