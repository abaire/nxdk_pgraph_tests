#ifndef NXDK_PGRAPH_TESTS_VERTEX_SHADER_SWIZZLE_TESTS_H
#define NXDK_PGRAPH_TESTS_VERTEX_SHADER_SWIZZLE_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests vertex shader instruction source swizzling and destination writemask combinations.
 */
class VertexShaderSwizzleTests : public TestSuite {
 public:
  struct Instruction {
    const char *name;
    const char *mask;
    const char *swizzle;
    const uint32_t instruction[4];
  };

 public:
  VertexShaderSwizzleTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  //! Tests executing vertex shader programs with various component swizzle and writemask permutations.
  void Test(const std::string &name, const Instruction *instructions, uint32_t count, bool full_opacity = false);

  void DrawCheckerboardBackground() const;

 private:
  uint32_t *shader_code_{nullptr};
  uint32_t shader_code_size_{0};
};

#endif  // NXDK_PGRAPH_TESTS_VERTEX_SHADER_SWIZZLE_TESTS_H
