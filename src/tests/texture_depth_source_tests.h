#ifndef NXDK_PGRAPH_TESTS_TEXTURE_SHADOW_COMPARATOR_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_SHADOW_COMPARATOR_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class TextureShadowComparatorTests : public TestSuite {
 public:
  TextureShadowComparatorTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();
  void TestD16();

 private:
  struct s_CtxDma texture_target_ctx_ {};
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_SHADOW_COMPARATOR_TESTS_H
