#ifndef NXDK_PGRAPH_TESTS_MENU_ITEM_H
#define NXDK_PGRAPH_TESTS_MENU_ITEM_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class TestSuite;

struct MenuItem {
 public:
  MenuItem(std::string name, uint32_t width, uint32_t height) : width(width), height(height), name(std::move(name)) {}

  // Whether or not this menu item becomes the active drawable when activated.
  virtual bool IsEnterable() const { return !submenu.empty(); }

  virtual void Draw() const;

  // Invoked when this MenuItem becomes the active drawable.
  virtual void OnEnter();

  // Invoked when the user activates this MenuItem.
  virtual void Activate();

  virtual bool Deactivate();
  virtual void CursorUp();
  virtual void CursorDown();
  virtual void CursorLeft();
  virtual void CursorRight();

 protected:
  void PrepareDraw(uint32_t background_color = 0xFF3E003E) const;
  static void Swap();

 public:
  uint32_t width;
  uint32_t height;
  std::string name;

  uint32_t cursor_position{0};
  std::vector<std::shared_ptr<MenuItem>> submenu{};
  std::shared_ptr<MenuItem> active_submenu{};
};

struct MenuItemCallable : public MenuItem {
  MenuItemCallable(std::function<void()> on_activate, std::string name, uint32_t width, uint32_t height);

  void Draw() const override;

  void Activate() override;

  std::function<void()> on_activate;
};

struct MenuItemTest : public MenuItem {
  MenuItemTest(std::shared_ptr<TestSuite> suite, std::string name, uint32_t width, uint32_t height);

  bool IsEnterable() const override { return true; }

  void Draw() const override;
  void OnEnter() override;
  void Activate() override {}

  std::shared_ptr<TestSuite> suite;
};

struct MenuItemSuite : public MenuItem {
  explicit MenuItemSuite(const std::shared_ptr<TestSuite>& suite, uint32_t width, uint32_t height);
  std::shared_ptr<TestSuite> suite;
};

struct MenuItemRoot : public MenuItem {
  explicit MenuItemRoot(const std::vector<std::shared_ptr<TestSuite>>& suites, std::function<void()> on_run_all,
                        std::function<void()> on_exit, uint32_t width, uint32_t height);

  bool Deactivate() override;

  std::function<void()> on_run_all;
  std::function<void()> on_exit;
};

#endif  // NXDK_PGRAPH_TESTS_MENU_ITEM_H
