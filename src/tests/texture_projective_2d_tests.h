#ifndef NXDK_PGRAPH_TESTS_TEXTURE_PROJECTIVE_2D_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_PROJECTIVE_2D_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TexCoord4 {
  float s, t, r, q;
};

/**
 * Tests 2D projective texturing (STAGE_2D_PROJECTIVE / PS_TEXTUREMODES_PROJECTIVE2D)
 * with explicit Q coordinate interpolation across quads and triangles.
 */
class TextureProjective2DTests : public TestSuite {
 public:
  struct TestConfig {
    const char *test_name;
    const char *description;
    TexCoord4 corners[4];
    bool draw_quad{true};
    const char *notes_line1{nullptr};
  };

  TextureProjective2DTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void TestExplicitQ(const TestConfig &config);
  void TestUniformSigns();

  void GenerateTestTexture();

 private:
  static constexpr uint32_t kTextureSize = 128;
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_PROJECTIVE_2D_TESTS_H
