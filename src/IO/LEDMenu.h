#pragma once

#include "StatusLed.h"
#include <vector>
#include <functional>
#include <Arduino.h>
#include <map>

enum class LEDMenuState
{
  IDLE,
  ACTIVE
};

enum class LEDMenuItemType
{
  ACTION,
  TOGGLE,
  SELECT,
  SUBMENU,
  BACK
};

enum class LEDMenuItemState
{
  NORMAL,
  EDITING
};

enum class ButtonEventType
{
  SINGLE_CLICK,
  DOUBLE_CLICK,
  TRIPLE_CLICK,
  LONG_CLICK,
  DOUBLE_LONG_CLICK,
  TRIPLE_LONG_CLICK,
};

enum class ButtonType
{
  BOOT,
  PREV,
  SELECT,
  NEXT,
};

struct ButtonEvent
{
  ButtonType button;
  ButtonEventType type;
};

// Enhanced pattern types
enum class LEDPatternType
{
  SOLID,     // Static colors
  BLINK,     // Simple blinking
  PULSE,     // Sine wave brightness modulation
  ALTERNATE, // Alternating between two states
  FLASH,     // Quick flash pattern
  CUSTOM     // Custom pattern function
};

// Enhanced LED pattern structure
struct LEDPattern
{
  LEDPatternType type = LEDPatternType::SOLID;

  // Colors for different states
  uint32_t led1Color = 0x000000;    // Primary color for LED1
  uint32_t led2Color = 0x000000;    // Primary color for LED2
  uint32_t led1AltColor = 0x000000; // Alternate color for LED1 (for ALTERNATE type)
  uint32_t led2AltColor = 0x000000; // Alternate color for LED2 (for ALTERNATE type)

  // Timing parameters
  uint16_t period = 1000;   // Pattern period in ms
  uint16_t onTime = 500;    // On time for blink/flash patterns (ms)
  uint16_t phaseOffset = 0; // Phase offset between LEDs (ms)
  uint8_t blinkCount = 0;   // Number of blinks (0 = continuous)

  // Pulse parameters
  uint8_t minBrightness = 50;  // Minimum brightness for pulse (0-255)
  uint8_t maxBrightness = 255; // Maximum brightness for pulse (0-255)

  // Custom pattern function
  std::function<void(uint32_t time, uint32_t led1Color, uint32_t led2Color)> customFunction;

  // Constructors for backward compatibility and new features
  LEDPattern() = default;

  // Simple solid color constructor
  LEDPattern(uint32_t led1, uint32_t led2)
      : type(LEDPatternType::SOLID), led1Color(led1), led2Color(led2) {}

  // Simple blink constructor (backward compatibility)
  LEDPattern(uint32_t led1, uint32_t led2, uint16_t blinkPeriod, uint8_t count = 0)
      : type(LEDPatternType::BLINK), led1Color(led1), led2Color(led2),
        period(blinkPeriod), onTime(blinkPeriod / 2), blinkCount(count) {}

  // Advanced constructor
  LEDPattern(LEDPatternType patternType, uint32_t led1, uint32_t led2,
             uint16_t patternPeriod = 1000, uint16_t onDuration = 500,
             uint32_t led1Alt = 0x000000, uint32_t led2Alt = 0x000000)
      : type(patternType), led1Color(led1), led2Color(led2),
        led1AltColor(led1Alt), led2AltColor(led2Alt),
        period(patternPeriod), onTime(onDuration) {}

  // Pulse pattern constructor
  static LEDPattern createPulse(uint32_t led1, uint32_t led2, uint16_t pulsePeriod = 1000,
                                uint8_t minBright = 50, uint8_t maxBright = 255)
  {
    LEDPattern p;
    p.type = LEDPatternType::PULSE;
    p.led1Color = led1;
    p.led2Color = led2;
    p.period = pulsePeriod;
    p.minBrightness = minBright;
    p.maxBrightness = maxBright;
    return p;
  }

  // Alternating pattern constructor
  static LEDPattern createAlternating(uint32_t led1Primary, uint32_t led1Alt,
                                      uint32_t led2Primary, uint32_t led2Alt,
                                      uint16_t switchPeriod = 500)
  {
    LEDPattern p;
    p.type = LEDPatternType::ALTERNATE;
    p.led1Color = led1Primary;
    p.led2Color = led2Primary;
    p.led1AltColor = led1Alt;
    p.led2AltColor = led2Alt;
    p.period = switchPeriod * 2; // Full cycle
    p.onTime = switchPeriod;
    return p;
  }

  // Flash pattern constructor
  static LEDPattern createFlash(uint32_t led1, uint32_t led2,
                                uint16_t flashPeriod = 200, uint16_t pausePeriod = 1800)
  {
    LEDPattern p;
    p.type = LEDPatternType::FLASH;
    p.led1Color = led1;
    p.led2Color = led2;
    p.period = flashPeriod + pausePeriod;
    p.onTime = flashPeriod;
    return p;
  }

  // Custom pattern constructor
  static LEDPattern createCustom(std::function<void(uint32_t, uint32_t, uint32_t)> func,
                                 uint32_t led1Base = 0x000000, uint32_t led2Base = 0x000000)
  {
    LEDPattern p;
    p.type = LEDPatternType::CUSTOM;
    p.led1Color = led1Base;
    p.led2Color = led2Base;
    p.customFunction = func;
    return p;
  }
};

// Forward declaration for recursive menu structure
class LEDMenu;

struct LEDMenuItem
{
  String name;
  LEDMenuItemType type;
  LEDPattern pattern;
  LEDPattern editingPattern;
  LEDMenuItemState state = LEDMenuItemState::NORMAL;

  // for TOGGLE
  bool *toggleValuePtr = nullptr;
  // for SELECT
  int *intValuePtr = nullptr;
  int currentValue = 0; // Current editing value
  bool valueModified = false;
  int minValue = 0, maxValue = 0;
  std::vector<String> *options = nullptr;

  // for ACTION and SELECT on-save
  std::function<void()> actionCallback;

  // for SUBMENU
  std::vector<LEDMenuItem> *subMenuItems = nullptr;
  LEDMenu *parentMenu = nullptr;

  // Static factory methods
  static LEDMenuItem createAction(const String &name,
                                  std::function<void()> cb,
                                  const LEDPattern &pattern,
                                  const LEDPattern &editPattern = LEDPattern());
  static LEDMenuItem createToggle(const String &name,
                                  bool *ptr,
                                  const LEDPattern &pattern,
                                  const LEDPattern &editPattern = LEDPattern());
  static LEDMenuItem createSelect(const String &name,
                                  int *ptr,
                                  int minV,
                                  int maxV,
                                  std::vector<String> *opts,
                                  const LEDPattern &pattern,
                                  const LEDPattern &editPattern = LEDPattern(),
                                  std::function<void()> onSave = nullptr);
  static LEDMenuItem createSubMenu(const String &name,
                                   std::vector<LEDMenuItem> *subItems,
                                   const LEDPattern &pattern);
  static LEDMenuItem createBack(const String &name = "Back",
                                const LEDPattern &pattern = LEDPattern());

  // State management
  void startEditing();
  void stopEditing();
  bool isEditing() const { return state == LEDMenuItemState::EDITING; }

  // Value management
  void incrementValue();
  void decrementValue();
  int getCurrentValue() const;
  void setCurrentValue(int v);
  void saveValue();
  void cancelEdit();

  // Pattern management
  LEDPattern getCurrentPattern() const;
};

class LEDMenu
{
public:
  LEDMenu();
  ~LEDMenu();

  void begin();
  void update(); // drive LEDs & auto-exit
  void setEnabled(bool ena);
  bool isEnabled() const;
  void exit();

  void addMenuItem(const LEDMenuItem &item);
  void clearMenuItems();

  // Sub-menu support
  void enterSubMenu(std::vector<LEDMenuItem> *subItems);
  void exitSubMenu();
  bool isInSubMenu() const { return _menuStack.size() > 1; }

  // feed all button events here:
  void handleButtonEvent(const ButtonEvent &ev);

  // default patterns
  static const LEDPattern IDLE_PATTERN;
  static const LEDPattern EDITING_PATTERN;

private:
  struct MenuLevel
  {
    std::vector<LEDMenuItem> *items;
    int currentIndex;

    MenuLevel(std::vector<LEDMenuItem> *menuItems, int index = 0)
        : items(menuItems), currentIndex(index) {}
  };

  bool _enabled = false;
  LEDMenuState _state = LEDMenuState::IDLE;
  std::vector<LEDMenuItem> _rootMenuItems;
  std::vector<MenuLevel> _menuStack;

  // pattern timing and state
  uint32_t _patternTimer = 0;
  uint32_t _patternStartTime = 0;
  bool _patternState = false;
  uint8_t _blinkCounter = 0;

  // auto-exit
  uint32_t _lastActivity = 0;
  static const uint32_t AUTO_EXIT_TIMEOUT = 10000;

  // state transitions
  void _enterMenu();
  void _exitMenu();

  // rendering
  void _updateLEDs();
  void _showPattern(const LEDPattern &p);

  // helper functions for complex patterns
  uint32_t _modulateBrightness(uint32_t color, uint8_t brightness);
  uint8_t _calculatePulseBrightness(uint32_t time, uint16_t period, uint8_t minBright, uint8_t maxBright);

  // menu logic
  void _incrementCurrentItem();
  void _decrementCurrentItem();
  void _executeCurrentItem();

  // Current item access
  LEDMenuItem *_getCurrentItem();
  MenuLevel *_getCurrentLevel();

  // activity/time
  void _resetActivity();
  bool _timedOut() const;

  // event dispatch
  void _onMenuEvent(const ButtonEvent &ev);
  void _onEditingEvent(const ButtonEvent &ev);
};

extern LEDMenu ledMenu;