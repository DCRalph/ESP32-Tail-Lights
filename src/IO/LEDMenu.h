#pragma once

#include "config.h"
#include "StatusLed.h"
#include "GPIO.h"
#include <vector>
#include <functional>

// Forward declaration
class Application;

enum class LEDMenuState
{
  IDLE,         // Not in menu mode
  MAIN_MENU,    // In main menu, navigating items
  EDITING_VALUE // Editing a specific menu item value
};

enum class LEDMenuItemType
{
  ACTION, // Execute an action
  TOGGLE, // Boolean on/off
  SELECT, // Multiple choice selection
  NUMBER  // Numeric value
};

struct LEDMenuItem
{
  String name;
  LEDMenuItemType type;

  // Separate fields for different item types (instead of union)
  // Toggle data
  bool *toggleValuePtr;

  // Select/Number data
  int *intValuePtr;
  int minValue;
  int maxValue;
  int step;
  std::vector<String> *options;

  // Action data
  std::function<void()> actionCallback;

  // Constructors
  LEDMenuItem();
  LEDMenuItem(const LEDMenuItem &other);
  LEDMenuItem &operator=(const LEDMenuItem &other);
  ~LEDMenuItem();

  // Constructor helpers
  static LEDMenuItem createToggle(const String &name, bool *valuePtr);
  static LEDMenuItem createSelect(const String &name, int *valuePtr, int minValue, int maxValue, std::vector<String> *options);
  static LEDMenuItem createNumber(const String &name, int *valuePtr, int minValue, int maxValue, int step = 1);
  static LEDMenuItem createAction(const String &name, std::function<void()> callback);
};

class LEDMenu
{
public:
  LEDMenu();
  ~LEDMenu();

  void begin();
  void update();

  void setEnabled(bool enabled);
  bool isEnabled() const;

  void addMenuItem(const LEDMenuItem &item);
  void clearMenuItems();

private:
  bool _enabled;
  LEDMenuState _state;
  std::vector<LEDMenuItem> _menuItems;

  int _currentMenuItem;
  int _currentValue;
  bool _valueModified;

  // Button state tracking
  unsigned long _lastButtonUpdate;
  static const unsigned long BUTTON_UPDATE_INTERVAL = 50; // 20Hz

  // LED feedback timing
  unsigned long _lastLEDUpdate;
  unsigned long _ledBlinkTimer;
  bool _ledBlinkState;
  static const unsigned long LED_BLINK_INTERVAL = 500;

  // Auto-exit timeout
  unsigned long _lastActivity;
  static const unsigned long AUTO_EXIT_TIMEOUT = 10000; // 10 seconds

  void _updateButtons();
  void _updateLEDs();
  void _handleMainMenuButtons();
  void _handleEditingButtons();

  void _enterMenu();
  void _exitMenu();
  void _enterEditing();
  void _exitEditing();

  void _showMenuPosition();
  void _showEditingValue();
  void _showIdleState();

  void _executeCurrentItem();
  void _incrementCurrentItem();
  void _decrementCurrentItem();

  int _getCurrentItemValue();
  void _setCurrentItemValue(int value);
  String _getCurrentItemDisplayValue();

  void _resetActivity();
  bool _isActivityTimedOut();

  // Color definitions for different states
  static const uint32_t COLOR_IDLE = 0x000020;      // Dim blue
  static const uint32_t COLOR_MENU_POS = 0x002000;  // Green for menu position
  static const uint32_t COLOR_EDITING = 0x200020;   // Purple for editing mode
  static const uint32_t COLOR_VALUE_ON = 0x002000;  // Green for ON/active values
  static const uint32_t COLOR_VALUE_OFF = 0x200000; // Red for OFF/inactive values
  static const uint32_t COLOR_NUMBER = 0x202000;    // Yellow for numbers
};

extern LEDMenu ledMenu;