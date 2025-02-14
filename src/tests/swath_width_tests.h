#ifndef SWATHWIDTHTESTS_H
#define SWATHWIDTHTESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests the behavior of NV097_SET_SWATH_WIDTH (0x09f8).
 */
class SwathWidthTests : public TestSuite {
 public:
  SwathWidthTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  void Test(const std::string &name, uint32_t alpha_func);
};

#endif  // SWATHWIDTHTESTS_H
