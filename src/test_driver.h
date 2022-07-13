#ifndef NXDK_PGRAPH_TESTS_TEST_DRIVER_H
#define NXDK_PGRAPH_TESTS_TEST_DRIVER_H

#include <SDL.h>

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "test_host.h"
#include "tests/test_suite.h"

constexpr uint32_t kMaxGamepads = 4;

struct MenuItem;

class TestDriver {
 public:
  TestDriver(TestHost &host, const std::vector<std::shared_ptr<TestSuite>> &test_suites, uint32_t framebuffer_width,
             uint32_t framebuffer_height, bool show_options_menu = false);
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
  void OnX();
  void OnY();

 private:
  volatile bool running_{true};
  // Whether tests should render once and stop (true) or continually render frames (false).
  bool one_shot_tests_{true};

  const std::vector<std::shared_ptr<TestSuite>> &test_suites_;
  SDL_GameController *gamepads_[kMaxGamepads]{nullptr};

  uint32_t framebuffer_width_;
  uint32_t framebuffer_height_;

  TestHost &test_host_;
  std::shared_ptr<MenuItem> active_menu_;
  std::shared_ptr<MenuItem> root_menu_;
  std::shared_ptr<MenuItem> options_menu_;
};

#endif  // NXDK_PGRAPH_TESTS_TEST_DRIVER_H
