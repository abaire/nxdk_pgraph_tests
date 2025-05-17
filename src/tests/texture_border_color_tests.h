#ifndef NXDK_PGRAPH_TESTS_TEXTURE_BORDER_COLOR_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_BORDER_COLOR_TESTS_H

#include <memory>

#include "test_host.h"
#include "test_suite.h"

class TestHost;

//! Tests behavior of texture border colors with various texture formats
class TextureBorderColorTests : public TestSuite {
 public:
  TextureBorderColorTests(TestHost& host, std::string output_dir, const Config& config);
  void Initialize() override;

 private:
  void Test();
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_BORDER_COLOR_TESTS_H
