#pragma once

#include "config.h"
#include "IO/Display.h"
#include "IO/GPIO.h"

#include "IO/Menu.h"

class HomeScreen : public Screen
{
public:
  HomeScreen(String _name);

  Menu menu = Menu(MenuSize::Medium);

  MenuItemNavigate applicationMenuItem = MenuItemNavigate("Application", "Application");
  MenuItemNavigate syncMenuItem = MenuItemNavigate("Sync Manager", "Sync");
  MenuItemNavigate settingsMenuItem = MenuItemNavigate("Settings", "Settings");

  MenuItemAction powerOffItem = MenuItemAction(
      "Power Off", -1, []()
      { screenManager.setScreen("Shutdown"); });

  void draw() override;
  void update() override;
  void onEnter() override;
  uint8_t active;
};

HomeScreen::HomeScreen(String _name) : Screen(_name)
{
  active = 1;

  menu.addMenuItem(&applicationMenuItem);
  menu.addMenuItem(&syncMenuItem);
  menu.addMenuItem(&settingsMenuItem);
  menu.addMenuItem(&powerOffItem);

  settingsMenuItem.addRoute(2, "Debug");
}

void HomeScreen::draw()
{
  menu.draw();
}

void HomeScreen::update()
{
  menu.update();
}

void HomeScreen::onEnter()
{
}