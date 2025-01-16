#ifndef NXDK_PGRAPH_TESTS_COLOR_ZETA_DISABLE_TESTS_H
#define NXDK_PGRAPH_TESTS_COLOR_ZETA_DISABLE_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

class ColorZetaDisableTests : public TestSuite {
 public:
  struct Instruction {
    const char *name;
    const char *mask;
    const char *swizzle;
    const uint32_t instruction[4];
  };

 public:
  ColorZetaDisableTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void Test();

  void DrawCheckerboardBackground() const;
};

#endif  // NXDK_PGRAPH_TESTS_COLOR_ZETA_DISABLE_TESTS_H
