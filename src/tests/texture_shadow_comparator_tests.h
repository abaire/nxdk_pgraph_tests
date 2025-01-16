#ifndef NXDK_PGRAPH_TESTS_TEXTURE_SHADOW_COMPARATOR_TESTS_H
#define NXDK_PGRAPH_TESTS_TEXTURE_SHADOW_COMPARATOR_TESTS_H

#include <functional>
#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

class PrecalculatedVertexShader;
class PerspectiveVertexShader;
struct TextureFormatInfo;
class VertexBuffer;

class TextureShadowComparatorTests : public TestSuite {
 public:
  TextureShadowComparatorTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void TestRawValues(uint32_t depth_format, uint32_t texture_format, TestHost::ShaderStageProgram mode,
                     uint32_t shadow_comp_function, uint32_t min_val, uint32_t max_val, uint32_t ref,
                     const std::string &name);
  void TestFixedFunction(uint32_t depth_format, bool float_depth, uint32_t texture_format,
                         TestHost::ShaderStageProgram mode, uint32_t shadow_comp_function, float min_val, float max_val,
                         float ref_val, const std::string &name);
  void TestProgrammable(uint32_t depth_format, bool float_depth, uint32_t texture_format,
                        TestHost::ShaderStageProgram mode, uint32_t shadow_comp_function, float min_val, float max_val,
                        float ref_val, const std::string &name);

  void TestProjected(uint32_t depth_format, uint32_t texture_format, TestHost::ShaderStageProgram mode,
                     uint32_t shadow_comp_function, float min_val, float max_val, float ref_val,
                     std::function<void(vector_t &, const vector_t &)> project_point,
                     std::function<void(vector_t &, const vector_t &, float)> unproject_point, const std::string &name);

 private:
  struct s_CtxDma texture_target_ctx_{};
  std::shared_ptr<PrecalculatedVertexShader> raw_value_shader_;
};

#endif  // NXDK_PGRAPH_TESTS_TEXTURE_SHADOW_COMPARATOR_TESTS_H
