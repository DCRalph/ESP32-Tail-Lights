#include "LEDMenu.h"
#include "Application.h"
#include "esp_log.h"

static const char *TAG = "LEDMenu";

// Global instance
LEDMenu ledMenu;

// LEDMenuItem constructors
LEDMenuItem::LEDMenuItem()
{
  type = LEDMenuItemType::ACTION;
  toggleValuePtr = nullptr;
  intValuePtr = nullptr;
  minValue = 0;
  maxValue = 0;
  step = 1;
  options = nullptr;
}

LEDMenuItem::LEDMenuItem(const LEDMenuItem &other)
{
  name = other.name;
  type = other.type;
  toggleValuePtr = other.toggleValuePtr;
  intValuePtr = other.intValuePtr;
  minValue = other.minValue;
  maxValue = other.maxValue;
  step = other.step;
  options = other.options;
  actionCallback = other.actionCallback;
}

LEDMenuItem &LEDMenuItem::operator=(const LEDMenuItem &other)
{
  if (this != &other)
  {
    name = other.name;
    type = other.type;
    toggleValuePtr = other.toggleValuePtr;
    intValuePtr = other.intValuePtr;
    minValue = other.minValue;
    maxValue = other.maxValue;
    step = other.step;
    options = other.options;
    actionCallback = other.actionCallback;
  }
  return *this;
}

LEDMenuItem::~LEDMenuItem()
{
  // Nothing to clean up - we don't own the pointers
}

// LEDMenuItem static factory methods
LEDMenuItem LEDMenuItem::createToggle(const String &name, bool *valuePtr)
{
  LEDMenuItem item;
  item.name = name;
  item.type = LEDMenuItemType::TOGGLE;
  item.toggleValuePtr = valuePtr;
  return item;
}

LEDMenuItem LEDMenuItem::createSelect(const String &name, int *valuePtr, int minValue, int maxValue, std::vector<String> *options)
{
  LEDMenuItem item;
  item.name = name;
  item.type = LEDMenuItemType::SELECT;
  item.intValuePtr = valuePtr;
  item.minValue = minValue;
  item.maxValue = maxValue;
  item.options = options;
  return item;
}

LEDMenuItem LEDMenuItem::createNumber(const String &name, int *valuePtr, int minValue, int maxValue, int step)
{
  LEDMenuItem item;
  item.name = name;
  item.type = LEDMenuItemType::NUMBER;
  item.intValuePtr = valuePtr;
  item.minValue = minValue;
  item.maxValue = maxValue;
  item.step = step;
  return item;
}

LEDMenuItem LEDMenuItem::createAction(const String &name, std::function<void()> callback)
{
  LEDMenuItem item;
  item.name = name;
  item.type = LEDMenuItemType::ACTION;
  item.actionCallback = callback;
  return item;
}

// LEDMenu implementation
LEDMenu::LEDMenu()
{
  _enabled = false;
  _state = LEDMenuState::IDLE;
  _currentMenuItem = 0;
  _currentValue = 0;
  _valueModified = false;

  _lastButtonUpdate = 0;
  _lastLEDUpdate = 0;
  _ledBlinkTimer = 0;
  _ledBlinkState = false;
  _lastActivity = 0;
}

LEDMenu::~LEDMenu()
{
  clearMenuItems();
}

void LEDMenu::begin()
{
  ESP_LOGI(TAG, "LED Menu initialized");

  // Set initial LED state
  _showIdleState();
}

void LEDMenu::update()
{
  if (!_enabled)
    return;

  unsigned long currentTime = millis();

  // Update buttons at 20Hz
  if (currentTime - _lastButtonUpdate >= BUTTON_UPDATE_INTERVAL)
  {
    _lastButtonUpdate = currentTime;
    _updateButtons();
  }

  // Update LEDs
  _updateLEDs();

  // Check for auto-exit timeout
  if (_state != LEDMenuState::IDLE && _isActivityTimedOut())
  {
    ESP_LOGI(TAG, "Menu auto-exit due to inactivity");
    _exitMenu();
  }
}

void LEDMenu::setEnabled(bool enabled)
{
  if (_enabled != enabled)
  {
    _enabled = enabled;

    if (_enabled)
    {
      ESP_LOGI(TAG, "LED Menu enabled");
      _showIdleState();
    }
    else
    {
      ESP_LOGI(TAG, "LED Menu disabled");
      _exitMenu();
    }
  }
}

bool LEDMenu::isEnabled() const
{
  return _enabled;
}

void LEDMenu::addMenuItem(const LEDMenuItem &item)
{
  _menuItems.push_back(item);
  ESP_LOGD(TAG, "Added menu item: %s", item.name.c_str());
}

void LEDMenu::clearMenuItems()
{
  _menuItems.clear();
  _currentMenuItem = 0;
}

void LEDMenu::_updateButtons()
{
  // Update button states
  BtnBoot.Update();
  BtnPrev.Update();
  BtnSel.Update();
  BtnNext.Update();

  switch (_state)
  {
  case LEDMenuState::IDLE:
    // Long press SELECT to enter menu
    if (BtnSel.clicks == -1)
    {
      BtnSel.clicks = 0;
      _enterMenu();
    }
    break;

  case LEDMenuState::MAIN_MENU:
    _handleMainMenuButtons();
    break;

  case LEDMenuState::EDITING_VALUE:
    _handleEditingButtons();
    break;
  }
}

void LEDMenu::_handleMainMenuButtons()
{
  // BOOT button - exit menu
  if (BtnBoot.clicks != 0)
  {
    BtnBoot.clicks = 0;
    _exitMenu();
    return;
  }

  // NEXT button - next menu item
  if (BtnNext.clicks == 1)
  {
    BtnNext.clicks = 0;
    _incrementCurrentItem();
    _resetActivity();
  }

  // PREV button - previous menu item
  if (BtnPrev.clicks == 1)
  {
    BtnPrev.clicks = 0;
    _decrementCurrentItem();
    _resetActivity();
  }

  // SEL button - enter editing or execute action
  if (BtnSel.clicks == 1)
  {
    BtnSel.clicks = 0;
    _executeCurrentItem();
    _resetActivity();
  }
}

void LEDMenu::_handleEditingButtons()
{
  // BOOT button - exit editing without saving
  if (BtnBoot.clicks != 0)
  {
    BtnBoot.clicks = 0;
    _exitEditing();
    return;
  }

  // SEL button - save and exit editing
  if (BtnSel.clicks == 1)
  {
    BtnSel.clicks = 0;

    if (_valueModified)
    {
      _setCurrentItemValue(_currentValue);
      ESP_LOGI(TAG, "Saved value: %d", _currentValue);
    }

    _exitEditing();
    return;
  }

  // NEXT button - increase value
  if (BtnNext.clicks == 1)
  {
    BtnNext.clicks = 0;

    const LEDMenuItem &item = _menuItems[_currentMenuItem];

    switch (item.type)
    {
    case LEDMenuItemType::TOGGLE:
      _currentValue = !_currentValue;
      _valueModified = true;
      break;

    case LEDMenuItemType::SELECT:
      if (_currentValue < item.maxValue)
      {
        _currentValue++;
        _valueModified = true;
      }
      break;

    case LEDMenuItemType::NUMBER:
      if (_currentValue < item.maxValue)
      {
        _currentValue = min(_currentValue + item.step, item.maxValue);
        _valueModified = true;
      }
      break;

    default:
      break;
    }

    _resetActivity();
  }

  // PREV button - decrease value
  if (BtnPrev.clicks == 1)
  {
    BtnPrev.clicks = 0;

    const LEDMenuItem &item = _menuItems[_currentMenuItem];

    switch (item.type)
    {
    case LEDMenuItemType::TOGGLE:
      _currentValue = !_currentValue;
      _valueModified = true;
      break;

    case LEDMenuItemType::SELECT:
      if (_currentValue > item.minValue)
      {
        _currentValue--;
        _valueModified = true;
      }
      break;

    case LEDMenuItemType::NUMBER:
      if (_currentValue > item.minValue)
      {
        _currentValue = max(_currentValue - item.step, item.minValue);
        _valueModified = true;
      }
      break;

    default:
      break;
    }

    _resetActivity();
  }
}

void LEDMenu::_updateLEDs()
{
  switch (_state)
  {
  case LEDMenuState::IDLE:
    _showIdleState();
    break;

  case LEDMenuState::MAIN_MENU:
    _showMenuPosition();
    break;

  case LEDMenuState::EDITING_VALUE:
    _showEditingValue();
    break;
  }
}

void LEDMenu::_enterMenu()
{
  if (_menuItems.empty())
  {
    ESP_LOGW(TAG, "Cannot enter menu - no menu items");
    return;
  }

  _state = LEDMenuState::MAIN_MENU;
  _currentMenuItem = 0;
  _resetActivity();

  ESP_LOGI(TAG, "Entered menu mode");
}

void LEDMenu::_exitMenu()
{
  _state = LEDMenuState::IDLE;
  _currentMenuItem = 0;
  _valueModified = false;

  ESP_LOGI(TAG, "Exited menu mode");
}

void LEDMenu::_enterEditing()
{
  _state = LEDMenuState::EDITING_VALUE;
  _currentValue = _getCurrentItemValue();
  _valueModified = false;

  ESP_LOGI(TAG, "Entered editing mode for: %s", _menuItems[_currentMenuItem].name.c_str());
}

void LEDMenu::_exitEditing()
{
  _state = LEDMenuState::MAIN_MENU;
  _valueModified = false;

  ESP_LOGI(TAG, "Exited editing mode");
}

void LEDMenu::_showMenuPosition()
{
  unsigned long currentTime = millis();

  // Blink LED1 to show menu position (1-based)
  if (currentTime - _ledBlinkTimer >= LED_BLINK_INTERVAL)
  {
    _ledBlinkTimer = currentTime;
    _ledBlinkState = !_ledBlinkState;
  }

  if (_ledBlinkState)
  {
    statusLed1.setMode(RGB_MODE::Overide);
    statusLed1.setOverideColor(COLOR_MENU_POS);
  }
  else
  {
    statusLed1.setMode(RGB_MODE::Overide);
    statusLed1.setOverideColor(0x000000);
  }

  // LED2 shows current menu item number by blinking N times
  static int blinkCount = 0;
  static unsigned long lastBlink = 0;
  static bool blinkPhase = false; // false = off, true = on

  if (currentTime - lastBlink >= 200) // Fast blinks every 200ms
  {
    lastBlink = currentTime;

    if (!blinkPhase)
    {
      // Turn on
      statusLed2.setMode(RGB_MODE::Overide);
      statusLed2.setOverideColor(COLOR_MENU_POS);
      blinkPhase = true;
    }
    else
    {
      // Turn off
      statusLed2.setMode(RGB_MODE::Overide);
      statusLed2.setOverideColor(0x000000);
      blinkPhase = false;
      blinkCount++;

      // After showing the menu number, pause
      if (blinkCount >= (_currentMenuItem + 1))
      {
        blinkCount = 0;
        lastBlink = currentTime + 1000; // 1 second pause
      }
    }
  }
}

void LEDMenu::_showEditingValue()
{
  // LED1 shows editing mode (steady purple)
  statusLed1.setMode(RGB_MODE::Overide);
  statusLed1.setOverideColor(COLOR_EDITING);

  // LED2 shows the value
  const LEDMenuItem &item = _menuItems[_currentMenuItem];
  uint32_t valueColor = 0x000000;

  switch (item.type)
  {
  case LEDMenuItemType::TOGGLE:
    valueColor = _currentValue ? COLOR_VALUE_ON : COLOR_VALUE_OFF;
    break;

  case LEDMenuItemType::SELECT:
  case LEDMenuItemType::NUMBER:
    valueColor = COLOR_NUMBER;
    // Could implement blinking pattern based on value
    break;

  default:
    valueColor = 0x101010; // Dim white
    break;
  }

  // For numbers, blink LED2 to represent the value
  if (item.type == LEDMenuItemType::NUMBER || item.type == LEDMenuItemType::SELECT)
  {
    unsigned long currentTime = millis();
    static int blinkCount = 0;
    static unsigned long lastBlink = 0;
    static bool blinkPhase = false;

    if (currentTime - lastBlink >= 300)
    {
      lastBlink = currentTime;

      if (!blinkPhase)
      {
        statusLed2.setMode(RGB_MODE::Overide);
        statusLed2.setOverideColor(valueColor);
        blinkPhase = true;
      }
      else
      {
        statusLed2.setMode(RGB_MODE::Overide);
        statusLed2.setOverideColor(0x000000);
        blinkPhase = false;
        blinkCount++;

        // Show value as number of blinks (capped at 10)
        int displayValue = min(_currentValue, 10);
        if (blinkCount >= displayValue)
        {
          blinkCount = 0;
          lastBlink = currentTime + 1500; // Longer pause between sequences
        }
      }
    }
  }
  else
  {
    // For toggles, show steady color
    statusLed2.setMode(RGB_MODE::Overide);
    statusLed2.setOverideColor(valueColor);
  }
}

void LEDMenu::_showIdleState()
{
  // Both LEDs dim blue to indicate system is ready
  statusLed1.setMode(RGB_MODE::Overide);
  statusLed1.setOverideColor(COLOR_IDLE);

  statusLed2.setMode(RGB_MODE::Overide);
  statusLed2.setOverideColor(COLOR_IDLE);
}

void LEDMenu::_executeCurrentItem()
{
  if (_currentMenuItem >= _menuItems.size())
    return;

  const LEDMenuItem &item = _menuItems[_currentMenuItem];

  switch (item.type)
  {
  case LEDMenuItemType::ACTION:
    ESP_LOGI(TAG, "Executing action: %s", item.name.c_str());
    if (item.actionCallback)
    {
      item.actionCallback();
    }
    break;

  case LEDMenuItemType::TOGGLE:
  case LEDMenuItemType::SELECT:
  case LEDMenuItemType::NUMBER:
    _enterEditing();
    break;
  }
}

void LEDMenu::_incrementCurrentItem()
{
  if (_menuItems.empty())
    return;

  _currentMenuItem = (_currentMenuItem + 1) % _menuItems.size();
  ESP_LOGD(TAG, "Menu item: %d (%s)", _currentMenuItem, _menuItems[_currentMenuItem].name.c_str());
}

void LEDMenu::_decrementCurrentItem()
{
  if (_menuItems.empty())
    return;

  _currentMenuItem = (_currentMenuItem == 0) ? (_menuItems.size() - 1) : (_currentMenuItem - 1);
  ESP_LOGD(TAG, "Menu item: %d (%s)", _currentMenuItem, _menuItems[_currentMenuItem].name.c_str());
}

int LEDMenu::_getCurrentItemValue()
{
  if (_currentMenuItem >= _menuItems.size())
    return 0;

  const LEDMenuItem &item = _menuItems[_currentMenuItem];

  switch (item.type)
  {
  case LEDMenuItemType::TOGGLE:
    return item.toggleValuePtr ? (*item.toggleValuePtr ? 1 : 0) : 0;

  case LEDMenuItemType::SELECT:
    return item.intValuePtr ? *item.intValuePtr : 0;

  case LEDMenuItemType::NUMBER:
    return item.intValuePtr ? *item.intValuePtr : 0;

  default:
    return 0;
  }
}

void LEDMenu::_setCurrentItemValue(int value)
{
  if (_currentMenuItem >= _menuItems.size())
    return;

  const LEDMenuItem &item = _menuItems[_currentMenuItem];

  switch (item.type)
  {
  case LEDMenuItemType::TOGGLE:
    if (item.toggleValuePtr)
    {
      *item.toggleValuePtr = (value != 0);
    }
    break;

  case LEDMenuItemType::SELECT:
    if (item.intValuePtr)
    {
      *item.intValuePtr = constrain(value, item.minValue, item.maxValue);
    }
    break;

  case LEDMenuItemType::NUMBER:
    if (item.intValuePtr)
    {
      *item.intValuePtr = constrain(value, item.minValue, item.maxValue);
    }
    break;

  default:
    break;
  }
}

String LEDMenu::_getCurrentItemDisplayValue()
{
  if (_currentMenuItem >= _menuItems.size())
    return "?";

  const LEDMenuItem &item = _menuItems[_currentMenuItem];
  int value = _getCurrentItemValue();

  switch (item.type)
  {
  case LEDMenuItemType::TOGGLE:
    return value ? "ON" : "OFF";

  case LEDMenuItemType::SELECT:
    if (item.options && value >= 0 && value < item.options->size())
    {
      return (*item.options)[value];
    }
    return String(value);

  case LEDMenuItemType::NUMBER:
    return String(value);

  case LEDMenuItemType::ACTION:
    return "EXEC";

  default:
    return "?";
  }
}

void LEDMenu::_resetActivity()
{
  _lastActivity = millis();
}

bool LEDMenu::_isActivityTimedOut()
{
  return (millis() - _lastActivity) >= AUTO_EXIT_TIMEOUT;
}