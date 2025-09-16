#ifndef NXDK_PGRAPH_TESTS_TEST_SUITE_H
#define NXDK_PGRAPH_TESTS_TEST_SUITE_H

#include <ftp_logger.h>

#include <chrono>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "pgraph_diff_token.h"

class TestHost;

/**
 * Base class for all test suites.
 */
class TestSuite {
 public:
  //! Runtime configuration for TestSuites.
  struct Config {
    //! Enable logging of test progress to file.
    bool enable_progress_log;

    // !Enables a diff of the nv2a PGRAPH registers before and after each test case.
    bool enable_pgraph_region_diff;

    //! Artificial delay before starting each test.
    uint32_t delay_milliseconds_between_tests;

    // Optional FTPLogger used to transfer test artifacts to a remote host.
    std::shared_ptr<FTPLogger> ftp_logger;
  };

 public:
  TestSuite() = delete;
  TestSuite(TestHost &host, std::string output_dir, std::string suite_name, const Config &config,
            bool interactive_only = false);
  virtual ~TestSuite() = default;

  [[nodiscard]] const std::string &Name() const { return suite_name_; };

  //! Called to initialize the test suite.
  virtual void Initialize();

  //! Called to tear down the test suite.
  virtual void Deinitialize();

  //! Called before running an individual test within this suite.
  virtual void SetupTest();

  //! Called after running an individual test within this suite.
  virtual void TearDownTest();

  void DisableTests(const std::set<std::string> &tests_to_skip);

  [[nodiscard]] std::vector<std::string> TestNames() const;
  [[nodiscard]] bool HasEnabledTests() const { return !tests_.empty(); };

  void Run(const std::string &test_name);

  void RunAll();

  bool IsInteractiveOnly() const { return interactive_only_; }
  void SetSavingAllowed(bool enable = true) { allow_saving_ = enable; }

  //! Inserts a pattern of NV097_NO_OPERATION's into the pushbuffer to allow identification when viewing nv2a traces.
  static void TagNV2ATrace(uint32_t num_nops);

 protected:
  void SetDefaultTextureFormat() const;

 private:
  std::chrono::steady_clock::time_point LogTestStart(const std::string &test_name);
  long LogTestEnd(const std::string &test_name, const std::chrono::steady_clock::time_point &start_time) const;

 protected:
  TestHost &host_;
  std::string output_dir_;
  std::string suite_name_;

  // Flag indicating that suite should be skipped if not directly invoked by the user.
  bool interactive_only_;

  // Flag to forcibly disallow saving of output (e.g., when in multiframe test mode for debugging).
  bool allow_saving_{true};

  // Map of `test_name` to `void test()`
  std::map<std::string, std::function<void()>> tests_{};

  PGRAPHDiffToken pgraph_diff_;

  bool enable_progress_log_;
  bool enable_pgraph_region_diff_;
  uint32_t delay_milliseconds_between_tests_;

  std::shared_ptr<FTPLogger> ftp_logger_;
};

#endif  // NXDK_PGRAPH_TESTS_TEST_SUITE_H
