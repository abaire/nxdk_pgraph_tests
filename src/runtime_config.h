#ifndef NXDK_PGRAPH_TESTS_RUNTIME_CONFIG_H
#define NXDK_PGRAPH_TESTS_RUNTIME_CONFIG_H

#include <vector>

#include "configure.h"
#include "tests/test_suite.h"

class RuntimeConfig {
 public:
  enum SkipConfiguration {
    DEFAULT,
    SKIPPED,
    UNSKIPPED,
  };

 public:
  RuntimeConfig() = default;
  explicit RuntimeConfig(const RuntimeConfig&) = delete;

  /**
   * Loads the JSON config file from the given path.
   * @param config_file_path - The path to the file containing the JSON config.
   * @param errors - Vector of strings into which any error messages will be placed.
   * @return true on success, false on failure
   */
  bool LoadConfig(const char* config_file_path, std::vector<std::string>& errors);

  /**
   * Loads the JSON config file from the given string buffer.
   * @param config_content - String containing the JSON config.
   * @param errors - Vector of strings into which any error messages will be placed.
   * @return true on success, false on failure
   */
  bool LoadConfigBuffer(const std::string& config_content, std::vector<std::string>& errors);

  /**
   * Processes the JSON config file at the given path and adjusts the given set of test suites. Returns false if parsing
   * fails for any reason.
   */
  bool ApplyConfig(std::vector<std::shared_ptr<TestSuite>>& test_suites, std::vector<std::string>& errors);

#ifdef DUMP_CONFIG_FILE
  /**
   * Generates a default config file at the given path.
   */
  bool DumpConfig(const std::string& config_file_path, const std::vector<std::shared_ptr<TestSuite>>& test_suites,
                  std::vector<std::string>& errors);

  bool DumpConfigToStream(std::ostream& config_file, const std::vector<std::shared_ptr<TestSuite>>& test_suites,
                          std::vector<std::string>& errors);
#endif  // DUMP_CONFIG_FILE

  [[nodiscard]] bool enable_progress_log() const { return enable_progress_log_; }
  [[nodiscard]] bool disable_autorun() const { return disable_autorun_; }
  [[nodiscard]] bool enable_autorun_immediately() const { return enable_autorun_immediately_; }
  [[nodiscard]] bool enable_shutdown_on_completion() const { return enable_shutdown_on_completion_; }
  [[nodiscard]] bool enable_pgraph_region_diff() const { return enable_pgraph_region_diff_; }
  [[nodiscard]] bool skip_tests_by_default() const { return skip_tests_by_default_; }

  [[nodiscard]] const std::string& output_directory_path() const { return output_directory_path_; }

  static std::string SanitizePath(const std::string& path);

 private:
#ifdef DUMP_CONFIG_FILE
  static std::string EscapePath(const std::string& path);

  void write_settings(std::ostream& output) const;
  void write_test_suite(std::ostream& output, const std::shared_ptr<TestSuite>& suite) const;
  void write_test_case(std::ostream& output, const std::string& suite_name, const std::string& test_name) const;
#endif  // DUMP_CONFIG_FILE

 private:
  bool enable_progress_log_ = DEFAULT_ENABLE_PROGRESS_LOG;
  bool disable_autorun_ = DEFAULT_DISABLE_AUTORUN;
  bool enable_autorun_immediately_ = DEFAULT_AUTORUN_IMMEDIATELY;
  bool enable_shutdown_on_completion_ = DEFAULT_ENABLE_SHUTDOWN;
  bool enable_pgraph_region_diff_ = DEFAULT_ENABLE_PGRAPH_REGION_DIFF;
  bool skip_tests_by_default_ = DEFAULT_SKIP_TESTS_BY_DEFAULT;

  std::string output_directory_path_ = RuntimeConfig::SanitizePath(DEFAULT_OUTPUT_DIRECTORY_PATH);

  //! Map of test suite name to skip config.
  std::map<std::string, SkipConfiguration> configured_test_suites_;
  //! Map of test suite name to a map of test case to skip config.
  std::map<std::string, std::map<std::string, SkipConfiguration>> configured_test_cases_;
};

#endif  // NXDK_PGRAPH_TESTS_RUNTIME_CONFIG_H
