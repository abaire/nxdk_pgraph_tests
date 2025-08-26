#ifndef NXDK_PGRAPH_TESTS_DEGENERATE_BEGIN_END_TESTS_H
#define NXDK_PGRAPH_TESTS_DEGENERATE_BEGIN_END_TESTS_H

#include <memory>
#include <vector>

#include "test_host.h"
#include "test_suite.h"

/**
 * Tests atypical situations involving NV097_SET_BEGIN_END.
 */
class DegenerateBeginEndTests : public TestSuite {
 public:
  DegenerateBeginEndTests(TestHost& host, std::string output_dir, const Config& config);

  void Initialize() override;

 private:
  void TestBeginWithoutEnd();
};

#endif  // NXDK_PGRAPH_TESTS_DEGENERATE_BEGIN_END_TESTS_H
