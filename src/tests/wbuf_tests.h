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
  template <typename Func>
  void Test(int depthf, bool zbias, bool zslope, bool vsh, const char *prim_name, Func draw_prim,
            uint32_t clip_left = 150, uint32_t clip_top = 0);
};

#endif  // NXDK_PGRAPH_TESTS_WBUF_TESTS_H
