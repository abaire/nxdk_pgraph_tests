#include "test_driver.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-attributes"
#include <hal/debug.h>
#pragma clang diagnostic pop

#include <pbkit/pbkit.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#include <windows.h>
#pragma clang diagnostic pop

#include "menu_item.h"

static constexpr auto kButtonRepeatMilliseconds = 100;

TestDriver::TestDriver(TestHost &host, const std::vector<std::shared_ptr<TestSuite>> &test_suites,
                       uint32_t framebuffer_width, uint32_t framebuffer_height, bool show_options_menu,
                       bool disable_autorun, bool autorun_immediately)
    : test_suites_(test_suites), test_host_(host) {
  auto on_run_all = [this]() { RunAllTestsNonInteractive(); };
  auto on_exit = [this]() { running_ = false; };
  root_menu_ = std::make_shared<MenuItemRoot>(test_suites, on_run_all, on_exit, framebuffer_width, framebuffer_height,
                                              disable_autorun, autorun_immediately);

  if (show_options_menu) {
    auto on_options_exit = [this]() { active_menu_ = root_menu_; };
    options_menu_ =
        std::make_shared<MenuItemOptions>(test_suites, on_options_exit, framebuffer_width, framebuffer_height);
    active_menu_ = options_menu_;
  } else {
    active_menu_ = root_menu_;
  }
}

TestDriver::~TestDriver() {
  for (auto handle : gamepads_) {
    if (handle) {
      SDL_GameControllerClose(handle);
    }
  }
}

void TestDriver::Run() {
  std::map<SDL_GameControllerButton, std::chrono::time_point<std::chrono::steady_clock>> button_repeat_map;

  while (running_) {
    auto now = std::chrono::high_resolution_clock::now();
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_CONTROLLERDEVICEADDED:
          OnControllerAdded(event.cdevice);
          break;

        case SDL_CONTROLLERDEVICEREMOVED:
          OnControllerRemoved(event.cdevice);
          break;

        case SDL_CONTROLLERBUTTONDOWN:
          button_repeat_map[static_cast<SDL_GameControllerButton>(event.cbutton.button)] = now;
          break;

        case SDL_CONTROLLERBUTTONUP:
          button_repeat_map.erase(static_cast<SDL_GameControllerButton>(event.cbutton.button));
          OnControllerButtonEvent(event.cbutton);
          break;

        default:
          break;
      }
    }

    for (const auto &pair : button_repeat_map) {
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - pair.second).count();
      if (elapsed > kButtonRepeatMilliseconds) {
        OnButtonActivated(pair.first, true);
        button_repeat_map[pair.first] = now;
      }
    }

    if (!test_host_.GetSaveResults()) {
      active_menu_->SetBackgroundColor(0xFF3E1E1E);
    } else {
      active_menu_->SetBackgroundColor(0xFF1E1E1E);
    }

    active_menu_->Draw();

    Sleep(10);
  }
}

void TestDriver::RunAllTestsNonInteractive() {
  for (auto &suite : test_suites_) {
    suite->Initialize();
    suite->RunAll();
    suite->Deinitialize();
  }
  running_ = false;
}

void TestDriver::OnControllerAdded(const SDL_ControllerDeviceEvent &event) {
  SDL_GameController *controller = SDL_GameControllerOpen(event.which);
  if (!controller) {
    debugPrint("Failed to handle controller add event.");
    debugPrint("%s", SDL_GetError());
    ShowDebugMessageAndExit();
    return;
  }

  auto player_index = SDL_GameControllerGetPlayerIndex(controller);

  if (player_index < 0 || player_index >= kMaxGamepads) {
    debugPrint("Player index %d is out of range (max %d).", player_index, kMaxGamepads);
    ShowDebugMessageAndExit();
    return;
  }

  gamepads_[player_index] = controller;
}

void TestDriver::OnControllerRemoved(const SDL_ControllerDeviceEvent &event) {
  SDL_GameController *controller = SDL_GameControllerFromInstanceID(event.which);

  for (auto i = 0; i < kMaxGamepads; ++i) {
    SDL_GameController *handle = gamepads_[i];
    if (!handle) {
      continue;
    }

    SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(handle));
    if (id == event.which) {
      SDL_GameControllerClose(controller);
      gamepads_[i] = nullptr;
      return;
    }
  }

  // Handle case where the controller was never mapped for some reason.
  SDL_GameControllerClose(controller);
}

void TestDriver::OnControllerButtonEvent(const SDL_ControllerButtonEvent &event) {
  if (event.state != SDL_RELEASED) {
    return;
  }

  OnButtonActivated(static_cast<SDL_GameControllerButton>(event.button), false);
}

void TestDriver::OnButtonActivated(SDL_GameControllerButton button, bool is_repeat) {
  switch (button) {
    case SDL_CONTROLLER_BUTTON_BACK:
      OnBack(is_repeat);
      break;

    case SDL_CONTROLLER_BUTTON_START:
      OnStart(is_repeat);
      break;

    case SDL_CONTROLLER_BUTTON_DPAD_UP:
      OnUp(is_repeat);
      break;

    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
      OnDown(is_repeat);
      break;

    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
      OnLeft(is_repeat);
      break;

    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
      OnRight(is_repeat);
      break;

    case SDL_CONTROLLER_BUTTON_A:
      OnA(is_repeat);
      break;

    case SDL_CONTROLLER_BUTTON_B:
      OnB(is_repeat);
      break;

    case SDL_CONTROLLER_BUTTON_X:
      OnX(is_repeat);
      break;

    case SDL_CONTROLLER_BUTTON_Y:
      OnY(is_repeat);
      break;

    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
      OnBlack(is_repeat);
      break;

    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
      OnWhite(is_repeat);
      break;

    default:
      break;
  }
}

void TestDriver::ShowDebugMessageAndExit() {
  pb_show_debug_screen();
  Sleep(2000);
  running_ = false;
}

void TestDriver::OnBack(bool is_repeat) {
  if (is_repeat) {
    return;
  }
  active_menu_->Deactivate();
}

void TestDriver::OnStart(bool is_repeat) {
  if (is_repeat) {
    return;
  }
  active_menu_->Activate();
}

void TestDriver::OnBlack(bool is_repeat) {
  if (is_repeat) {
    return;
  }
  running_ = false;
}

void TestDriver::OnWhite(bool is_repeat) {}

void TestDriver::OnA(bool is_repeat) {
  if (is_repeat) {
    return;
  }
  active_menu_->Activate();
}

void TestDriver::OnB(bool is_repeat) {
  if (is_repeat) {
    return;
  }
  active_menu_->Deactivate();
}

void TestDriver::OnX(bool is_repeat) {
  if (is_repeat) {
    return;
  }
  active_menu_->ActivateCurrentSuite();
}

void TestDriver::OnY(bool is_repeat) {
  if (is_repeat) {
    return;
  }
  bool save_results = !test_host_.GetSaveResults();
  test_host_.SetSaveResults(save_results);
  MenuItemTest::SetOneShotMode(save_results);
}

void TestDriver::OnUp(bool is_repeat) { active_menu_->CursorUp(is_repeat); }

void TestDriver::OnDown(bool is_repeat) { active_menu_->CursorDown(is_repeat); }

void TestDriver::OnLeft(bool is_repeat) { active_menu_->CursorLeft(is_repeat); }

void TestDriver::OnRight(bool is_repeat) { active_menu_->CursorRight(is_repeat); }
