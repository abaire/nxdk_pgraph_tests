#include "menu_item.h"

#include <pbkit/pbkit.h>

#include <chrono>
#include <memory>
#include <utility>

#include "configure.h"
#include "debug_output.h"
#include "pushbuffer.h"
#include "tests/test_suite.h"

static constexpr uint32_t kAutoTestAllTimeoutMilliseconds = 3000;
static constexpr uint32_t kNumItemsPerPage = 12;
static constexpr uint32_t kNumItemsPerHalfPage = kNumItemsPerPage >> 1;

uint32_t MenuItem::menu_background_color_ = 0xFF3E003E;
bool MenuItemTest::one_shot_mode_ = true;

void MenuItem::PrepareDraw(uint32_t background_color) const {
  pb_wait_for_vbl();
  pb_target_back_buffer();
  Pushbuffer::Flush();
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

void MenuItem::Draw() {
  if (active_submenu) {
    active_submenu->Draw();
    return;
  }

  PrepareDraw(menu_background_color_);

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

void MenuItem::ActivateCurrentSuite() {
  if (active_submenu) {
    active_submenu->ActivateCurrentSuite();
    return;
  }
  auto activated_item = submenu[cursor_position];
  activated_item->ActivateCurrentSuite();
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

void MenuItem::CursorUp(bool is_repeat) {
  if (active_submenu) {
    active_submenu->CursorUp(is_repeat);
    return;
  }

  if (cursor_position > 0) {
    --cursor_position;
  } else {
    cursor_position = submenu.size() - 1;
  }
}

void MenuItem::CursorDown(bool is_repeat) {
  if (active_submenu) {
    active_submenu->CursorDown(is_repeat);
    return;
  }

  if (cursor_position < submenu.size() - 1) {
    ++cursor_position;
  } else {
    cursor_position = 0;
  }
}

void MenuItem::CursorLeft(bool is_repeat) {
  if (active_submenu) {
    active_submenu->CursorLeft(is_repeat);
    return;
  }

  if (cursor_position > kNumItemsPerHalfPage) {
    cursor_position -= kNumItemsPerHalfPage;
  } else {
    cursor_position = 0;
  }
}

void MenuItem::CursorRight(bool is_repeat) {
  if (active_submenu) {
    active_submenu->CursorRight(is_repeat);
    return;
  }

  cursor_position += kNumItemsPerHalfPage;
  if (cursor_position >= submenu.size()) {
    cursor_position = submenu.size() - 1;
  }
}

void MenuItem::CursorUpAndActivate() {
  active_submenu->Deactivate();
  active_submenu = nullptr;
  CursorUp(false);
  Activate();
}

void MenuItem::CursorDownAndActivate() {
  active_submenu->Deactivate();
  active_submenu = nullptr;
  CursorDown(false);
  Activate();
}

void MenuItem::SetBackgroundColor(uint32_t background_color) { menu_background_color_ = background_color; }

MenuItemCallable::MenuItemCallable(std::function<void()> callback, std::string name, uint32_t width, uint32_t height)
    : MenuItem(std::move(name), width, height), on_activate(std::move(callback)) {}

void MenuItemCallable::Draw() {}

void MenuItemCallable::Activate() { on_activate(); }

MenuItemTest::MenuItemTest(std::shared_ptr<TestSuite> suite, std::string name, uint32_t width, uint32_t height)
    : MenuItem(std::move(name), width, height), suite(std::move(suite)) {}

void MenuItemTest::Draw() {
  if (one_shot_mode_ && has_run_once_) {
    return;
  }

  suite->Run(name);
  suite->SetSavingAllowed(false);
  has_run_once_ = true;
}

void MenuItemTest::OnEnter() {
  // Blank the screen.
  PrepareDraw(0xFF000000);
  pb_print("Running %s", name.c_str());
  Swap();

  if (has_run_once_) {
    suite->Deinitialize();
  }
  suite->Initialize();
  suite->SetSavingAllowed(true);
  has_run_once_ = false;
}

bool MenuItemTest::Deactivate() {
  suite->Deinitialize();
  has_run_once_ = false;
  return MenuItem::Deactivate();
}

void MenuItemTest::CursorUp(bool is_repeat) {
  if (!is_repeat) {
    parent->CursorUpAndActivate();
  }
}

void MenuItemTest::CursorDown(bool is_repeat) {
  if (!is_repeat) {
    parent->CursorDownAndActivate();
  }
}

MenuItemSuite::MenuItemSuite(const std::shared_ptr<TestSuite> &suite, uint32_t width, uint32_t height)
    : MenuItem(suite->Name(), width, height), suite(suite) {
  auto tests = suite->TestNames();
  submenu.reserve(tests.size());

  for (auto &test : tests) {
    auto child = std::make_shared<MenuItemTest>(suite, test, width, height);
    child->parent = this;
    submenu.push_back(child);
  }
}

void MenuItemSuite::ActivateCurrentSuite() {
  suite->Initialize();
  suite->SetSavingAllowed(true);
  suite->RunAll();
  suite->Deinitialize();
  MenuItem::Deactivate();
}

MenuItemRoot::MenuItemRoot(const std::vector<std::shared_ptr<TestSuite>> &suites, std::function<void()> on_run_all,
                           std::function<void()> on_exit, uint32_t width, uint32_t height, bool disable_autorun,
                           bool autorun_immediately)
    : MenuItem("<<root>>", width, height),
      on_run_all(std::move(on_run_all)),
      on_exit(std::move(on_exit)),
      disable_autorun_(disable_autorun),
      autorun_immediately_(autorun_immediately) {
  if (!disable_autorun) {
    submenu.push_back(std::make_shared<MenuItemCallable>(on_run_all, "Run all and exit", width, height));
  }

  for (auto &suite : suites) {
    auto child = std::make_shared<MenuItemSuite>(suite, width, height);
    child->parent = this;
    submenu.push_back(child);
  }

  if (disable_autorun) {
    submenu.push_back(std::make_shared<MenuItemCallable>(on_run_all, "! Run all and exit", width, height));
  }
}

void MenuItemRoot::ActivateCurrentSuite() {
  timer_cancelled = true;
  MenuItem::ActivateCurrentSuite();
}

void MenuItemRoot::Draw() {
  if (!timer_valid) {
    start_time = std::chrono::high_resolution_clock::now();
    timer_valid = true;
  }

  if (!disable_autorun_) {
    if (!timer_cancelled) {
      auto now = std::chrono::high_resolution_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();

      if (autorun_immediately_ || elapsed > kAutoTestAllTimeoutMilliseconds) {
        on_run_all();
        return;
      }

      char run_all[128] = {0};
      snprintf(run_all, 127, "Run all and exit (automatic in %d ms)", kAutoTestAllTimeoutMilliseconds - elapsed);
      submenu[0]->name = run_all;
    } else {
      submenu[0]->name = "Run all and exit";
    }
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

void MenuItemRoot::CursorUp(bool is_repeat) {
  timer_cancelled = true;
  MenuItem::CursorUp(is_repeat);
}

void MenuItemRoot::CursorDown(bool is_repeat) {
  timer_cancelled = true;
  MenuItem::CursorDown(is_repeat);
}

void MenuItemRoot::CursorLeft(bool is_repeat) {
  timer_cancelled = true;
  MenuItem::CursorLeft(is_repeat);
}

void MenuItemRoot::CursorRight(bool is_repeat) {
  timer_cancelled = true;
  MenuItem::CursorRight(is_repeat);
}

struct MenuItemOption : public MenuItem {
  MenuItemOption(const std::string &name, std::function<void(const MenuItemOption &)> on_apply);
  MenuItemOption(const std::string &label, const std::vector<std::string> &values,
                 std::function<void(const MenuItemOption &)> on_apply);

  inline void UpdateName() { name = label + ": " + values[current_option]; }

  void Activate() override;
  void CursorLeft(bool is_repeat) override;
  void CursorRight(bool is_repeat) override;

  std::string label;
  std::vector<std::string> values;
  uint32_t current_option = 0;

  std::function<void(const MenuItemOption &)> on_apply;
};

MenuItemOption::MenuItemOption(const std::string &name, std::function<void(const MenuItemOption &)> on_apply)
    : MenuItem(name, 0, 0), on_apply(on_apply) {}

MenuItemOption::MenuItemOption(const std::string &label, const std::vector<std::string> &values,
                               std::function<void(const MenuItemOption &)> on_apply)
    : MenuItem("", 0, 0), label(label), values(values), on_apply(on_apply) {
  ASSERT(!values.empty())
  UpdateName();
}

void MenuItemOption::Activate() {
  current_option = (current_option + 1) % values.size();
  UpdateName();
}

void MenuItemOption::CursorLeft(bool is_repeat) {
  current_option = (current_option - 1) % values.size();
  UpdateName();
}

void MenuItemOption::CursorRight(bool is_repeat) { Activate(); }

MenuItemOptions::MenuItemOptions(const std::vector<std::shared_ptr<TestSuite>> &suites, std::function<void()> on_exit,
                                 uint32_t width, uint32_t height)
    : MenuItem("<<options>>", width, height), on_exit(std::move(on_exit)) {
  submenu.push_back(
      std::make_shared<MenuItemOption>("Accept", [this](const MenuItemOption &_ignored) { this->on_exit(); }));
}

void MenuItemOptions::Draw() {
  if (!timer_valid) {
    start_time = std::chrono::high_resolution_clock::now();
    timer_valid = true;
  }
  if (!timer_cancelled) {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
    if (elapsed > kAutoTestAllTimeoutMilliseconds) {
      cursor_position = 0;
      Activate();
      return;
    }

    char run_all[128] = {0};
    snprintf(run_all, 127, "Accept (automatic in %d ms)", kAutoTestAllTimeoutMilliseconds - elapsed);
    submenu[0]->name = run_all;
  } else {
    submenu[0]->name = "Accept";
  }

  MenuItem::Draw();
}

void MenuItemOptions::Activate() {
  timer_cancelled = true;
  if (cursor_position == 0) {
    for (auto i = 1; i < submenu.size(); ++i) {
      const auto &item = *(reinterpret_cast<MenuItemOption *>(submenu[i].get()));
      item.on_apply(item);
    }
    on_exit();
    return;
  }
  MenuItem::Activate();
}
void MenuItemOptions::ActivateCurrentSuite() {
  timer_cancelled = true;
  MenuItem::ActivateCurrentSuite();
}

bool MenuItemOptions::Deactivate() {
  timer_cancelled = true;
  on_exit();
  return false;
}

void MenuItemOptions::CursorUp(bool is_repeat) {
  timer_cancelled = true;
  MenuItem::CursorUp(is_repeat);
}

void MenuItemOptions::CursorDown(bool is_repeat) {
  timer_cancelled = true;
  MenuItem::CursorDown(is_repeat);
}

void MenuItemOptions::CursorLeft(bool is_repeat) {
  timer_cancelled = true;
  if (cursor_position > 0) {
    submenu[cursor_position]->CursorLeft(is_repeat);
  }
}

void MenuItemOptions::CursorRight(bool is_repeat) {
  timer_cancelled = true;
  if (cursor_position > 0) {
    submenu[cursor_position]->CursorRight(is_repeat);
  }
}
