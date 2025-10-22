#include "Menu.h"
#ifdef ENABLE_DISPLAY

static const char *TAG = "Menu";

// ###### MenuItem ######

void MenuItem::draw(uint8_t _x, uint8_t _y, bool _active)
{
  MenuSize size = parent ? parent->getMenuSize() : MenuSize::Large;

  if (size == MenuSize::Small)
  {
    display.u8g2.setFont(u8g2_font_6x10_tf);

    if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 9);
      display.u8g2.setDrawColor(0);
    }
    else
      display.u8g2.setDrawColor(1);
    display.u8g2.drawStr(_x + 1, _y + 8, getName().c_str());
  }
  else if (size == MenuSize::Medium)
  {
    display.u8g2.setFont(u8g2_font_doomalpha04_tr);

    if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 12);
      display.u8g2.setDrawColor(0);
    }
    else
      display.u8g2.setDrawColor(1);
    display.u8g2.drawStr(_x + 1, _y + 11, getName().c_str());
  }
  else
  {
    display.u8g2.setFont(u8g2_font_profont22_tf);

    if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 16);
      display.u8g2.setDrawColor(0);
    }
    else
      display.u8g2.setDrawColor(1);
    display.u8g2.drawStr(_x + 1, _y + 15, getName().c_str());
  }
}

// ###### MenuItemToggle ######

void MenuItemToggle::draw(uint8_t _x, uint8_t _y, bool _active)
{
  MenuSize size = parent ? parent->getMenuSize() : MenuSize::Large;

  if (size == MenuSize::Small)
  {
    display.u8g2.setFont(u8g2_font_6x10_tf);

    if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 9);
      display.u8g2.setDrawColor(0);
    }
    else
      display.u8g2.setDrawColor(1);
    display.u8g2.drawStr(_x + 1, _y + 8, getName().c_str());

    String valueStr = *value ? "ON" : "OFF";
    display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 8, valueStr.c_str());
  }
  else if (size == MenuSize::Medium)
  {
    display.u8g2.setFont(u8g2_font_doomalpha04_tr);

    if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 12);
      display.u8g2.setDrawColor(0);
    }
    else
      display.u8g2.setDrawColor(1);
    display.u8g2.drawStr(_x + 1, _y + 11, getName().c_str());

    String valueStr = *value ? "ON" : "OFF";
    display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 11, valueStr.c_str());
  }
  else
  {
    display.u8g2.setFont(u8g2_font_profont22_tf);

    if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 16);
      display.u8g2.setDrawColor(0);
    }
    else
      display.u8g2.setDrawColor(1);
    display.u8g2.drawStr(_x + 1, _y + 15, getName().c_str());

    String valueStr = *value ? "ON" : "OFF";
    display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 15, valueStr.c_str());
  }
}

// ###### MenuItemString ######

void MenuItemString::draw(uint8_t _x, uint8_t _y, bool _active)
{
  MenuSize size = parent ? parent->getMenuSize() : MenuSize::Large;

  if (size == MenuSize::Small)
  {
    display.u8g2.setFont(u8g2_font_6x10_tf);

    if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 9);
      display.u8g2.setDrawColor(0);
    }
    else
      display.u8g2.setDrawColor(1);

    display.u8g2.drawStr(_x + 1, _y + 8, getName().c_str());
    display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(getValue().c_str()) - 5, _y + 8, getValue().c_str());
  }
  else if (size == MenuSize::Medium)
  {
    display.u8g2.setFont(u8g2_font_doomalpha04_tr);

    if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 12);
      display.u8g2.setDrawColor(0);
    }
    else
      display.u8g2.setDrawColor(1);

    display.u8g2.drawStr(_x + 1, _y + 11, getName().c_str());
    display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(getValue().c_str()) - 5, _y + 11, getValue().c_str());
  }
  else
  {
    display.u8g2.setFont(u8g2_font_profont22_tf);

    if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 16);
      display.u8g2.setDrawColor(0);
    }
    else
      display.u8g2.setDrawColor(1);

    display.u8g2.drawStr(_x + 1, _y + 15, getName().c_str());
    display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(getValue().c_str()) - 5, _y + 15, getValue().c_str());
  }
}

// ###### MenuItemNumber ######

template <typename T>
void MenuItemNumber<T>::draw(uint8_t _x, uint8_t _y, bool _active)
{
  MenuSize size = parent ? parent->getMenuSize() : MenuSize::Large;

  if (size == MenuSize::Small)
  {
    display.u8g2.setFont(u8g2_font_6x10_tf);

    if (!_active && selected)
    {
      selected = false;
    }

    String valueStr = String(*value);

    if (_active && selected)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawFrame(_x, _y, DISPLAY_WIDTH - 4, 9);
      display.u8g2.drawStr(_x + 1, _y + 8, getName().c_str());
      display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 8, valueStr.c_str());
    }
    else if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 9);
      display.u8g2.setDrawColor(0);
      display.u8g2.drawStr(_x + 1, _y + 8, getName().c_str());
      display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 8, valueStr.c_str());
    }
    else
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawStr(_x + 1, _y + 8, getName().c_str());
      display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 8, valueStr.c_str());
    }
  }
  else if (size == MenuSize::Medium)
  {
    display.u8g2.setFont(u8g2_font_doomalpha04_tr);

    if (!_active && selected)
    {
      selected = false;
    }

    String valueStr = String(*value);

    if (_active && selected)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawFrame(_x, _y, DISPLAY_WIDTH - 4, 12);
      display.u8g2.drawStr(_x + 1, _y + 11, getName().c_str());
      display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 11, valueStr.c_str());
    }
    else if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 12);
      display.u8g2.setDrawColor(0);
      display.u8g2.drawStr(_x + 1, _y + 11, getName().c_str());
      display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 11, valueStr.c_str());
    }
    else
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawStr(_x + 1, _y + 11, getName().c_str());
      display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 11, valueStr.c_str());
    }
  }
  else
  {
    display.u8g2.setFont(u8g2_font_profont22_tf);

    if (!_active && selected)
    {
      selected = false;
    }

    String valueStr = String(*value);

    if (_active && selected)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawFrame(_x, _y, DISPLAY_WIDTH - 4, 16);
      display.u8g2.drawStr(_x + 1, _y + 15, getName().c_str());
      display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 15, valueStr.c_str());
    }
    else if (_active)
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 16);
      display.u8g2.setDrawColor(0);
      display.u8g2.drawStr(_x + 1, _y + 15, getName().c_str());
      display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 15, valueStr.c_str());
    }
    else
    {
      display.u8g2.setDrawColor(1);
      display.u8g2.drawStr(_x + 1, _y + 15, getName().c_str());
      display.u8g2.drawStr(DISPLAY_WIDTH - display.u8g2.getStrWidth(valueStr.c_str()) - 5, _y + 15, valueStr.c_str());
    }
  }
}

// ###### MenuItemSelect ######

void MenuItemSelect::draw(uint8_t _x, uint8_t _y, bool _active)
{
  MenuSize size = parent ? parent->getMenuSize() : MenuSize::Large;
  String dispText = getName();
  String optionText = getSelectedOption();

  if (size == MenuSize::Small)
  {
    display.u8g2.setFont(u8g2_font_6x10_tf);

    if (_active && selected)
    {
      // Editing mode: draw a frame around the option.
      display.u8g2.setDrawColor(1);
      display.u8g2.drawFrame(_x, _y, DISPLAY_WIDTH - 4, 9);
      display.u8g2.drawStr(_x + 1, _y + 8, dispText.c_str());
      display.u8g2.drawStr(
          DISPLAY_WIDTH - display.u8g2.getStrWidth(optionText.c_str()) - 5,
          _y + 8,
          optionText.c_str());
    }
    else if (_active)
    {
      // Active but not editing: draw with a filled background.
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 9);
      display.u8g2.setDrawColor(0);
      display.u8g2.drawStr(_x + 1, _y + 8, dispText.c_str());
      display.u8g2.drawStr(
          DISPLAY_WIDTH - display.u8g2.getStrWidth(optionText.c_str()) - 5,
          _y + 8,
          optionText.c_str());
    }
    else
    {
      // Inactive: normal drawing.
      display.u8g2.setDrawColor(1);
      display.u8g2.drawStr(_x + 1, _y + 8, dispText.c_str());
      display.u8g2.drawStr(
          DISPLAY_WIDTH - display.u8g2.getStrWidth(optionText.c_str()) - 5,
          _y + 8,
          optionText.c_str());
    }
  }
  else if (size == MenuSize::Medium)
  {
    display.u8g2.setFont(u8g2_font_doomalpha04_tr);

    if (_active && selected)
    {
      // Editing mode: draw a frame around the option.
      display.u8g2.setDrawColor(1);
      display.u8g2.drawFrame(_x, _y, DISPLAY_WIDTH - 4, 12);
      display.u8g2.drawStr(_x + 1, _y + 11, dispText.c_str());
      display.u8g2.drawStr(
          DISPLAY_WIDTH - display.u8g2.getStrWidth(optionText.c_str()) - 5,
          _y + 11,
          optionText.c_str());
    }
    else if (_active)
    {
      // Active but not editing: draw with a filled background.
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 12);
      display.u8g2.setDrawColor(0);
      display.u8g2.drawStr(_x + 1, _y + 11, dispText.c_str());
      display.u8g2.drawStr(
          DISPLAY_WIDTH - display.u8g2.getStrWidth(optionText.c_str()) - 5,
          _y + 11,
          optionText.c_str());
    }
    else
    {
      // Inactive: normal drawing.
      display.u8g2.setDrawColor(1);
      display.u8g2.drawStr(_x + 1, _y + 11, dispText.c_str());
      display.u8g2.drawStr(
          DISPLAY_WIDTH - display.u8g2.getStrWidth(optionText.c_str()) - 5,
          _y + 11,
          optionText.c_str());
    }
  }
  else
  {
    // Draw similar to other items.
    display.u8g2.setFont(u8g2_font_profont22_tf);

    if (_active && selected)
    {
      // Editing mode: draw a frame around the option.
      display.u8g2.setDrawColor(1);
      display.u8g2.drawFrame(_x, _y, DISPLAY_WIDTH - 4, 16);
      display.u8g2.drawStr(_x + 1, _y + 15, dispText.c_str());
      display.u8g2.drawStr(
          DISPLAY_WIDTH - display.u8g2.getStrWidth(optionText.c_str()) - 5,
          _y + 15,
          optionText.c_str());
    }
    else if (_active)
    {
      // Active but not editing: draw with a filled background.
      display.u8g2.setDrawColor(1);
      display.u8g2.drawBox(_x, _y, DISPLAY_WIDTH - 4, 16);
      display.u8g2.setDrawColor(0);
      display.u8g2.drawStr(_x + 1, _y + 15, dispText.c_str());
      display.u8g2.drawStr(
          DISPLAY_WIDTH - display.u8g2.getStrWidth(optionText.c_str()) - 5,
          _y + 15,
          optionText.c_str());
    }
    else
    {
      // Inactive: normal drawing.
      display.u8g2.setDrawColor(1);
      display.u8g2.drawStr(_x + 1, _y + 15, dispText.c_str());
      display.u8g2.drawStr(
          DISPLAY_WIDTH - display.u8g2.getStrWidth(optionText.c_str()) - 5,
          _y + 15,
          optionText.c_str());
    }
  }
}

// ###### Menu ######

void Menu::draw()
{
  // Delegate to active submenu if present
  if (activeSubmenu)
  {
    activeSubmenu->draw();
    return;
  }

  int lineHeight;
  int startY = 12;

  if (menuSize == MenuSize::Small)
  {
    lineHeight = 8;
    display.u8g2.setFont(u8g2_font_6x10_tf);
  }
  else if (menuSize == MenuSize::Medium)
  {
    lineHeight = 12;
    display.u8g2.setFont(u8g2_font_doomalpha04_tr);
  }
  else
  {
    lineHeight = 18;
    display.u8g2.setFont(u8g2_font_profont22_tf);
  }

  uint8_t itemMap[numItems];

  uint8_t numItemsVisible = 0;
  uint8_t numItemsHidden = 0;
  for (uint8_t i = 0; i < numItems; i++)
  {
    if (!items[i]->isHidden())
    {
      itemMap[numItemsVisible] = i;
      numItemsVisible++;
    }

    if (items[i]->isHidden() && active > i)
      numItemsHidden++;
  }

  uint8_t calcActive = active - numItemsHidden;

  // Check if we have any visible items to draw
  if (numItemsVisible == 0)
  {
    return;
  }

  uint8_t itemRelIdx = calcActive - topItem;

  // if first item on screen is selected and item is the first item in the list - update topItem
  if (itemRelIdx <= 0 && calcActive == 0)
    topItem = 0;

  // if first item on screen is selected and item is not the first item in the list - update topItem
  if (itemRelIdx <= 0 && calcActive > 0)
    topItem--;

  // if last item on screen is selected and item is not the last item in the list - update topItem
  if (itemRelIdx >= numItemsPerPage - 1 && calcActive < numItemsVisible - 1)
    topItem++;

  // if last item on screen is selected and item is the last item in the list - update topItem
  if (itemRelIdx >= numItemsPerPage - 1 && calcActive == numItemsVisible - 1)
    topItem = numItemsVisible - numItemsPerPage;

  for (uint8_t i = 0; i < numItemsPerPage; i++)
  {
    uint8_t itemIdx = i + topItem;

    // Add bounds checking to prevent crash
    if (itemIdx >= numItemsVisible)
    {
      break;
    }

    MenuItem *item = items[itemMap[itemIdx]];

    item->draw(0, startY + (i * lineHeight), active == itemMap[itemIdx]);
  }

  display.u8g2.setDrawColor(1);

  uint8_t scrollBarPosition = (DISPLAY_HEIGHT - 13) / (numItemsVisible < 1 ? 1 : numItemsVisible) * calcActive;
  uint8_t scrollBarHeight = (numItemsVisible < 1 ? 1 : numItemsVisible) - 1 == calcActive ? DISPLAY_HEIGHT - 12 - scrollBarPosition : (DISPLAY_HEIGHT - 13) / (numItemsVisible < 1 ? 1 : numItemsVisible);

  display.u8g2.drawLine(DISPLAY_WIDTH - 2, 12, DISPLAY_WIDTH - 2, DISPLAY_HEIGHT - 1);
  display.u8g2.drawBox(DISPLAY_WIDTH - 3, 12 + scrollBarPosition, 3, scrollBarHeight);
}

void Menu::update()
{
  // Delegate to active submenu if present
  if (activeSubmenu)
  {
    activeSubmenu->update();
    return;
  }

  if (numItems < 1)
  {
    if (BtnSel.clicks == 1)
    {
      BtnSel.clicks = 0;
      screenManager.back();
    }

    return;
  }

  if (BtnBoot.clicks == -1)
  {
    BtnBoot.clicks = 0;
    // check if the current menu has a back item and if it does, run it
    for (auto item : items)
    {
      if (item->getType() == MenuItemType::Back)
      {
        if (item->functions.size() > 0)
        {
          item->functions[0].func();
          return;
        }
      }
    }
    return;
  }

  if (BtnSel.clicks != 0)
  {
    Serial.print("[Menu] Clicks: ");
    Serial.println(BtnSel.clicks);
    items[active]->run();
    BtnSel.clicks = 0;
  }

  MenuItem *currentItem = items[active];

  if (BtnNext.clicks == 1)
  {
    switch (currentItem->getType())
    {
    case MenuItemType::Select:
    {
      MenuItemSelect *selectItem = (MenuItemSelect *)currentItem;

      if (selectItem && selectItem->isSelected())
        selectItem->nextOption();
      else
        nextItem();
    }
    break;

    case MenuItemType::Number:
    {
      MenuItemNumberBase *numberItem = (MenuItemNumberBase *)currentItem;

      if (numberItem && numberItem->isSelected())
        numberItem->increase();
      else
        nextItem();
    }
    break;

    default:
      nextItem();
      break;
    }

    BtnNext.clicks = 0;
  }
  else if (BtnPrev.clicks == 1)
  {
    switch (currentItem->getType())
    {
    case MenuItemType::Select:
    {
      MenuItemSelect *selectItem = (MenuItemSelect *)currentItem;

      if (selectItem && selectItem->isSelected())
        selectItem->prevOption();
      else
        prevItem();
    }
    break;

    case MenuItemType::Number:
    {
      MenuItemNumberBase *numberItem = (MenuItemNumberBase *)currentItem;

      if (numberItem && numberItem->isSelected())
        numberItem->decrease();
      else
        prevItem();
    }
    break;

    default:
      prevItem();
      break;
    }

    BtnPrev.clicks = 0;
  }
}

#endif