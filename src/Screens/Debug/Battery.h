#pragma once

#include "config.h"
#include "IO/Display.h"
#include "IO/GPIO.h"
#include "IO/Battery.h"

class BatteryScreen : public Screen
{
public:
  BatteryScreen(String _name);

  void draw() override;
  void update() override;
};

BatteryScreen::BatteryScreen(String _name) : Screen(_name)
{
}

void BatteryScreen::draw()
{

  display.u8g2.setFont(u8g2_font_logisoso16_tf);
  display.u8g2.setDrawColor(1);

  char buffer[32];
  sprintf(buffer, "%.2fV. %d%", batteryGetVoltage(), batteryGetPercentage());
  display.u8g2.drawStr(0, 28, buffer);

  sprintf(buffer, "%.2fV. %d%", batteryGetVoltageSmooth(), batteryGetPercentageSmooth());
  display.u8g2.drawStr(0, 46, buffer);
}

void BatteryScreen::update()
{
  if (BtnSel.clicks != 0)
  {
    screenManager.back();
  }
}