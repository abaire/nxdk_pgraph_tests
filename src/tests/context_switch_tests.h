#ifndef NXDK_PGRAPH_TESTS_CONTEXT_SWITCH_TESTS_H
#define NXDK_PGRAPH_TESTS_CONTEXT_SWITCH_TESTS_H

#include <string>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests behavior when modifying the NV097_PGRAPH_CTX_SWITCHx registers.
 */
class ContextSwitchTests : public TestSuite {
 public:
  ContextSwitchTests(TestHost &host, std::string output_dir, const Config &config);

  void Initialize() override;

 private:
  //! Tests PGRAPH graphics channel context switching and state restoration across pushbuffer channels.
  void Test();
};

#endif  // NXDK_PGRAPH_TESTS_CONTEXT_SWITCH_TESTS_H
