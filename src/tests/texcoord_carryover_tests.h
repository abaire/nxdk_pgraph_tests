#ifndef NXDK_PGRAPH_TESTS_TEXCOORD_CARRYOVER_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXCOORD_CARRYOVER_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

class TestHost;

/**
 * Tests the behavior when a texture stage consumes a texture coordinate that the vertex shader never wrote.
 *
 * This test investigates behavior observed in "Driv3r" https://github.com/xemu-project/xemu/issues/1614
 * The game draws with texture stage 3 in cube map mode, but its vertex shader never writes oT3.
 *
 * These tests check whether an unwritten texture coordinate output keeps a stale value from previously processed
 * vertices (as is the case for the fog coordinate, see FogCarryoverTests) or uses a fixed default. The behavior of a
 * cube map lookup with a zero length direction is covered separately by TextureCubemapTests::CubemapZeroVector.
 *
 * The "Stage<N>*" tests render the texture coordinate directly as a color (texture shader mode PASS_THROUGH, with the
 * combiner outputting TEX<N>), so any carried value is visible without any texture lookup. Each row uses a different
 * submission mode: IB (inline buffer), IE (inline elements), IA (inline array), DA (draw arrays). Quads are used
 * except by PerVertex, which uses points so that no interpolation occurs. <N> is 3 (the Driv3r case) or 0 (to see
 * whether oT0 behaves like oT3).
 *
 * Observations from HW:
 * - Unwritten texture coordinate outputs do not carry over. In every test, every quad and point drawn with a shader
 *   that does not write oT0 or oT3 renders black, regardless of the values written by the preceding draws. Not
 *   writing a texture coordinate from the vertex shader results in (0, 0, 0) for that coordinate.
 */
class TexcoordCarryoverTests : public TestSuite {
 public:
  TexcoordCarryoverTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;
  void TearDownTest() override;

 private:
  void ConfigureStageOutput(uint32_t stage);
  void TestConstant(const std::string& name, uint32_t stage);
  void TestReprime(const std::string& name, uint32_t stage);
  void TestPerVertex(const std::string& name, uint32_t stage);

  void ActivateShader(const std::shared_ptr<PBKitPlusPlus::VertexShaderProgram>& shader);

 private:
  std::shared_ptr<PBKitPlusPlus::VertexShaderProgram> prime_const_shader_;
  std::shared_ptr<PBKitPlusPlus::VertexShaderProgram> prime_attr_shader_;
  std::shared_ptr<PBKitPlusPlus::VertexShaderProgram> position_and_color_shader_;
};

#endif  // NXDK_PGRAPH_TESTS_TEXCOORD_CARRYOVER_TESTS_H
