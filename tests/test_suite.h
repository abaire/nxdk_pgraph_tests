#ifndef NXDK_PGRAPH_TESTS_TEST_SUITE_H
#define NXDK_PGRAPH_TESTS_TEST_SUITE_H

#include <map>
#include <string>
#include <vector>

class TestHost;

class TestSuite {
 public:
  TestSuite(TestHost &host, std::string output_dir);

  virtual std::string Name() = 0;

  virtual void Initialize() = 0;
  virtual void Deinitialize() {}

  std::vector<std::string> TestNames() const;
  void Run(const std::string &test_name);

  void RunAll();

 protected:
  TestHost &host_;
  std::string output_dir_;

  std::map<std::string, std::function<void()>> tests_{};
};

#endif  // NXDK_PGRAPH_TESTS_TEST_SUITE_H
