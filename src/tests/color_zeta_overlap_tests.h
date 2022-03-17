#ifndef NXDK_PGRAPH_TESTS_COLOR_ZETA_OVERLAP_TESTS_H
#define NXDK_PGRAPH_TESTS_COLOR_ZETA_OVERLAP_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class ColorZetaOverlapTests : public TestSuite {
 public:
  ColorZetaOverlapTests(TestHost &host, std::string output_dir);

  void Initialize() override;

 private:
  void TestColorIntoDepth();
  void TestDepthIntoColor();
  void TestSwap();
};

#endif  // NXDK_PGRAPH_TESTS_COLOR_ZETA_OVERLAP_TESTS_H
