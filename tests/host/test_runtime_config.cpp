#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "configure.h"
#include "runtime_config.h"
#include "test_host.h"
#include "tests/test_suite.h"

using ::testing::ElementsAre;

static void PopulateConfig(RuntimeConfig& config, const std::string& json);

#ifdef DUMP_CONFIG_FILE

#include <sstream>

TEST(RuntimeConfig, DumpConfigBuffer_DefaultSettings) {
  RuntimeConfig config;
  std::vector<std::string> errors;
  std::stringstream output;
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };

  EXPECT_TRUE(config.DumpConfigToStream(output, suites, errors));
  EXPECT_TRUE(errors.empty());
  EXPECT_STREQ(output.str().c_str(), R"({
  "settings": {
    "enable_progress_log": false,
    "disable_autorun": false,
    "enable_autorun_immediately": false,
    "enable_shutdown_on_completion": false,
    "enable_pgraph_region_diff": false,
    "skip_tests_by_default": false,
    "output_directory_path": "e:/nxdk_pgraph_tests"
  },
  "test_suites": {
    "Suite_1": {
      "Test_1": {
      },
      "Test_2": {
      },
      "Test_3": {
      }
    },
    "Suite_2": {
      "Test_1": {
      },
      "Test_2": {
      },
      "Test_3": {
      }
    }
  }
}
)");
}

TEST(RuntimeConfig, DumpConfigBuffer_ModifiedSettings) {
  RuntimeConfig config;
  std::vector<std::string> errors;
  PopulateConfig(config, R"({
    "settings": {
      "enable_progress_log": true,
      "disable_autorun": true,
      "enable_autorun_immediately": true,
      "enable_shutdown_on_completion": true,
      "enable_pgraph_region_diff": true,
      "skip_tests_by_default": true,
      "output_directory_path": "c:/foobar"
    }
  })");

  std::stringstream output;
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };

  EXPECT_TRUE(config.DumpConfigToStream(output, suites, errors));
  EXPECT_TRUE(errors.empty());
  EXPECT_STREQ(output.str().c_str(), R"({
  "settings": {
    "enable_progress_log": true,
    "disable_autorun": true,
    "enable_autorun_immediately": true,
    "enable_shutdown_on_completion": true,
    "enable_pgraph_region_diff": true,
    "skip_tests_by_default": true,
    "output_directory_path": "c:/foobar"
  },
  "test_suites": {
    "Suite_1": {
      "Test_1": {
      },
      "Test_2": {
      },
      "Test_3": {
      }
    },
    "Suite_2": {
      "Test_1": {
      },
      "Test_2": {
      },
      "Test_3": {
      }
    }
  }
}
)");
}

TEST(RuntimeConfig, DumpConfigBuffer_FilteredTests_SkipByDefaultEnabled) {
  RuntimeConfig config;
  std::vector<std::string> errors;
  PopulateConfig(config, R"({
    "settings": {
      "enable_progress_log": true,
      "disable_autorun": true,
      "enable_autorun_immediately": true,
      "enable_shutdown_on_completion": true,
      "enable_pgraph_region_diff": true,
      "skip_tests_by_default": true,
      "output_directory_path": "c:/foobar"
    },
    "test_suites": {
      "Suite_2": {
        "skipped": true,
        "Test_1": { "skipped": false }
      }
    }
  })");

  std::stringstream output;
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };

  EXPECT_TRUE(config.DumpConfigToStream(output, suites, errors));
  EXPECT_TRUE(errors.empty());
  EXPECT_STREQ(output.str().c_str(), R"({
  "settings": {
    "enable_progress_log": true,
    "disable_autorun": true,
    "enable_autorun_immediately": true,
    "enable_shutdown_on_completion": true,
    "enable_pgraph_region_diff": true,
    "skip_tests_by_default": true,
    "output_directory_path": "c:/foobar"
  },
  "test_suites": {
    "Suite_1": {
      "Test_1": {
      },
      "Test_2": {
      },
      "Test_3": {
      }
    },
    "Suite_2": {
      "skipped": true,
      "Test_1": {
        "skipped": false
      },
      "Test_2": {
      },
      "Test_3": {
      }
    }
  }
}
)");
}

TEST(RuntimeConfig, DumpConfigBuffer_FilteredTests_SkipByDefaultDisabled) {
  RuntimeConfig config;
  std::vector<std::string> errors;
  PopulateConfig(config, R"({
    "settings": {
      "enable_progress_log": true,
      "disable_autorun": true,
      "enable_autorun_immediately": true,
      "enable_shutdown_on_completion": true,
      "enable_pgraph_region_diff": true,
      "skip_tests_by_default": false,
      "output_directory_path": "c:/foobar"
    },
    "test_suites": {
      "Suite_1": {
        "skipped": false
      },
      "Suite_2": {
        "skipped": true,
        "Test_1": { "skipped": false }
      }
    }
  })");

  std::stringstream output;
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };

  EXPECT_TRUE(config.DumpConfigToStream(output, suites, errors));
  EXPECT_TRUE(errors.empty());
  EXPECT_STREQ(output.str().c_str(), R"({
  "settings": {
    "enable_progress_log": true,
    "disable_autorun": true,
    "enable_autorun_immediately": true,
    "enable_shutdown_on_completion": true,
    "enable_pgraph_region_diff": true,
    "skip_tests_by_default": false,
    "output_directory_path": "c:/foobar"
  },
  "test_suites": {
    "Suite_1": {
      "skipped": false,
      "Test_1": {
      },
      "Test_2": {
      },
      "Test_3": {
      }
    },
    "Suite_2": {
      "skipped": true,
      "Test_1": {
        "skipped": false
      },
      "Test_2": {
      },
      "Test_3": {
      }
    }
  }
}
)");
}

#else  // ifdef DUMP_CONFIG_FILE

static std::vector<std::string> FlattenEnabledTests(std::vector<std::shared_ptr<TestSuite> >& suites);

TEST(RuntimeConfig, LoadConfigBuffer_InvalidBuffer) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("123xyz", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "Failed to parse config file.");
}

TEST(RuntimeConfig, LoadConfigBuffer_NoSettings) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("{}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "'settings' not found");
}

TEST(RuntimeConfig, LoadConfigBuffer_SettingsInvalid) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("{\"settings\": 123}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "'settings' not an object");
}

TEST(RuntimeConfig, LoadConfigBuffer_SettingsEmpty) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_TRUE(config.LoadConfigBuffer("{\"settings\": {}}", errors));
  EXPECT_TRUE(errors.empty());

  EXPECT_EQ(config.enable_progress_log(), DEFAULT_ENABLE_PROGRESS_LOG);
  EXPECT_EQ(config.disable_autorun(), DEFAULT_DISABLE_AUTORUN);
  EXPECT_EQ(config.enable_autorun_immediately(), DEFAULT_AUTORUN_IMMEDIATELY);
  EXPECT_EQ(config.enable_shutdown_on_completion(), DEFAULT_ENABLE_SHUTDOWN);
  EXPECT_EQ(config.enable_pgraph_region_diff(), DEFAULT_ENABLE_PGRAPH_REGION_DIFF);
  EXPECT_EQ(config.skip_tests_by_default(), DEFAULT_SKIP_TESTS_BY_DEFAULT);
  EXPECT_EQ(config.output_directory_path(), RuntimeConfig::SanitizePath(DEFAULT_OUTPUT_DIRECTORY_PATH));
}

TEST(RuntimeConfig, LoadConfigBuffer_ValidBoolOption) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_TRUE(config.LoadConfigBuffer("{\"settings\": {\"enable_progress_log\": true}}", errors));
  EXPECT_TRUE(errors.empty());
  EXPECT_TRUE(config.enable_progress_log());
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidEnableProgressLog) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("{\"settings\": {\"enable_progress_log\": 1}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "settings[enable_progress_log] must be a boolean");
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidDisableAutorun) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("{\"settings\": {\"disable_autorun\": 1}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "settings[disable_autorun] must be a boolean");
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidEnableAutorunImmediately) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("{\"settings\": {\"enable_autorun_immediately\": 1}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "settings[enable_autorun_immediately] must be a boolean");
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidEnableShutdownOnCompletion) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("{\"settings\": {\"enable_shutdown_on_completion\": 1}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "settings[enable_shutdown_on_completion] must be a boolean");
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidEnablePGRAPHRegionDiff) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("{\"settings\": {\"enable_pgraph_region_diff\": 1}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "settings[enable_pgraph_region_diff] must be a boolean");
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidSkipTestsByDefault) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("{\"settings\": {\"skip_tests_by_default\": 1}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "settings[skip_tests_by_default] must be a boolean");
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidOutputDirectoryPath) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("{\"settings\": {\"output_directory_path\": 1}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "settings[output_directory_path] must be a string");
}

TEST(RuntimeConfig, LoadConfigBuffer_SetOutputDirectoryPath) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_TRUE(config.LoadConfigBuffer("{\"settings\": {\"output_directory_path\": \"q/test\\\\foo\"}}", errors));
  EXPECT_TRUE(errors.empty());
  EXPECT_STREQ(config.output_directory_path().c_str(), R"(q\test\foo)");
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidTestSuitesContainer) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("{\"settings\": {}, \"test_suites\": 123}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "'test_suites' not an object");
}

TEST(RuntimeConfig, LoadConfigBuffer_EmptyTestSuitesContainer) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_TRUE(config.LoadConfigBuffer("{\"settings\": {}, \"test_suites\": {}}", errors));
  EXPECT_TRUE(errors.empty());
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidTestSuite_IsIgnoredButNoted) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_TRUE(config.LoadConfigBuffer("{\"settings\": {}, \"test_suites\": {\"foo\": 1}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "test_suites[foo] must be an object. Ignoring");
}

TEST(RuntimeConfig, LoadConfigBuffer_EmptyTestSuite) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_TRUE(config.LoadConfigBuffer("{\"settings\": {}, \"test_suites\": {\"Suite\": {}}}", errors));
  EXPECT_TRUE(errors.empty());
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidMemberInTestSuite_IsIgnoredButNoted) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_TRUE(config.LoadConfigBuffer("{\"settings\": {}, \"test_suites\": {\"Suite\": {\"foo\": 1}}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "test_suites[Suite][foo] must be an object. Ignoring");
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidBooleanMemberInTestSuite_IsIgnoredButNoted) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_TRUE(config.LoadConfigBuffer("{\"settings\": {}, \"test_suites\": {\"Suite\": {\"foo\": true}}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "test_suites[Suite][foo] must be an object. Ignoring");
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidSkippedMemberInTestSuite) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(
      config.LoadConfigBuffer("{\"settings\": {}, \"test_suites\": {\"Suite\": {\"skipped\": \"true\"}}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "test_suites[Suite][skipped] must be a boolean.");
}

TEST(RuntimeConfig, LoadConfigBuffer_EmptyTestCase) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_TRUE(config.LoadConfigBuffer("{\"settings\": {}, \"test_suites\": {\"Suite\": {\"Case\": {}}}}", errors));
  EXPECT_TRUE(errors.empty());
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidBooleanMemberInTestCase_IsIgnoredButNoted) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_TRUE(
      config.LoadConfigBuffer("{\"settings\": {}, \"test_suites\": {\"Suite\": {\"Case\": {\"foo\": 1}}}}", errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "test_suites[Suite][Case][foo] unsupported. Ignoring");
}

TEST(RuntimeConfig, LoadConfigBuffer_InvalidSkippedsMemberInTestCase) {
  RuntimeConfig config;
  std::vector<std::string> errors;

  EXPECT_FALSE(config.LoadConfigBuffer("{\"settings\": {}, \"test_suites\": {\"Suite\": {\"Case\": {\"skipped\": 1}}}}",
                                       errors));
  EXPECT_EQ(errors.size(), 1);
  EXPECT_STREQ(errors.at(0).c_str(), "test_suites[Suite][Case][skipped] must be a boolean");
}

TEST(RuntimeConfig, ApplyConfig_NoJSON_EmptyTestSuite) {
  RuntimeConfig config;
  std::vector<std::shared_ptr<TestSuite> > suites;
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
}

TEST(RuntimeConfig, ApplyConfig_NoJSON) {
  RuntimeConfig config;
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector<std::shared_ptr<TestSuite> > suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "SuiteOne", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "SuiteTwo", test_suite_config),
  };
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
  EXPECT_EQ(suites.size(), 2);
}

TEST(RuntimeConfig, ApplyConfig_NoTestsFiltered_DefaultNotSkipped) {
  RuntimeConfig config;
  PopulateConfig(config, "{\"settings\": {}}");
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector<std::shared_ptr<TestSuite> > suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
  ASSERT_THAT(FlattenEnabledTests(suites), ElementsAre("Suite_1::Test_1", "Suite_1::Test_2", "Suite_1::Test_3",
                                                       "Suite_2::Test_1", "Suite_2::Test_2", "Suite_2::Test_3"));
}

TEST(RuntimeConfig, ApplyConfig_NonMatchingTestsFiltered_DefaultNotSkipped) {
  RuntimeConfig config;
  PopulateConfig(config, R"({"settings": {}, "test_suites": { "MadeUp": { "skipped": true } }})");
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector<std::shared_ptr<TestSuite> > suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
  ASSERT_THAT(FlattenEnabledTests(suites), ElementsAre("Suite_1::Test_1", "Suite_1::Test_2", "Suite_1::Test_3",
                                                       "Suite_2::Test_1", "Suite_2::Test_2", "Suite_2::Test_3"));
}

TEST(RuntimeConfig, ApplyConfig_TestSuiteExplicitSkip_DefaultNotSkipped) {
  RuntimeConfig config;
  PopulateConfig(config, R"({"settings": {}, "test_suites": { "Suite_1": { "skipped": true } }})");
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector<std::shared_ptr<TestSuite> > suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
  ASSERT_THAT(FlattenEnabledTests(suites), ElementsAre("Suite_2::Test_1", "Suite_2::Test_2", "Suite_2::Test_3"));
}

TEST(RuntimeConfig, ApplyConfig_TestCaseExplicitSkip_DefaultNotSkipped) {
  RuntimeConfig config;
  PopulateConfig(config, R"({"settings": {}, "test_suites": { "Suite_1": { "Test_2": { "skipped": true } } }})");
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector<std::shared_ptr<TestSuite> > suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
  ASSERT_THAT(FlattenEnabledTests(suites), ElementsAre("Suite_1::Test_1", "Suite_1::Test_3", "Suite_2::Test_1",
                                                       "Suite_2::Test_2", "Suite_2::Test_3"));
}

TEST(RuntimeConfig, ApplyConfig_TestCaseExplicitEnableWithinDisabledSuite_DefaultNotSkipped) {
  RuntimeConfig config;
  PopulateConfig(config, R"({
    "settings": { "skip_tests_by_default": false },
    "test_suites": {
      "Suite_2": {
        "skipped": true,
        "Test_1": { "skipped": false }
      }
    }
  })");
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
  ASSERT_THAT(FlattenEnabledTests(suites),
              ElementsAre("Suite_1::Test_1", "Suite_1::Test_2", "Suite_1::Test_3", "Suite_2::Test_1"));
}

TEST(RuntimeConfig, ApplyConfig_NoTestsFiltered_DefaultSkipped) {
  RuntimeConfig config;
  PopulateConfig(config, R"({"settings": {"skip_tests_by_default": true}})");
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector<std::shared_ptr<TestSuite> > suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
  ASSERT_THAT(FlattenEnabledTests(suites), ElementsAre());
}

TEST(RuntimeConfig, ApplyConfig_NonMatchingTestsFiltered_DefaultSkipped) {
  RuntimeConfig config;
  PopulateConfig(config, R"({"settings": {"skip_tests_by_default": true}, "test_suites": { "MadeUp": { "skipped":
  true } }})");
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector<std::shared_ptr<TestSuite> > suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
  ASSERT_THAT(FlattenEnabledTests(suites), ElementsAre());
}

TEST(RuntimeConfig, ApplyConfig_TestSuiteExplicitEnable_DefaultSkipped) {
  RuntimeConfig config;
  PopulateConfig(config, R"({
    "settings": {"skip_tests_by_default": true},
    "test_suites": {
      "Suite_2": { "skipped": false }
    }
  })");
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector<std::shared_ptr<TestSuite> > suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
  ASSERT_THAT(FlattenEnabledTests(suites), ElementsAre("Suite_2::Test_1", "Suite_2::Test_2", "Suite_2::Test_3"));
}

TEST(RuntimeConfig, ApplyConfig_TestCaseExplicitEnable_DefaultSkipped) {
  RuntimeConfig config;
  PopulateConfig(config, R"({
    "settings": { "skip_tests_by_default": true },
    "test_suites": {
      "Suite_2": {
        "Test_1": { "skipped": false }
      }
    }
  })");
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
  ASSERT_THAT(FlattenEnabledTests(suites), ElementsAre("Suite_2::Test_1"));
}

TEST(RuntimeConfig, ApplyConfig_TestCaseExplicitDisableWithinEnabledSuite_DefaultSkipped) {
  RuntimeConfig config;
  PopulateConfig(config, R"({
    "settings": { "skip_tests_by_default": true },
    "test_suites": {
      "Suite_2": {
        "skipped": false,
        "Test_1": { "skipped": true }
      }
    }
  })");
  TestHost host(1024, 768, 32, 32);
  auto test_suite_config = TestSuite::Config{false, false};
  std::vector suites = {
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_1", test_suite_config),
      std::make_shared<TestSuite>(host, "/dev/null", "Suite_2", test_suite_config),
  };
  std::vector<std::string> errors;

  EXPECT_TRUE(config.ApplyConfig(suites, errors));
  EXPECT_TRUE(errors.empty());
  ASSERT_THAT(FlattenEnabledTests(suites), ElementsAre("Suite_2::Test_2", "Suite_2::Test_3"));
}

static std::vector<std::string> FlattenEnabledTests(std::vector<std::shared_ptr<TestSuite> >& suites) {
  std::vector<std::string> ret;
  for (auto& suite : suites) {
    for (auto& test_case : suite->TestNames()) {
      ret.push_back(suite->Name() + "::" + test_case);
    }
  }
  std::sort(ret.begin(), ret.end());
  return std::move(ret);
}

#endif  // ifdef DUMP_CONFIG_FILE

static void PopulateConfig(RuntimeConfig& config, const std::string& json) {
  std::vector<std::string> errors;
  EXPECT_TRUE(config.LoadConfigBuffer(json, errors));
  ASSERT_THAT(errors, ElementsAre());
}

#pragma mark Stub definitions

TestHost::TestHost(uint32_t framebuffer_width, uint32_t framebuffer_height, uint32_t max_texture_width,
                   uint32_t max_texture_height, uint32_t max_texture_depth) {}
TestHost::~TestHost() {}

TextureStage::TextureStage() {}

static void NoOpTestBody() {}

TestSuite::TestSuite(TestHost& host, std::string output_dir, std::string suite_name, const Config& config)
    : host_(host),
      output_dir_(std::move(output_dir)),
      suite_name_(std::move(suite_name)),
      pgraph_diff_(false, config.enable_progress_log),
      enable_progress_log_{config.enable_progress_log},
      enable_pgraph_region_diff_{config.enable_pgraph_region_diff} {
  tests_["Test_1"] = NoOpTestBody;
  tests_["Test_2"] = NoOpTestBody;
  tests_["Test_3"] = NoOpTestBody;
}

std::vector<std::string> TestSuite::TestNames() const {
  std::vector<std::string> ret;
  ret.reserve(tests_.size());

  for (auto& kv : tests_) {
    ret.push_back(kv.first);
  }
  return std::move(ret);
}

void TestSuite::DisableTests(const std::set<std::string>& tests_to_skip) {
  for (auto& name : tests_to_skip) {
    tests_.erase(name);
  }
}
void TestSuite::Initialize() {}
void TestSuite::Deinitialize() {}
void TestSuite::SetupTest() {}
void TestSuite::TearDownTest() {}

PGRAPHDiffToken::PGRAPHDiffToken(bool initialize, bool enable_progress_log)
    : registers{0}, enable_progress_log{enable_progress_log} {}
