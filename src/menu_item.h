#ifndef NXDK_PGRAPH_TESTS_MENU_ITEM_H
#define NXDK_PGRAPH_TESTS_MENU_ITEM_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class TestSuite;

struct MenuItem {
 public:
  MenuItem(std::string name, uint32_t width, uint32_t height) : width(width), height(height), name(std::move(name)) {}
  virtual ~MenuItem() = default;

  static void SetBackgroundColor(uint32_t background_color);

  // Whether or not this menu item becomes the active drawable when activated.
  [[nodiscard]] virtual bool IsEnterable() const { return !submenu.empty(); }

  virtual void Draw();

  // Invoked when this MenuItem becomes the active drawable.
  virtual void OnEnter();

  // Invoked when the user activates this MenuItem.
  virtual void Activate();

  virtual void ActivateCurrentSuite();

  virtual bool Deactivate();

  virtual void CursorUp(bool is_repeat);
  virtual void CursorDown(bool is_repeat);
  virtual void CursorLeft(bool is_repeat);
  virtual void CursorRight(bool is_repeat);

  void CursorUpAndActivate();
  void CursorDownAndActivate();

 protected:
  void PrepareDraw(uint32_t background_color) const;
  static void Swap();

 public:
  uint32_t width;
  uint32_t height;
  std::string name;

  static uint32_t menu_background_color_;

  uint32_t cursor_position{0};
  std::vector<std::shared_ptr<MenuItem>> submenu{};
  std::shared_ptr<MenuItem> active_submenu{};
  MenuItem* parent{nullptr};
};

struct MenuItemCallable : public MenuItem {
  MenuItemCallable(std::function<void()> on_activate, std::string name, uint32_t width, uint32_t height);

  void Draw() override;

  void Activate() override;
  void ActivateCurrentSuite() override {}
  std::function<void()> on_activate;
};

struct MenuItemTest : public MenuItem {
  static bool one_shot_mode_;

  MenuItemTest(std::shared_ptr<TestSuite> suite, std::string name, uint32_t width, uint32_t height);

  static void SetOneShotMode(bool val) { one_shot_mode_ = val; }

  [[nodiscard]] bool IsEnterable() const override { return true; }

  void Draw() override;
  void OnEnter() override;
  void Activate() override { OnEnter(); }
  bool Deactivate() override;
  void ActivateCurrentSuite() override {}
  void CursorUp(bool is_repeat) override;
  void CursorDown(bool is_repeat) override;
  void CursorLeft(bool is_repeat) override {}
  void CursorRight(bool is_repeat) override {}

  std::shared_ptr<TestSuite> suite;
  bool has_run_once_{false};
};

struct MenuItemSuite : public MenuItem {
  explicit MenuItemSuite(const std::shared_ptr<TestSuite>& suite, uint32_t width, uint32_t height);

  void ActivateCurrentSuite() override;
  std::shared_ptr<TestSuite> suite;
};

struct MenuItemRoot : public MenuItem {
  explicit MenuItemRoot(const std::vector<std::shared_ptr<TestSuite>>& suites, std::function<void()> on_run_all,
                        std::function<void()> on_exit, uint32_t width, uint32_t height, bool disable_autorun,
                        bool autorun_immediately);

  void Draw() override;
  void Activate() override;
  void ActivateCurrentSuite() override;
  bool Deactivate() override;
  void CursorUp(bool is_repeat) override;
  void CursorDown(bool is_repeat) override;
  void CursorLeft(bool is_repeat) override;
  void CursorRight(bool is_repeat) override;

  std::function<void()> on_run_all;
  std::function<void()> on_exit;
  std::chrono::steady_clock::time_point start_time;
  bool timer_valid{false};
  bool timer_cancelled{false};

 private:
  bool disable_autorun_;
  bool autorun_immediately_;
};

struct MenuItemOptions : public MenuItem {
  MenuItemOptions(const std::vector<std::shared_ptr<TestSuite>>& suites, std::function<void()> on_exit, uint32_t width,
                  uint32_t height);

  void Draw() override;
  void Activate() override;
  void ActivateCurrentSuite() override;
  bool Deactivate() override;
  void CursorUp(bool is_repeat) override;
  void CursorDown(bool is_repeat) override;
  void CursorLeft(bool is_repeat) override;
  void CursorRight(bool is_repeat) override;

  std::function<void()> on_exit;
  std::chrono::steady_clock::time_point start_time;
  bool timer_valid{false};
  bool timer_cancelled{false};
};

#endif  // NXDK_PGRAPH_TESTS_MENU_ITEM_H
