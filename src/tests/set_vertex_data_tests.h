#ifndef NXDK_PGRAPH_TESTS_SET_VERTEX_DATA_TESTS_H
#define NXDK_PGRAPH_TESTS_SET_VERTEX_DATA_TESTS_H

#include <pbkit/nv_regs.h>

#include <memory>
#include <vector>

#include "test_suite.h"

struct Color;
class TestHost;
class VertexBuffer;

// Tests behavior of various SET_VERTEX_DATAX methods.
class SetVertexDataTests : public TestSuite {
 public:
  enum SetFunction {
    FUNC_2F_M = NV097_SET_VERTEX_DATA2F_M,
    FUNC_4F_M = NV097_SET_VERTEX_DATA4F_M,
    FUNC_2S = NV097_SET_VERTEX_DATA2S,
    FUNC_4UB = NV097_SET_VERTEX_DATA4UB,
    FUNC_4S_M = NV097_SET_VERTEX_DATA4S_M,
  };

 public:
  SetVertexDataTests(TestHost& host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test(SetFunction func, const Color& diffuse, bool saturate_signed);

  static std::string MakeTestName(SetFunction func, bool saturate_signed);

  std::shared_ptr<VertexBuffer> diffuse_buffer_;
  std::shared_ptr<VertexBuffer> lit_buffer_;
  std::shared_ptr<VertexBuffer> lit_buffer_negative_;
};

#endif  // NXDK_PGRAPH_TESTS_SET_VERTEX_DATA_TESTS_H
