#ifndef NXDK_PGRAPH_TESTS_TEST_SUITE_H
#define NXDK_PGRAPH_TESTS_TEST_SUITE_H

#include <map>
#include <string>
#include <vector>

class TestHost;

class TestSuite {
 public:
  TestSuite(TestHost &host, std::string output_dir, std::string test_name);

  const std::string &Name() const { return test_name_; };

  virtual void Initialize();
  virtual void Deinitialize() {}

  std::vector<std::string> TestNames() const;
  void Run(const std::string &test_name);

  void RunAll();

 protected:
  void SetDefaultTextureFormat() const;

 protected:
  TestHost &host_;
  std::string output_dir_;
  std::string test_name_;
  std::map<std::string, std::function<void()>> tests_{};
};

#endif  // NXDK_PGRAPH_TESTS_TEST_SUITE_H
