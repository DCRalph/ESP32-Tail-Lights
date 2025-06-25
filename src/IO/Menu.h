#pragma once

#include "config.h"
#ifdef ENABLE_DISPLAY
#include "Display.h"

#include <vector>

#define MENU_DEFUALT_CLICKS 1

class Menu;

enum class MenuItemType
{
  None,
  Action,
  Navigate,
  Back,
  Toggle,
  String,
  Number,
  Select,
  Submenu
};

enum class MenuSize
{
  Large,
  Medium,
  Small
};

struct ActionFunction
{
  std::function<void()> func;
  int8_t clicksToRun;
};

/**
 * @class MenuItem
 * @brief Represents a menu item.
 */
class MenuItem
{
  friend class Menu;

private:
  String name;

protected:
  Menu *parent;
  MenuItemType type;
  bool hidden = false;

  u16_t textColor;
  u16_t activeTextColor;
  u16_t bgColor;

public:
  std::vector<ActionFunction> functions;
  /**
   * @brief Constructs a MenuItem object with the specified name.
   * @param _name The name of the menu item.
   */
  MenuItem(String _name);

  /**
   * @brief Sets the action to be performed when the menu item is selected.
   *
   * @param _clicksToRun The number of clicks required to run the action.
   * @param _func The action to be performed.
   */
  void addFunc(int8_t _clicksToRun, std::function<void()> _func);

  /**
   * @brief Sets the name of the menu item.
   * @param _name The name of the menu item.
   */
  void setName(String _name);

  /**
   * @brief Gets the name of the menu item.
   * @return The name of the menu item.
   */
  String getName();

  /**
   * @brief Gets the type of the menu item.
   * @return The type of the menu item.
   */
  MenuItemType getType();

  /**
   * @brief Sets the text color of the menu item.
   * @param _color The text color.
   */
  void setTextColor(u16_t _color);

  /**
   * @brief Sets the active text color of the menu item.
   * @param _color The active text color.
   */
  void setActiveTextColor(u16_t _color);

  /**
   * @brief Sets the background color of the menu item.
   * @param _color The background color.
   */
  void setBgColor(u16_t _color);

  /**
   * @brief Sets the visibility of the menu item.
   * @param _hidden Indicates whether the menu item is hidden or not.
   */
  void setHidden(bool _hidden);

  /**
   * @brief Checks if the menu item is hidden.
   * @return true if the menu item is hidden, false otherwise.
   */
  bool isHidden();

  void executeFunc();
  void executeFunc(int8_t _clicks);

  /**
   * @brief Draws the menu item on the screen.
   * @param _x The x-coordinate of the menu item.
   * @param _y The y-coordinate of the menu item.
   * @param _active Indicates whether the menu item is active or not.
   */
  virtual void draw(uint8_t _x, uint8_t _y, bool _active);

  /**
   * @brief Runs the menu item.
   */
  virtual void run();
};

/**
 * @class MenuItemAction
 * @brief A class representing a menu item with an associated action.
 *
 * This class inherits from the base class MenuItem and adds a function pointer
 * to store the action to be performed when the menu item is selected.
 */
class MenuItemAction : public MenuItem
{
public:
  /**
   * @brief Constructs a MenuItemAction object with the specified name and action.
   *
   * @param _name The name of the menu item.
   * @param _clicksToRun The number of clicks required to run the action.
   * @param _func The action to be performed when the menu item is selected.
   */
  MenuItemAction(String _name, int8_t _clicksToRun, std::function<void()> _func);

  // void draw(u8 _x, u8 _y, bool _active) override;
};
class MenuItemNavigate : public MenuItem
{
private:
  // String target;
  const Screen2 *target;

public:
  /**
   * @brief Constructs a new MenuItemNavigate object.
   *
   * @param _name The name of the menu item.
   * @param _target The target of the navigation.
   */
  MenuItemNavigate(String _name, const Screen2 *_target);

  /**
   * Adds a new route to the menu.
   *
   * @param route The route to be added.
   */
  void addRoute(int8_t _clicksToRun, const Screen2 *_target);

  // void draw(u8 _x, u8 _y, bool _active) override;
};

/**
 * @brief Represents a menu item that allows the user to go back to the previous menu.
 */
class MenuItemBack : public MenuItem
{
public:
  /**
   * @brief Constructs a new MenuItemBack object.
   */
  MenuItemBack();

  // void draw(u8 _x, u8 _y, bool _active) override;
};

/**
 * @brief Represents a toggle menu item.
 *
 * This class inherits from the MenuItem class and provides functionality for a toggle menu item.
 * It allows the user to toggle a boolean value associated with the menu item.
 */
class MenuItemToggle : public MenuItem
{
private:
  bool *value;            ///< Pointer to the boolean value associated with the menu item.
  bool isMutable = false; ///< Flag indicating whether this menu item is currently selected.
  std::function<void()> onChange;

public:
  /**
   * @brief Constructs a new MenuItemToggle object.
   *
   * @param _name The name of the menu item.
   * @param _value A pointer to the boolean value associated with the menu item.
   */
  MenuItemToggle(String _name, bool *_value, bool _isMutable = true);

  void setOnChange(std::function<void()> _onChange);
  void removeOnChange();

  void set(bool _value);
  bool get();

  void draw(uint8_t _x, uint8_t _y, bool _active) override;
};

/**
 * @brief Represents a menu item for displaying a string.
 */
class MenuItemString : public MenuItem
{
private:
  String *value;

public:
  MenuItemString(String _name, String *_value);

  void draw(uint8_t _x, uint8_t _y, bool _active) override;
  void setValue(String _value);
  String getValue();
};

enum class NumberValueType
{
  INT,
  FLOAT,
  LONG,
  UINT8_T,
  UINT32_T,
  UNKNOWN
};

/**
 * @brief Abstract base class for MenuItemNumber to provide a common interface.
 */
class MenuItemNumberBase : public MenuItem
{
protected:
  bool selected = false; ///< Flag indicating whether this menu item is currently selected.

  bool isMutable = false; ///< Flag indicating whether this menu item is currently selected.
  bool fastUpdate = false;

public:
  MenuItemNumberBase(String _name);

  virtual ~MenuItemNumberBase() = default;

  // Pure virtual methods to be implemented by derived classes
  bool isSelected() const;
  virtual void increase() = 0;
  virtual void decrease() = 0;
  virtual NumberValueType getValueType() const = 0;

  void setFastUpdate(bool _fastUpdate);
  bool isFastUpdate();
};

/**
 * @brief Represents a menu item for selecting a number value.
 */
template <typename T>
class MenuItemNumber : public MenuItemNumberBase
{
private:
  T *value;   ///< Pointer to the value being controlled by this menu item.
  T min;      ///< The minimum value allowed.
  T max;      ///< The maximum value allowed.
  T step = 1; ///< The step size for increasing or decreasing the value.

  NumberValueType valueType; ///< The type of the value being controlled by this menu item.

  std::function<void()> onChange; ///< Optional callback when the value changes.

  void determineValueType();

public:
  // Constructors
  MenuItemNumber(String _name, T *_value, T _min, T _max);
  MenuItemNumber(String _name, T *_value, T _min, T _max, T _step);
  MenuItemNumber(String _name, T *_value);

  // Override virtual methods
  void toggleSelected();
  virtual void increase() override;
  virtual void decrease() override;
  virtual NumberValueType getValueType() const override;
  virtual void draw(uint8_t _x, uint8_t _y, bool _active) override;

  // Additional methods
  void setOnChange(std::function<void()> _onChange);
  void removeOnChange();
};

// /**
//  * @brief Represents a menu item for selecting a number value.
//  */
// template <typename T>
// class MenuItemNumber : public MenuItem
// {
// private:
//   T *value;   ///< Pointer to the value being controlled by this menu item.
//   T min;      ///< The minimum value allowed.
//   T max;      ///< The maximum value allowed.
//   T step = 1; ///< The step size for increasing or decreasing the value.

//   NumberValueType valueType; ///< The type of the value being controlled by this menu item.

//   bool isMutable = false; ///< Flag indicating whether this menu item is currently selected.

//   bool selected = false; ///< Flag indicating whether this menu item is currently selected.

//   std::function<void()> onChange;

// public:
//   /**
//    * @brief Constructs a new MenuItemNumber object.
//    *
//    * @param _name The name of the menu item.
//    * @param _value Pointer to the value being controlled by this menu item.
//    * @param _min The minimum value allowed.
//    * @param _max The maximum value allowed.
//    */
//   MenuItemNumber(String _name, T *_value, T _min, T _max);
//   MenuItemNumber(String _name, T *_value, T _min, T _max, T _step);

//   /**
//    * @brief Constructs a new MenuItemNumber object.
//    *
//    * @param _name The name of the menu item.
//    * @param _value Pointer to the value being controlled by this menu item.
//    */
//   MenuItemNumber(String _name, T *_value);

//   NumberValueType getValueType() const;

//   void setOnChange(std::function<void()> _onChange);
//   void removeOnChange();

//   /**
//    * @brief Checks if this menu item is currently selected.
//    *
//    * @return true if this menu item is selected, false otherwise.
//    */
//   bool isSelected();

//   /**
//    * @brief Increases the value by one.
//    */
//   void increase();

//   /**
//    * @brief Decreases the value by one.
//    */
//   void decrease();

//   void draw(uint8_t _x, uint8_t _y, bool _active) override;
// };

template class MenuItemNumber<int>;
template class MenuItemNumber<float>;
template class MenuItemNumber<long>;
template class MenuItemNumber<uint8_t>;
template class MenuItemNumber<uint16_t>;
template class MenuItemNumber<uint32_t>;
template class MenuItemNumber<uint64_t>;

// ##############################
// MenuItemSubmenu declaration
// ##############################
class MenuItemSubmenu : public MenuItem
{
private:
  Menu *submenu;

public:
  /**
   * @brief Constructs a MenuItemSubmenu.
   *
   * @param _name The display name of the submenu item.
   * @param _submenu Pointer to the submenu to display when clicked.
   */
  MenuItemSubmenu(String _name, Menu *_submenu);

  /// Override run to activate the submenu.
  virtual void run() override;
};

// ##############################
// New MenuItemSelect declaration
// ##############################
class MenuItemSelect : public MenuItem
{
private:
  std::vector<String> options;
  int currentIndex = 0;
  bool selected = false; // Indicates if the item is in "editing" mode

  // Optional callback: pass the new selection string
  std::function<void()> onChange;

public:
  /**
   * @brief Constructs a MenuItemSelect.
   *
   * @param _name The display name of the item.
   * @param _options A vector of available option strings.
   * @param initialIndex The initially selected index in _options.
   */
  MenuItemSelect(String _name, const std::vector<String> &_options, int initialIndex = 0);

  /// Cycle to the next option in the list.
  void nextOption();

  /// Cycle to the previous option in the list.
  void prevOption();

  /// Returns the currently selected option.
  String getSelectedOption() const;

  void setCurrentIndex(int _index);
  int getCurrentIndex() const;

  /// Set a callback to be notified when the selection changes.
  void setOnChange(std::function<void()> callback);

  /// Toggle the editing mode
  void toggleSelected();

  /// Returns whether the item is currently in editing mode.
  bool isSelected() const;

  /// Override run so that a click toggles between edit and normal mode.
  virtual void run() override;

  /// Draw the selection item. It draws the name and the currently selected option.
  virtual void draw(uint8_t _x, uint8_t _y, bool _active) override;
};

// ###### Menu ######
class Menu
{
  friend class MenuItem;

private:
  uint8_t active;

  uint8_t maxItemsPerPage = 0;

  uint8_t numItems;
  uint8_t numItemsPerPage;
  uint8_t offsetFromTop;
  uint8_t topItem = 0; // Persistent scroll position

  MenuSize menuSize = MenuSize::Large;
  Menu *activeSubmenu = nullptr; // Currently active submenu
  Menu *parentMenu = nullptr;    // Parent menu (for back navigation)

public:
  std::vector<MenuItem *> items;
  Menu();
  Menu(MenuSize _size);

  // String name;

  void setItemsPerPage(uint8_t _itemsPerPage);
  uint8_t getItemsPerPage();

  void setMenuSize(MenuSize _size);
  MenuSize getMenuSize() const;

  void setActive(uint8_t _active);
  uint8_t getActive();

  void nextItem();
  void prevItem();

  void setActiveSubmenu(Menu *submenu);
  void clearActiveSubmenu();
  Menu *getActiveSubmenu();
  void setParentMenu(Menu *parent);
  Menu *getParentMenu();

  void addMenuItem(MenuItem *_item);
  void draw();
  void update();
};

#endif