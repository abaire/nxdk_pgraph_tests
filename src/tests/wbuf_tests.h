#ifndef NXDK_PGRAPH_TESTS_WBUF_TESTS_H
#define NXDK_PGRAPH_TESTS_WBUF_TESTS_H

#include "test_suite.h"

class TestHost;

class WBufTests : public TestSuite {
 public:
  WBufTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;
  void Deinitialize() override;

 private:
  void Test(bool zbias, bool vsh);
};

#endif  // NXDK_PGRAPH_TESTS_WBUF_TESTS_H
