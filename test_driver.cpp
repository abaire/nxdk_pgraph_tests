#include "test_driver.h"

#include <hal/debug.h>
#include <pbkit/pbkit.h>
#include <windows.h>

#include "menu_item.h"

TestDriver::TestDriver(const std::vector<std::shared_ptr<TestSuite>> &test_suites, uint32_t framebuffer_width,
                       uint32_t framebuffer_height)
    : test_suites_(test_suites), framebuffer_width_(framebuffer_width), framebuffer_height_(framebuffer_height) {
  auto on_run_all = [this]() { RunAllTestsNonInteractive(); };
  auto on_exit = [this]() { running_ = false; };
  menu_ = std::make_shared<MenuItemRoot>(test_suites, on_run_all, on_exit, framebuffer_width, framebuffer_height);
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

    menu_->Draw();
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

void TestDriver::OnBack() { menu_->Deactivate(); }

void TestDriver::OnStart() { menu_->Activate(); }

void TestDriver::OnBlack() { running_ = false; }

void TestDriver::OnA() { menu_->Activate(); }

void TestDriver::OnB() { menu_->Deactivate(); }

void TestDriver::OnUp() { menu_->CursorUp(); }

void TestDriver::OnDown() { menu_->CursorDown(); }

void TestDriver::OnLeft() { menu_->CursorLeft(); }

void TestDriver::OnRight() { menu_->CursorRight(); }
