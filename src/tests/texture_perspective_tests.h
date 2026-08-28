#ifndef NXDK_PGRAPH_TESTS_TEXTURE_PERSPECTIVE_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_PERSPECTIVE_TESTS_H

#include "test_suite.h"

class TestHost;
namespace PBKitPlusPlus {
class VertexBuffer;
}

/**
 * Tests perspective-correct interpolation of texture coordinates versus vertex diffuse colors.
 */
class TexturePerspectiveTests : public TestSuite {
 public:
  TexturePerspectiveTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  //! Tests texture coordinate interpolation with perspective correction enabled or disabled on triangles and quads.
  void TestTexturePerspective(bool draw_quad, bool perspective_corrected);

  //! Tests diffuse color interpolation with perspective correction enabled or disabled on triangles and quads.
  void TestDiffusePerspective(bool draw_quad, bool perspective_corrected);
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_PERSPECTIVE_TESTS_H
