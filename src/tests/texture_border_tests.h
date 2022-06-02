#ifndef NXDK_PGRAPH_TESTS_TEXTURE_BORDER_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_BORDER_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;

class TextureBorderTests : public TestSuite {
 public:
  TextureBorderTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();

  void Test2D();
  void Test2DBorderedSwizzled();
  void Test2DPalettized();

 private:
  std::shared_ptr<VertexBuffer> vertex_buffers_[6];
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_BORDER_TESTS_H
