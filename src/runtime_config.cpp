#include "runtime_config.h"

#include <tiny-json/tiny-json.h>

#include "debug_output.h"

#define MAX_CONFIG_FILE_SIZE (1024 * 128)

static bool parse_test_suites(json_t const* test_suites, std::vector<std::string>& errors,
                              std::set<std::string>& skipped_test_suites,
                              std::map<std::string, std::set<std::string>>& skipped_test_cases);

//! C++ wrapper around Tiny-JSON jsonPool_t
class JSONParser : jsonPool_t {
 public:
  JSONParser() : jsonPool_t{&Alloc, &Alloc} {}
  explicit JSONParser(const char* str) : jsonPool_t{&Alloc, &Alloc}, json_string_{str} {
    root_node_ = json_createWithPool(json_string_.data(), this);
  }

  JSONParser(const JSONParser&) = delete;
  JSONParser(JSONParser&&) = delete;
  JSONParser& operator=(const JSONParser&) = delete;
  JSONParser& operator=(JSONParser&&) = delete;

  [[nodiscard]] json_t const* root() const { return root_node_; }

 private:
  static json_t* Alloc(jsonPool_t* pool) {
    const auto list_pool = (JSONParser*)pool;
    list_pool->object_list_.emplace_back();
    return &list_pool->object_list_.back();
  }

  std::list<json_t> object_list_{};
  std::string json_string_{};
  json_t const* root_node_{};
};

bool RuntimeConfig::LoadConfig(const char* config_file_path, std::vector<std::string>& errors) {
  std::string dos_style_path = config_file_path;
  std::replace(dos_style_path.begin(), dos_style_path.end(), '/', '\\');
  std::map<std::string, std::vector<std::string>> test_config;

  std::string json_string;

  {
    std::ifstream config_file(dos_style_path.c_str());
    if (!config_file) {
      errors.push_back(std::string("Missing config file at ") + config_file_path);
      return false;
    }

    config_file.seekg(0, std::ios::end);
    auto end_pos = config_file.tellg();
    config_file.seekg(0, std::ios::beg);
    std::streamsize file_size = end_pos - config_file.tellg();

    if (file_size > MAX_CONFIG_FILE_SIZE || file_size == -1) {
      errors.push_back(std::string("Config file at ") + config_file_path + " is too large.");
      return false;
    }

    json_string.resize(file_size);
    config_file.read(&json_string[0], file_size);
  }

  JSONParser parser{json_string.c_str()};

  auto root = parser.root();
  if (!root) {
    errors.emplace_back("Failed to parse config file.");
    return false;
  }

  auto settings = json_getProperty(root, "settings");
  if (!settings) {
    errors.emplace_back("'settings' not found");
    return false;
  }
  if (json_getType(settings) != JSON_OBJ) {
    errors.emplace_back("'settings' not an object");
    return false;
  }

  // generic lambda, operator() is a template with one parameter
  auto get_bool = [](auto object, const char* key, bool& out) {
    auto property = json_getProperty(object, key);
    if (!property) {
      return true;
    }

    if (json_getType(property) != JSON_BOOLEAN) {
      return false;
    }

    out = json_getBoolean(property);
    return true;
  };

  if (!get_bool(settings, "enable_progress_log", enable_progress_log_)) {
    errors.emplace_back("settings[enable_progress_log] must be a boolean");
    return false;
  }

  if (!get_bool(settings, "disable_autorun", disable_autorun_)) {
    errors.emplace_back("settings[disable_autorun] must be a boolean");
    return false;
  }

  if (!get_bool(settings, "enable_autorun_immediately", enable_autorun_immediately_)) {
    errors.emplace_back("settings[enable_autorun_immediately] must be a boolean");
    return false;
  }

  if (!get_bool(settings, "enable_shutdown_on_completion", enable_shutdown_on_completion_)) {
    errors.emplace_back("settings[enable_shutdown_on_completion] must be a boolean");
    return false;
  }

  if (!get_bool(settings, "enable_pgraph_region_diff", enable_pgraph_region_diff_)) {
    errors.emplace_back("settings[enable_pgraph_region_diff] must be a boolean");
    return false;
  }

  auto output_directory_path = json_getProperty(settings, "output_directory_path");
  if (output_directory_path) {
    if (json_getType(output_directory_path) != JSON_TEXT) {
      errors.emplace_back("settings[output_directory_path] must be a string");
      return false;
    }

    output_directory_path_ = SanitizePath(json_getValue(output_directory_path));
  }

  auto test_suites = json_getProperty(root, "test_suites");
  if (!test_suites) {
    return true;
  }

  if (json_getType(test_suites) != JSON_OBJ) {
    errors.emplace_back("'test_suites' not an object");
    return false;
  }

  return parse_test_suites(test_suites, errors, skipped_test_suites_, skipped_test_cases_);
}

static bool parse_test_case(json_t const* test_case, const std::string& suite_name, const std::string& test_name,
                            std::vector<std::string>& errors,
                            std::map<std::string, std::set<std::string>>& skipped_test_cases,
                            const std::string& test_case_error_message_prefix) {
  auto skipped = json_getProperty(test_case, "skipped");
  if (!skipped) {
    return true;
  }

  if (json_getType(skipped) != JSON_BOOLEAN) {
    errors.emplace_back(test_case_error_message_prefix + "[skipped] must be a boolean");
    return false;
  }

  if (!json_getBoolean(skipped)) {
    return true;
  }

  auto existing = skipped_test_cases.find(test_name);
  if (existing == skipped_test_cases.end()) {
    skipped_test_cases[suite_name] = {test_name};
  } else {
    skipped_test_cases[suite_name].insert(test_name);
  }

  return true;
}

static bool parse_test_cases(json_t const* test_suite, const std::string& suite_name, std::vector<std::string>& errors,
                             std::set<std::string>& skipped_test_suites,
                             std::map<std::string, std::set<std::string>>& skipped_test_cases,
                             const std::string& suite_error_message_prefix) {
  for (auto test_or_skipped = json_getChild(test_suite); test_or_skipped;
       test_or_skipped = json_getSibling(test_or_skipped)) {
    std::string test_name = json_getName(test_or_skipped);
    auto type = json_getType(test_or_skipped);

    if (type == JSON_BOOLEAN) {
      if (test_name != "skipped") {
        errors.emplace_back(suite_error_message_prefix + "[" + test_name + "] must be an object");
        return false;
      }

      if (json_getBoolean(test_or_skipped)) {
        skipped_test_suites.insert(suite_name);
      }
    } else if (type == JSON_OBJ) {
      if (!parse_test_case(test_or_skipped, suite_name, test_name, errors, skipped_test_cases,
                           suite_error_message_prefix + "[" + test_name + "]")) {
        return false;
      }
    } else {
      errors.emplace_back(suite_error_message_prefix + "[" + test_name + "] must be an object");
      return false;
    }
  }

  return true;
}

static bool parse_test_suites(json_t const* test_suites, std::vector<std::string>& errors,
                              std::set<std::string>& skipped_test_suites,
                              std::map<std::string, std::set<std::string>>& skipped_test_cases) {
  std::string test_suites_error_message_prefix("test_suites[");
  for (auto suite = json_getChild(test_suites); suite; suite = json_getSibling(suite)) {
    std::string suite_name = json_getName(suite);
    auto suite_error_message_prefix = test_suites_error_message_prefix + suite_name + "]";

    if (json_getType(suite) != JSON_OBJ) {
      errors.emplace_back(suite_error_message_prefix + " must be an object");
    }

    if (!parse_test_cases(suite, suite_name, errors, skipped_test_suites, skipped_test_cases,
                          suite_error_message_prefix)) {
      return false;
    }
  }

  return true;
}

bool RuntimeConfig::ApplyConfig(std::vector<std::shared_ptr<TestSuite>>& test_suites,
                                std::vector<std::string>& errors) {
  if (skipped_test_suites_.empty() && skipped_test_cases_.empty()) {
    return true;
  }

  if (!skipped_test_suites_.empty()) {
    std::vector<std::shared_ptr<TestSuite>> filtered_tests;
    for (auto& suite : test_suites) {
      if (skipped_test_suites_.find(suite->Name()) == skipped_test_suites_.end()) {
        filtered_tests.emplace_back(suite);
      }
    }

    test_suites = filtered_tests;
  }

  if (!skipped_test_cases_.empty()) {
    for (auto& suite : test_suites) {
      auto skipped_test_cases = skipped_test_cases_.find(suite->Name());
      if (skipped_test_cases == skipped_test_cases_.end()) {
        continue;
      }

      suite->DisableTests(skipped_test_cases->second);
    }
  }

  return true;
}

#ifdef DUMP_CONFIG_FILE
static std::string bool_str(bool value) { return value ? "true" : "false"; }

bool RuntimeConfig::DumpConfig(const std::string& config_file_path,
                               const std::vector<std::shared_ptr<TestSuite>>& test_suites,
                               std::vector<std::string>& errors) {
  const char* out_path = config_file_path.c_str();
  PrintMsg("Writing config file to %s\n", out_path);

  std::ofstream config_file(config_file_path);
  if (!config_file) {
    errors.emplace_back("Failed to open config file for output");
    return false;
  }

  config_file << "{" << std::endl;

  // General settings.

  write_settings(config_file);
  config_file << "," << std::endl;

  // Test suite enable/disable status.

  config_file << "  \"test_suites\": {" << std::endl;

  bool add_test_suite_comma = false;
  for (auto& suite : test_suites) {
    if (add_test_suite_comma) {
      config_file << "," << std::endl;
    }

    write_test_suite(config_file, suite);
    add_test_suite_comma = true;
  }

  config_file << std::endl;
  config_file << "  }" << std::endl;  // test_suites

  config_file << "}" << std::endl;

  return true;
}

void RuntimeConfig::write_settings(std::ostream& output) const {
  output << "  \"settings\": {" << std::endl;

#define BOOL_OPT(VALUE) output << "    \"" #VALUE "\": " << bool_str(VALUE())

  BOOL_OPT(enable_progress_log) << "," << std::endl;
  BOOL_OPT(disable_autorun) << "," << std::endl;
  BOOL_OPT(enable_autorun_immediately) << "," << std::endl;
  BOOL_OPT(enable_shutdown_on_completion) << "," << std::endl;
  BOOL_OPT(enable_pgraph_region_diff) << "," << std::endl;

#undef BOOL_OPT

  output << "    \"output_directory_path\": \"" << EscapePath(output_directory_path()) << "\"" << std::endl;

  output << "  }";
}

void RuntimeConfig::write_test_suite(std::ostream& output, const std::shared_ptr<TestSuite>& suite) const {
  output << "    \"" << suite->Name() << "\": {" << std::endl;
  bool add_test_comma = false;
  if (skipped_test_suites_.find(suite->Name()) != skipped_test_suites_.end()) {
    output << "      \"skipped\": true";
    add_test_comma = true;
  }

  for (auto& test_name : suite->TestNames()) {
    if (add_test_comma) {
      output << "," << std::endl;
    }

    write_test_case(output, test_name);

    add_test_comma = true;
  }
  output << std::endl;

  output << "    }";
}

void RuntimeConfig::write_test_case(std::ostream& output, const std::string& test_name) const {
  output << "      \"" << test_name << "\": {" << std::endl;
  if (skipped_test_cases_.find(test_name) != skipped_test_cases_.end()) {
    output << "        \"skipped\": true" << std::endl;
  }
  output << "      }";
}

std::string RuntimeConfig::EscapePath(const std::string& path) {
  std::string escaped(path);
  std::replace(escaped.begin(), escaped.end(), '\\', '/');
  return std::move(escaped);
}

#endif  // DUMP_CONFIG_FILE

std::string RuntimeConfig::SanitizePath(const std::string& path) {
  std::string sanitized(path);
  std::replace(sanitized.begin(), sanitized.end(), '/', '\\');
  return std::move(sanitized);
}
