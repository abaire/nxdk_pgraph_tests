#ifndef NXDK_PGRAPH_TESTS_TEXTURE_PERSPECTIVE_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_PERSPECTIVE_TESTS_H

#include "test_suite.h"

class TestHost;
namespace PBKitPlusPlus {
class VertexBuffer;
}

class TexturePerspectiveTests : public TestSuite {
 public:
  TexturePerspectiveTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void TestTexturePerspective(bool draw_quad, bool perspective_corrected);
  void TestDiffusePerspective(bool draw_quad, bool perspective_corrected);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_PERSPECTIVE_TESTS_H
