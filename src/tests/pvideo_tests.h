#pragma once

#include <memory>
#include <string>

#include "test_host.h"
#include "test_suite.h"

class PvideoTests : public TestSuite {
 public:
  PvideoTests(TestHost &host, std::string output_dir);
  void Initialize() override;
  void Deinitialize() override;

 private:
  void TestStopBehavior();
  void SetVideoFrameCR8YB8CB8YA8(const void *pixels, uint32_t width, uint32_t height);

 private:
  uint8_t *video_{nullptr};
};
