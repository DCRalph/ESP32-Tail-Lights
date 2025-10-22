#include "LEDMenu.h"

static const char *TAG = "LEDMenu";

// default patterns - updated to use new pattern system
const LEDPattern LEDMenu::IDLE_PATTERN = LEDPattern(0x00ff00, 0x001100);
const LEDPattern LEDMenu::EDITING_PATTERN = LEDPattern::createAlternating(0xFF0000, 0x000000, 0x0000FF, 0x000000, 250);

std::map<ButtonEventType, String> buttonEventTypeStrings = {
    {ButtonEventType::SINGLE_CLICK, "SINGLE_CLICK"},
    {ButtonEventType::DOUBLE_CLICK, "DOUBLE_CLICK"},
    {ButtonEventType::TRIPLE_CLICK, "TRIPLE_CLICK"},
    {ButtonEventType::LONG_CLICK, "LONG_CLICK"},
    {ButtonEventType::DOUBLE_LONG_CLICK, "DOUBLE_LONG_CLICK"},
    {ButtonEventType::TRIPLE_LONG_CLICK, "TRIPLE_LONG_CLICK"},
};

std::map<ButtonType, String> buttonTypeStrings = {
    {ButtonType::BOOT, "BOOT"},
    {ButtonType::PREV, "PREV"},
    {ButtonType::SELECT, "SELECT"},
    {ButtonType::NEXT, "NEXT"},
};

// LEDMenuItem implementation
LEDMenuItem LEDMenuItem::createAction(const String &name,
                                      std::function<void()> cb,
                                      const LEDPattern &pattern,
                                      const LEDPattern &editPattern)
{
  LEDMenuItem i;
  i.name = name;
  i.type = LEDMenuItemType::ACTION;
  i.pattern = pattern;
  i.editingPattern = editPattern.type != LEDPatternType::SOLID || editPattern.led1Color != 0 || editPattern.led2Color != 0 ? editPattern : LEDMenu::EDITING_PATTERN;
  i.actionCallback = cb;
  return i;
}

LEDMenuItem LEDMenuItem::createToggle(const String &name,
                                      bool *ptr,
                                      const LEDPattern &pattern,
                                      const LEDPattern &editPattern)
{
  LEDMenuItem i;
  i.name = name;
  i.type = LEDMenuItemType::TOGGLE;
  i.pattern = pattern;
  i.editingPattern = editPattern.type != LEDPatternType::SOLID || editPattern.led1Color != 0 || editPattern.led2Color != 0 ? editPattern : LEDMenu::EDITING_PATTERN;
  i.toggleValuePtr = ptr;
  return i;
}

LEDMenuItem LEDMenuItem::createSelect(const String &name,
                                      int *ptr,
                                      int minV,
                                      int maxV,
                                      std::vector<String> *opts,
                                      const LEDPattern &pattern,
                                      const LEDPattern &editPattern,
                                      std::function<void()> onSave)
{
  LEDMenuItem i;
  i.name = name;
  i.type = LEDMenuItemType::SELECT;
  i.pattern = pattern;
  i.editingPattern = editPattern.type != LEDPatternType::SOLID || editPattern.led1Color != 0 || editPattern.led2Color != 0 ? editPattern : LEDMenu::EDITING_PATTERN;
  i.intValuePtr = ptr;
  i.minValue = minV;
  i.maxValue = maxV;
  i.options = opts;
  i.actionCallback = onSave;
  return i;
}

LEDMenuItem LEDMenuItem::createSubMenu(const String &name,
                                       std::vector<LEDMenuItem> *subItems,
                                       const LEDPattern &pattern)
{
  LEDMenuItem i;
  i.name = name;
  i.type = LEDMenuItemType::SUBMENU;
  i.pattern = pattern;
  i.subMenuItems = subItems;
  return i;
}

LEDMenuItem LEDMenuItem::createBack(const String &name,
                                    const LEDPattern &pattern)
{
  LEDMenuItem i;
  i.name = name;
  i.type = LEDMenuItemType::BACK;
  i.pattern = pattern.type != LEDPatternType::SOLID || pattern.led1Color != 0 || pattern.led2Color != 0 ? pattern : LEDPattern::createFlash(0xFFA500, 0xFFA500, 300, 1700);
  return i;
}

void LEDMenuItem::startEditing()
{
  state = LEDMenuItemState::EDITING;
  currentValue = getCurrentValue();
  valueModified = false;
}

void LEDMenuItem::stopEditing()
{
  state = LEDMenuItemState::NORMAL;
  valueModified = false;
}

void LEDMenuItem::incrementValue()
{
  if (type == LEDMenuItemType::TOGGLE)
  {
    currentValue = !currentValue;
    valueModified = true;
  }
  else if (type == LEDMenuItemType::SELECT && currentValue < maxValue)
  {
    currentValue++;
    valueModified = true;
  }
}

void LEDMenuItem::decrementValue()
{
  if (type == LEDMenuItemType::TOGGLE)
  {
    currentValue = !currentValue;
    valueModified = true;
  }
  else if (type == LEDMenuItemType::SELECT && currentValue > minValue)
  {
    currentValue--;
    valueModified = true;
  }
}

int LEDMenuItem::getCurrentValue() const
{
  if (type == LEDMenuItemType::TOGGLE)
    return toggleValuePtr && *toggleValuePtr ? 1 : 0;
  if (type == LEDMenuItemType::SELECT)
    return intValuePtr ? *intValuePtr : 0;
  return 0;
}

void LEDMenuItem::setCurrentValue(int v)
{
  if (type == LEDMenuItemType::TOGGLE && toggleValuePtr)
    *toggleValuePtr = (v != 0);
  if (type == LEDMenuItemType::SELECT && intValuePtr)
    *intValuePtr = constrain(v, minValue, maxValue);
}

void LEDMenuItem::saveValue()
{
  if (valueModified)
  {
    setCurrentValue(currentValue);
    if (actionCallback)
      actionCallback();
  }
  stopEditing();
}

void LEDMenuItem::cancelEdit()
{
  currentValue = getCurrentValue(); // Reset to original value
  stopEditing();
}

LEDPattern LEDMenuItem::getCurrentPattern() const
{
  return isEditing() ? editingPattern : pattern;
}

// LEDMenu implementation
LEDMenu::LEDMenu() {}
LEDMenu::~LEDMenu() { clearMenuItems(); }

void LEDMenu::begin()
{
  Serial.println("[LEDMenu] LEDMenu init");
  _patternStartTime = millis();
  _showPattern(IDLE_PATTERN);
}

void LEDMenu::setEnabled(bool ena)
{
  if (_enabled == ena)
    return;
  _enabled = ena;
  if (_enabled)
  {
    Serial.println("[LEDMenu] Menu enabled");
    _showPattern(IDLE_PATTERN);
  }
  else
  {
    Serial.println("[LEDMenu] Menu disabled");
    _exitMenu();
  }
}

bool LEDMenu::isEnabled() const { return _enabled; }

void LEDMenu::addMenuItem(const LEDMenuItem &item)
{
  _rootMenuItems.push_back(item);
  Serial.print("[LEDMenu] DEBUG: Added menu item '");
  Serial.print(item.name);
  Serial.println("'");
}

void LEDMenu::clearMenuItems()
{
  _rootMenuItems.clear();
  _menuStack.clear();
}

void LEDMenu::enterSubMenu(std::vector<LEDMenuItem> *subItems)
{
  if (subItems && !subItems->empty())
  {
    _menuStack.push_back(MenuLevel(subItems, 0));
    _patternStartTime = millis();
    Serial.print("[LEDMenu] Entered submenu with ");
    Serial.print(subItems->size());
    Serial.println(" items");
  }
}

void LEDMenu::exitSubMenu()
{
  if (_menuStack.size() > 1)
  {
    _menuStack.pop_back();
    _patternStartTime = millis();
    Serial.println("[LEDMenu] Exited submenu");
  }
  else
  {
    _exitMenu();
  }
}

void LEDMenu::update()
{
  if (!_enabled)
    return;
  _updateLEDs();
  if (_state != LEDMenuState::IDLE && _timedOut())
  {
    Serial.println("[LEDMenu] Auto-exit menu (timeout)");
    _exitMenu();
  }
}

void LEDMenu::handleButtonEvent(const ButtonEvent &ev)
{
  if (!_enabled)
    return;
  _resetActivity();

  // print event
  Serial.print("[LEDMenu] Button event: ");
  Serial.print(buttonTypeStrings[ev.button]);
  Serial.print(", ");
  Serial.println(buttonEventTypeStrings[ev.type]);

  if (_state == LEDMenuState::IDLE)
  {
    if (ev.button == ButtonType::SELECT && ev.type == ButtonEventType::LONG_CLICK)
    {
      _enterMenu();
    }
  }
  else
  {
    LEDMenuItem *currentItem = _getCurrentItem();
    if (currentItem && currentItem->isEditing())
    {
      _onEditingEvent(ev);
    }
    else
    {
      _onMenuEvent(ev);
    }
  }
}

void LEDMenu::exit()
{
  _exitMenu();
}

// — state transitions —

void LEDMenu::_enterMenu()
{
  if (_rootMenuItems.empty())
  {
    Serial.println("[LEDMenu] WARNING: No items – cannot enter menu");
    return;
  }
  _state = LEDMenuState::ACTIVE;
  _menuStack.clear();
  _menuStack.push_back(MenuLevel(&_rootMenuItems, 0));
  _patternStartTime = millis();
  Serial.println("[LEDMenu] Entered menu");
}

void LEDMenu::_exitMenu()
{
  // Stop any editing
  LEDMenuItem *currentItem = _getCurrentItem();
  if (currentItem && currentItem->isEditing())
  {
    currentItem->stopEditing();
  }

  _state = LEDMenuState::IDLE;
  _menuStack.clear();
  _patternStartTime = millis();
  Serial.println("[LEDMenu] Exited menu");
}

// — event dispatch —

void LEDMenu::_onMenuEvent(const ButtonEvent &ev)
{
  if (ev.type != ButtonEventType::SINGLE_CLICK)
    return;

  switch (ev.button)
  {
  case ButtonType::BOOT:
    exitSubMenu();
    break;
  case ButtonType::NEXT:
    _incrementCurrentItem();
    break;
  case ButtonType::PREV:
    _decrementCurrentItem();
    break;
  case ButtonType::SELECT:
    _executeCurrentItem();
    break;
  }
}

void LEDMenu::_onEditingEvent(const ButtonEvent &ev)
{
  LEDMenuItem *currentItem = _getCurrentItem();
  if (!currentItem)
    return;

  // BOOT cancels edit
  if (ev.button == ButtonType::BOOT && ev.type == ButtonEventType::SINGLE_CLICK)
  {
    currentItem->cancelEdit();
    _patternStartTime = millis();
    return;
  }

  // SELECT saves & calls onSave
  if (ev.button == ButtonType::SELECT && ev.type == ButtonEventType::SINGLE_CLICK)
  {
    Serial.print("[LEDMenu] Saved value=");
    Serial.println(currentItem->currentValue);
    currentItem->saveValue();
    _patternStartTime = millis();
    return;
  }

  if (ev.type != ButtonEventType::SINGLE_CLICK)
    return;

  switch (ev.button)
  {
  case ButtonType::NEXT:
    currentItem->incrementValue();
    break;
  case ButtonType::PREV:
    currentItem->decrementValue();
    break;
  default:
    break;
  }
}

// — core logic —

void LEDMenu::_incrementCurrentItem()
{
  MenuLevel *level = _getCurrentLevel();
  if (!level || !level->items || level->items->empty())
    return;

  level->currentIndex = (level->currentIndex + 1) % level->items->size();
  _patternStartTime = millis();
  Serial.print("[LEDMenu] DEBUG: Menu idx=");
  Serial.println(level->currentIndex);
}

void LEDMenu::_decrementCurrentItem()
{
  MenuLevel *level = _getCurrentLevel();
  if (!level || !level->items || level->items->empty())
    return;

  if (level->currentIndex == 0)
    level->currentIndex = level->items->size() - 1;
  else
    level->currentIndex--;
  _patternStartTime = millis();
  Serial.print("[LEDMenu] DEBUG: Menu idx=");
  Serial.println(level->currentIndex);
}

void LEDMenu::_executeCurrentItem()
{
  LEDMenuItem *item = _getCurrentItem();
  if (!item)
    return;

  switch (item->type)
  {
  case LEDMenuItemType::ACTION:
    Serial.print("[LEDMenu] Action '");
    Serial.print(item->name);
    Serial.println("'");
    if (item->actionCallback)
      item->actionCallback();
    break;

  case LEDMenuItemType::TOGGLE:
  case LEDMenuItemType::SELECT:
    item->startEditing();
    _patternStartTime = millis();
    Serial.print("[LEDMenu] Editing '");
    Serial.print(item->name);
    Serial.println("'");
    break;

  case LEDMenuItemType::SUBMENU:
    if (item->subMenuItems)
    {
      enterSubMenu(item->subMenuItems);
      Serial.print("[LEDMenu] Entered submenu '");
      Serial.print(item->name);
      Serial.println("'");
    }
    break;

  case LEDMenuItemType::BACK:
    exitSubMenu();
    Serial.println("[LEDMenu] Back from submenu");
    break;
  }
}

LEDMenuItem *LEDMenu::_getCurrentItem()
{
  MenuLevel *level = _getCurrentLevel();
  if (!level || !level->items || level->items->empty())
    return nullptr;

  if (level->currentIndex < 0 || level->currentIndex >= level->items->size())
    return nullptr;

  return &(*level->items)[level->currentIndex];
}

LEDMenu::MenuLevel *LEDMenu::_getCurrentLevel()
{
  if (_menuStack.empty())
    return nullptr;
  return &_menuStack.back();
}

// — LEDs & timing —

void LEDMenu::_resetActivity()
{
  _lastActivity = millis();
}

bool LEDMenu::_timedOut() const
{
  return (millis() - _lastActivity) >= AUTO_EXIT_TIMEOUT;
}

void LEDMenu::_updateLEDs()
{
  LEDPattern currentPattern;

  if (_state == LEDMenuState::IDLE)
  {
    currentPattern = IDLE_PATTERN;
  }
  else
  {
    LEDMenuItem *item = _getCurrentItem();
    if (item)
    {
      currentPattern = item->getCurrentPattern();
    }
    else
    {
      currentPattern = IDLE_PATTERN;
    }
  }

  _showPattern(currentPattern);
}

void LEDMenu::_showPattern(const LEDPattern &p)
{
  uint32_t now = millis();
  uint32_t elapsed = now - _patternStartTime;

  switch (p.type)
  {
  case LEDPatternType::SOLID:
  {
    statusLed1.setMode(RGB_MODE::Overide);
    statusLed1.setOverideColor(p.led1Color);
    statusLed2.setMode(RGB_MODE::Overide);
    statusLed2.setOverideColor(p.led2Color);
    break;
  }

  case LEDPatternType::BLINK:
  {
    uint32_t cycleTime = elapsed % p.period;
    bool isOn = cycleTime < p.onTime;

    // Handle blink count limit
    if (p.blinkCount > 0)
    {
      uint32_t completedCycles = elapsed / p.period;
      if (completedCycles >= p.blinkCount)
      {
        isOn = false; // Stay off after completing blink count
      }
    }

    statusLed1.setMode(RGB_MODE::Overide);
    statusLed2.setMode(RGB_MODE::Overide);

    if (isOn)
    {
      statusLed1.setOverideColor(p.led1Color);
      statusLed2.setOverideColor(p.led2Color);
    }
    else
    {
      statusLed1.setOverideColor(0x000000);
      statusLed2.setOverideColor(0x000000);
    }
    break;
  }

  case LEDPatternType::PULSE:
  {
    uint8_t brightness = _calculatePulseBrightness(elapsed, p.period, p.minBrightness, p.maxBrightness);

    statusLed1.setMode(RGB_MODE::Overide);
    statusLed2.setMode(RGB_MODE::Overide);
    statusLed1.setOverideColor(_modulateBrightness(p.led1Color, brightness));
    statusLed2.setOverideColor(_modulateBrightness(p.led2Color, brightness));
    break;
  }

  case LEDPatternType::ALTERNATE:
  {
    uint32_t cycleTime = elapsed % p.period;
    bool useAlternate = cycleTime >= p.onTime;

    statusLed1.setMode(RGB_MODE::Overide);
    statusLed2.setMode(RGB_MODE::Overide);

    if (useAlternate)
    {
      statusLed1.setOverideColor(p.led1AltColor);
      statusLed2.setOverideColor(p.led2AltColor);
    }
    else
    {
      statusLed1.setOverideColor(p.led1Color);
      statusLed2.setOverideColor(p.led2Color);
    }
    break;
  }

  case LEDPatternType::FLASH:
  {
    uint32_t cycleTime = elapsed % p.period;
    bool isFlashing = cycleTime < p.onTime;

    statusLed1.setMode(RGB_MODE::Overide);
    statusLed2.setMode(RGB_MODE::Overide);

    if (isFlashing)
    {
      statusLed1.setOverideColor(p.led1Color);
      statusLed2.setOverideColor(p.led2Color);
    }
    else
    {
      statusLed1.setOverideColor(0x000000);
      statusLed2.setOverideColor(0x000000);
    }
    break;
  }

  case LEDPatternType::CUSTOM:
  {
    if (p.customFunction)
    {
      p.customFunction(elapsed, p.led1Color, p.led2Color);
    }
    else
    {
      // Fallback to solid pattern if no custom function
      statusLed1.setMode(RGB_MODE::Overide);
      statusLed1.setOverideColor(p.led1Color);
      statusLed2.setMode(RGB_MODE::Overide);
      statusLed2.setOverideColor(p.led2Color);
    }
    break;
  }
  }
}

uint32_t LEDMenu::_modulateBrightness(uint32_t color, uint8_t brightness)
{
  if (color == 0)
    return 0;

  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;

  r = (r * brightness) / 255;
  g = (g * brightness) / 255;
  b = (b * brightness) / 255;

  return (r << 16) | (g << 8) | b;
}

uint8_t LEDMenu::_calculatePulseBrightness(uint32_t time, uint16_t period, uint8_t minBright, uint8_t maxBright)
{
  float phase = (float)(time % period) / (float)period;
  float sinValue = sin(phase * 2 * PI);
  float normalizedSin = (sinValue + 1.0f) / 2.0f; // Normalize to 0-1

  return minBright + (uint8_t)((maxBright - minBright) * normalizedSin);
}

LEDMenu ledMenu;
