#ifndef NXDK_PGRAPH_TESTS_TEXTURE_DEPTH_SOURCE_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_DEPTH_SOURCE_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class TextureDepthSourceTests : public TestSuite {
 public:
  TextureDepthSourceTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();
  void TestD16();

 private:
  struct s_CtxDma texture_target_ctx_ {};
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_DEPTH_SOURCE_TESTS_H
