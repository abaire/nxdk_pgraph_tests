#ifndef NXDK_PGRAPH_TESTS_TEXGEN_NORMAL_MAP_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXGEN_NORMAL_MAP_TESTS_H

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"
#include "xbox_math_types.h"

namespace PBKitPlusPlus {
class VertexBuffer;
}

/**
 * Tests texture coordinate generation using normal mapping NV097_SET_TEXGEN_S_V_NORMAL_MAP with non-flat geometry.
 */
class TexgenNormalMapTests : public TestSuite {
 public:
  TexgenNormalMapTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void Test(const std::string &test_name, const XboxMath::matrix4_t &matrix, bool matrix_enable,
            TestHost::ShaderStageProgram stage_program, uint32_t view_model);

  std::shared_ptr<PBKitPlusPlus::VertexBuffer> vertex_buffer_;
};

#endif  // NXDK_PGRAPH_TESTS_TEXGEN_NORMAL_MAP_TESTS_H
