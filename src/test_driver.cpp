#include "test_driver.h"

#include <hal/debug.h>
#include <pbkit/pbkit.h>
#include <windows.h>

#include "menu_item.h"

TestDriver::TestDriver(TestHost &host, const std::vector<std::shared_ptr<TestSuite>> &test_suites,
                       uint32_t framebuffer_width, uint32_t framebuffer_height, bool show_options_menu)
    : test_host_(host),
      test_suites_(test_suites),
      framebuffer_width_(framebuffer_width),
      framebuffer_height_(framebuffer_height) {
  auto on_run_all = [this]() { RunAllTestsNonInteractive(); };
  auto on_exit = [this]() { running_ = false; };
  root_menu_ = std::make_shared<MenuItemRoot>(test_suites, on_run_all, on_exit, framebuffer_width, framebuffer_height);

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
  while (running_) {
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
          // Fallthrough
        case SDL_CONTROLLERBUTTONUP:
          OnControllerButtonEvent(event.cbutton);
          break;

        default:
          break;
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

  switch (event.button) {
    case SDL_CONTROLLER_BUTTON_BACK:
      OnBack();
      break;

    case SDL_CONTROLLER_BUTTON_START:
      OnStart();
      break;

    case SDL_CONTROLLER_BUTTON_DPAD_UP:
      OnUp();
      break;

    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
      OnDown();
      break;

    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
      OnLeft();
      break;

    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
      OnRight();
      break;

    case SDL_CONTROLLER_BUTTON_A:
      OnA();
      break;

    case SDL_CONTROLLER_BUTTON_B:
      OnB();
      break;

    case SDL_CONTROLLER_BUTTON_X:
      OnX();
      break;

    case SDL_CONTROLLER_BUTTON_Y:
      OnY();
      break;

    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
      OnBlack();
      break;
  }
}

void TestDriver::ShowDebugMessageAndExit() {
  pb_show_debug_screen();
  Sleep(2000);
  running_ = false;
}

void TestDriver::OnBack() { active_menu_->Deactivate(); }

void TestDriver::OnStart() { active_menu_->Activate(); }

void TestDriver::OnBlack() { running_ = false; }

void TestDriver::OnA() { active_menu_->Activate(); }

void TestDriver::OnB() { active_menu_->Deactivate(); }

void TestDriver::OnX() { active_menu_->ActivateCurrentSuite(); }

void TestDriver::OnY() {
  bool save_results = !test_host_.GetSaveResults();
  test_host_.SetSaveResults(save_results);
  MenuItemTest::SetOneShotMode(save_results);
}

void TestDriver::OnUp() { active_menu_->CursorUp(); }

void TestDriver::OnDown() { active_menu_->CursorDown(); }

void TestDriver::OnLeft() { active_menu_->CursorLeft(); }

void TestDriver::OnRight() { active_menu_->CursorRight(); }
