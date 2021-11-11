#include "test_base.h"

TestBase::TestBase(TestHost& host, std::string output_dir) : host_(host), output_dir_(std::move(output_dir)) {}
