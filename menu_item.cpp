#include "menu_item.h"

#include <pbkit/pbkit.h>

#include <utility>

#include "tests/test_suite.h"

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

  for (auto i = 0; i < submenu.size(); ++i) {
    const char *prefix = i == cursor_position ? cursor_prefix : normal_prefix;
    pb_print("%s%s\n", prefix, submenu[i]->name.c_str());
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
}

void MenuItem::CursorRight() {
  if (active_submenu) {
    active_submenu->CursorRight();
    return;
  }
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
  Swap();

  suite->Initialize();
  suite->Run(name);
}

MenuItemSuite::MenuItemSuite(const std::shared_ptr<TestSuite> &suite, uint32_t width, uint32_t height)
    : MenuItem(std::move(suite->Name()), width, height), suite(suite) {
  auto tests = suite->TestNames();
  submenu.reserve(tests.size());

  for (auto &test : tests) {
    submenu.push_back(std::make_shared<MenuItemTest>(suite, test, width, height));
  }
}

MenuItemRoot::MenuItemRoot(const std::vector<std::shared_ptr<TestSuite>> &suites, std::function<void()> on_run_all,
                           std::function<void()> on_exit, uint32_t width, uint32_t height)
    : MenuItem("<<root>>", width, height), on_run_all(std::move(on_run_all)), on_exit(std::move(on_exit)) {
  submenu.push_back(std::make_shared<MenuItemCallable>(on_run_all, "Run all and exit", width, height));
  for (auto &suite : suites) {
    submenu.push_back(std::make_shared<MenuItemSuite>(suite, width, height));
  }
}

bool MenuItemRoot::Deactivate() {
  if (!active_submenu) {
    on_exit();
    return false;
  }

  return MenuItem::Deactivate();
}
