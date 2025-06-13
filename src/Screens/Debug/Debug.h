#pragma once

#include "config.h"
#include "IO/Display.h"
#include "IO/GPIO.h"

#include "IO/Menu.h"

class DebugScreen : public Screen
{
public:
  DebugScreen(String _name);

  long bootCount;
  bool ledState = false;
  bool otaSetupTmp = false;

  uint64_t totalMem = 0;
  uint64_t freeMem = 0;
  uint64_t usedMem = 0;

  String totalMemStr = "0";
  String freeMemStr = "0";
  String usedMemStr = "0";

  Menu menu = Menu(MenuSize::Medium);

  MenuItemBack backItem;

  MenuItemNavigate ioTestItem = MenuItemNavigate("IO Test", "IO Test");

  MenuItemNavigate batteryItem = MenuItemNavigate("Battery", "Battery");

  MenuItemNumber<long> bootCountItem = MenuItemNumber<long>("Boot", &bootCount);

  MenuItemString totalMemItem = MenuItemString("tmem", &totalMemStr);
  MenuItemString freeMemItem = MenuItemString("fmem", &freeMemStr);
  MenuItemString usedMemItem = MenuItemString("umem", &usedMemStr);

  void draw() override;
  void update() override;
  void onEnter() override;
};

DebugScreen::DebugScreen(String _name) : Screen(_name)
{

  menu.addMenuItem(&backItem);
  menu.addMenuItem(&ioTestItem);
  menu.addMenuItem(&batteryItem);
  menu.addMenuItem(&bootCountItem);

  menu.addMenuItem(&totalMemItem);
  menu.addMenuItem(&freeMemItem);
  menu.addMenuItem(&usedMemItem);
}

void DebugScreen::draw()
{
  menu.draw();
}

void DebugScreen::update()
{
  menu.update();

  totalMem = ESP.getHeapSize();
  freeMem = ESP.getFreeHeap();
  usedMem = totalMem - freeMem;

  totalMemStr = formatBytes(totalMem, true);
  freeMemStr = formatBytes(freeMem, true);
  usedMemStr = formatBytes(usedMem, true);
}

void DebugScreen::onEnter()
{

  bootCount = preferences.getLong("bootCount", 0);
}