#include "menu_item.h"

#include <pbkit/pbkit.h>

#include <chrono>
#include <utility>

#include "tests/test_suite.h"

static constexpr uint32_t kAutoTestAllTimeoutMilliseconds = 3000;
static constexpr uint32_t kNumItemsPerPage = 12;
static constexpr uint32_t kNumItemsPerHalfPage = kNumItemsPerPage >> 1;

void MenuItem::PrepareDraw(uint32_t background_color) const {
  pb_wait_for_vbl();
  pb_target_back_buffer();
  pb_reset();
  pb_fill(0, 0, width, height, background_color);
  pb_erase_text_screen();
}

void MenuItem::Swap() {
  pb_draw_text_screen();
  while (pb_busy()) {
    /* Wait for completion... */
  }

  /* Swap buffers (if we can) */
  while (pb_finished()) {
    /* Not ready to swap yet */
  }
}

void MenuItem::Draw() const {
  if (active_submenu) {
    active_submenu->Draw();
    return;
  }

  PrepareDraw();

  const char *cursor_prefix = "> ";
  const char *normal_prefix = "  ";
  const char *cursor_suffix = " <";
  const char *normal_suffix = "";

  uint32_t i = 0;
  if (cursor_position > kNumItemsPerHalfPage) {
    i = cursor_position - kNumItemsPerHalfPage;
    if (i + kNumItemsPerPage > submenu.size()) {
      if (submenu.size() < kNumItemsPerPage) {
        i = 0;
      } else {
        i = submenu.size() - kNumItemsPerPage;
      }
    }
  }

  if (i) {
    pb_print("...\n");
  }

  uint32_t i_end = i + std::min(kNumItemsPerPage, submenu.size());

  for (; i < i_end; ++i) {
    const char *prefix = i == cursor_position ? cursor_prefix : normal_prefix;
    const char *suffix = i == cursor_position ? cursor_suffix : normal_suffix;
    pb_print("%s%s%s\n", prefix, submenu[i]->name.c_str(), suffix);
  }

  if (i_end < submenu.size()) {
    pb_print("...\n");
  }

  Swap();
}

void MenuItem::OnEnter() {}

void MenuItem::Activate() {
  if (active_submenu) {
    active_submenu->Activate();
    return;
  }

  auto activated_item = submenu[cursor_position];
  if (activated_item->IsEnterable()) {
    active_submenu = activated_item;
    activated_item->OnEnter();
  } else {
    activated_item->Activate();
  }
}

bool MenuItem::Deactivate() {
  if (!active_submenu) {
    return false;
  }

  bool was_submenu_activated = active_submenu->Deactivate();
  if (!was_submenu_activated) {
    active_submenu.reset();
  }
  return true;
}

void MenuItem::CursorUp() {
  if (active_submenu) {
    active_submenu->CursorUp();
    return;
  }

  if (cursor_position > 0) {
    --cursor_position;
  } else {
    cursor_position = submenu.size() - 1;
  }
}

void MenuItem::CursorDown() {
  if (active_submenu) {
    active_submenu->CursorDown();
    return;
  }

  if (cursor_position < submenu.size() - 1) {
    ++cursor_position;
  } else {
    cursor_position = 0;
  }
}

void MenuItem::CursorLeft() {
  if (active_submenu) {
    active_submenu->CursorLeft();
    return;
  }

  if (cursor_position > kNumItemsPerHalfPage) {
    cursor_position -= kNumItemsPerHalfPage;
  } else {
    cursor_position = 0;
  }
}

void MenuItem::CursorRight() {
  if (active_submenu) {
    active_submenu->CursorRight();
    return;
  }

  cursor_position += kNumItemsPerHalfPage;
  if (cursor_position >= submenu.size()) {
    cursor_position = submenu.size() - 1;
  }
}

void MenuItem::CursorUpAndActivate() {
  active_submenu = nullptr;
  CursorUp();
  Activate();
}

void MenuItem::CursorDownAndActivate() {
  active_submenu = nullptr;
  CursorDown();
  Activate();
}

MenuItemCallable::MenuItemCallable(std::function<void()> callback, std::string name, uint32_t width, uint32_t height)
    : MenuItem(std::move(name), width, height), on_activate(std::move(callback)) {}

void MenuItemCallable::Draw() const {}

void MenuItemCallable::Activate() { on_activate(); }

MenuItemTest::MenuItemTest(std::shared_ptr<TestSuite> suite, std::string name, uint32_t width, uint32_t height)
    : MenuItem(std::move(name), width, height), suite(std::move(suite)) {}

void MenuItemTest::Draw() const {}

void MenuItemTest::OnEnter() {
  // Blank the screen.
  PrepareDraw(0xFF000000);
  pb_print("Running %s", name.c_str());
  Swap();

  suite->Initialize();
  suite->Run(name);
}

void MenuItemTest::CursorUp() { parent->CursorUpAndActivate(); }

void MenuItemTest::CursorDown() { parent->CursorDownAndActivate(); }

MenuItemSuite::MenuItemSuite(const std::shared_ptr<TestSuite> &suite, uint32_t width, uint32_t height)
    : MenuItem(std::move(suite->Name()), width, height), suite(suite) {
  auto tests = suite->TestNames();
  submenu.reserve(tests.size());

  for (auto &test : tests) {
    auto child = std::make_shared<MenuItemTest>(suite, test, width, height);
    child->parent = this;
    submenu.push_back(child);
  }
}

MenuItemRoot::MenuItemRoot(const std::vector<std::shared_ptr<TestSuite>> &suites, std::function<void()> on_run_all,
                           std::function<void()> on_exit, uint32_t width, uint32_t height)
    : MenuItem("<<root>>", width, height), on_run_all(std::move(on_run_all)), on_exit(std::move(on_exit)) {
  submenu.push_back(std::make_shared<MenuItemCallable>(on_run_all, "Run all and exit", width, height));
  for (auto &suite : suites) {
    auto child = std::make_shared<MenuItemSuite>(suite, width, height);
    child->parent = this;
    submenu.push_back(child);
  }

  start_time = std::chrono::high_resolution_clock::now();
}

void MenuItemRoot::Draw() const {
  if (!timer_cancelled) {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
    if (elapsed > kAutoTestAllTimeoutMilliseconds) {
      on_run_all();
      return;
    }

    char run_all[128] = {0};
    snprintf(run_all, 127, "Run all and exit (automatic in %d ms)", kAutoTestAllTimeoutMilliseconds - elapsed);
    submenu[0]->name = run_all;
  } else {
    submenu[0]->name = "Run all and exit";
  }

  MenuItem::Draw();
}

void MenuItemRoot::Activate() {
  timer_cancelled = true;
  MenuItem::Activate();
}

bool MenuItemRoot::Deactivate() {
  timer_cancelled = true;
  if (!active_submenu) {
    on_exit();
    return false;
  }

  return MenuItem::Deactivate();
}

void MenuItemRoot::CursorUp() {
  timer_cancelled = true;
  MenuItem::CursorUp();
}

void MenuItemRoot::CursorDown() {
  timer_cancelled = true;
  MenuItem::CursorDown();
}

void MenuItemRoot::CursorLeft() {
  timer_cancelled = true;
  MenuItem::CursorLeft();
}

void MenuItemRoot::CursorRight() {
  timer_cancelled = true;
  MenuItem::CursorRight();
}
