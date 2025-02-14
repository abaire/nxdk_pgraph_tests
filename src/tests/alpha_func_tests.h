#ifndef ALPHAFUNCTESTS_H
#define ALPHAFUNCTESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests the behavior of NV097_SET_ALPHA_FUNC (0x0000033C).
 */
class AlphaFuncTests : public TestSuite {
 public:
  AlphaFuncTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void Test(const std::string &name, uint32_t alpha_func, bool enable);

  void Draw(float red, float green, float blue, float left_alpha, float right_alpha, float top, float bottom) const;
};

#endif  // ALPHAFUNCTESTS_H
