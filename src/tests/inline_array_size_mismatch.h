#ifndef NXDK_PGRAPH_TESTS_INLINE_ARRAY_SIZE_MISMATCH_TESTS_H
#define NXDK_PGRAPH_TESTS_INLINE_ARRAY_SIZE_MISMATCH_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

struct TextureFormatInfo;
class VertexBuffer;

//! Verifies that using an NV097_INLINE_ARRAY draw loop that ends with a partial vertex description silently ignores
//! any partial vertices.
//!
//! See xemu#985
class InlineArraySizeMismatchTests : public TestSuite {
 public:
  InlineArraySizeMismatchTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void Test();
};

#endif  // NXDK_PGRAPH_TESTS_INLINE_ARRAY_SIZE_MISMATCH_TESTS_H
