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
  void TestInvalidSizeIn();

  void SetCheckerboardVideoFrameCR8YB8CB8YA8(uint32_t first_color, uint32_t second_color, uint32_t checker_size,
                                             uint32_t x_offset = 0, uint32_t y_offset = 0, uint32_t width = 0,
                                             uint32_t height = 0);
  void SetVideoFrameCR8YB8CB8YA8(const void *pixels, uint32_t width, uint32_t height);
  void DrawFullscreenOverlay();

 private:
  uint8_t *video_{nullptr};
};
