#ifndef NXDK_PGRAPH_TESTS_TEST_BASE_H
#define NXDK_PGRAPH_TESTS_TEST_BASE_H

#include <string>

class TestHost;

class TestBase {
 public:
  TestBase(TestHost &host, std::string output_dir);
  virtual void Run() = 0;

 protected:
  TestHost &host_;
  std::string output_dir_;
};

#endif  // NXDK_PGRAPH_TESTS_TEST_BASE_H
