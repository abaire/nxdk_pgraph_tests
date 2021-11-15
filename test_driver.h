#ifndef NXDK_PGRAPH_TESTS_TEST_DRIVER_H
#define NXDK_PGRAPH_TESTS_TEST_DRIVER_H

#include <SDL.h>

#include <cstdint>
#include <list>
#include <memory>
#include <vector>

#include "tests/test_suite.h"

constexpr uint32_t kMaxGamepads = 4;

struct MenuItem;

class TestDriver {
 public:
  TestDriver(const std::vector<std::shared_ptr<TestSuite>> &test_suites, uint32_t framebuffer_width,
             uint32_t framebuffer_height);
  ~TestDriver();

  void Run();
  void RunAllTestsNonInteractive();

 private:
  void OnControllerAdded(const SDL_ControllerDeviceEvent &event);
  void OnControllerRemoved(const SDL_ControllerDeviceEvent &event);
  void OnControllerButtonEvent(const SDL_ControllerButtonEvent &event);

  void ShowDebugMessageAndExit();

  void OnBack();
  void OnStart();
  void OnBlack();
  void OnUp();
  void OnDown();
  void OnLeft();
  void OnRight();
  void OnA();
  void OnB();

 private:
  bool running_{true};
  const std::vector<std::shared_ptr<TestSuite>> &test_suites_;
  SDL_GameController *gamepads_[kMaxGamepads]{nullptr};

  uint32_t framebuffer_width_;
  uint32_t framebuffer_height_;

  std::shared_ptr<MenuItem> menu_;
};

#endif  // NXDK_PGRAPH_TESTS_TEST_DRIVER_H
