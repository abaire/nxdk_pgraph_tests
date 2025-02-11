#ifndef NXDK_PGRAPH_TESTS_COLOR_KEY_TESTS_H
#define NXDK_PGRAPH_TESTS_COLOR_KEY_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

class TestHost;
class VertexBuffer;

/**
 * Tests NV097_SET_COLOR_KEY_COLOR (0x0AE0) and the
 * NV097_SET_TEXTURE_CONTROL0_COLOR_KEY_MODE color key functions.
 *
 * NV097_SET_TEXTURE_CONTROL0_COLOR_KEY_MODE is used to instruct the hardware
 * what to do when a texel's value matches a color key. See ColorKeyMode for the
 * supported modes.
 *
 * NV097_SET_COLOR_KEY_COLOR is used to instruct the hardware which texels
 * should have the color key mode applied.
 *
 * Each test renders a series of quads, with each quad being divided into four
 * subcomponents (NW = northwest/top left, etc...):
 *
 * NW: Checkerboard alternating a color-keyed color and a light grey.
 * NE: Checkerboard alternating a dark grey and an alpha-keyed color.
 * SW: Checkerboard alternating the color-keyed color with alpha from the alpha
 *     key and the color keyed color with 0xFF alpha that matches none of the keys.
 * SE: Checkerboard alternating a non-keyed color with alpha from the color key
 *     and a non-keyed color with alpha from the alpha key.
 *
 */
class ColorKeyTests : public TestSuite {
 public:
  ColorKeyTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void TearDownTest() override;

 private:
  //! Tests color key behavior using the fixed function rendering pipeline.
  void TestFixedFunction(const std::string& name, uint32_t mode, bool alpha_from_texture);

  //! Tests color key behavior using a custom shader.
  void Test(const std::string& name, uint32_t mode, bool alpha_from_texture);

  //! Demonstrate the fact that unsampled texels will kill pixels entirely.
  void TestUnsampled(const std::string& name);

  void SetupTextureStage(uint32_t stage, uint32_t mode) const;
  void SetupAllTextureStages(uint32_t mode) const;
  void DisableAllTextureStages() const;
};

#endif  // NXDK_PGRAPH_TESTS_COLOR_KEY_TESTS_H
