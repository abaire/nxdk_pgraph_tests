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

/**
 * Handles input processing and test execution.
 */
class TestDriver {
 public:
  TestDriver(TestHost &host, const std::vector<std::shared_ptr<TestSuite>> &test_suites, uint32_t framebuffer_width,
             uint32_t framebuffer_height, bool show_options_menu, bool disable_autorun, bool autorun_immediately);
  ~TestDriver();

  //! Enters a loop that reacts to user input until the user requests an exit.
  void Run();

  //! Runs all tests automatically without reacting to any user input.
  void RunAllTestsNonInteractive();

 private:
  void OnControllerAdded(const SDL_ControllerDeviceEvent &event);
  void OnControllerRemoved(const SDL_ControllerDeviceEvent &event);
  void OnControllerButtonEvent(const SDL_ControllerButtonEvent &event);
  void OnButtonActivated(SDL_GameControllerButton button, bool is_repeat);

  void ShowDebugMessageAndExit();

  void OnBack(bool is_repeat);
  void OnStart(bool is_repeat);
  void OnBlack(bool is_repeat);
  void OnWhite(bool is_repeat);
  void OnUp(bool is_repeat);
  void OnDown(bool is_repeat);
  void OnLeft(bool is_repeat);
  void OnRight(bool is_repeat);
  void OnA(bool is_repeat);
  void OnB(bool is_repeat);
  void OnX(bool is_repeat);
  void OnY(bool is_repeat);

 private:
  volatile bool running_{true};

  const std::vector<std::shared_ptr<TestSuite>> &test_suites_;
  SDL_GameController *gamepads_[kMaxGamepads]{nullptr};

  TestHost &test_host_;
  std::shared_ptr<MenuItem> active_menu_;
  std::shared_ptr<MenuItem> root_menu_;
  std::shared_ptr<MenuItem> options_menu_;
};

#endif  // NXDK_PGRAPH_TESTS_TEST_DRIVER_H
