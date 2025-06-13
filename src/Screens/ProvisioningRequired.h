#pragma once

#include "config.h"
#include "IO/Display.h"
#include "IO/GPIO.h"

class ProvisioningRequiredScreen : public Screen
{
public:
  ProvisioningRequiredScreen(String _name);

  void draw() override;
  void update() override;
  void onEnter() override;
};

ProvisioningRequiredScreen::ProvisioningRequiredScreen(String _name) : Screen(_name)
{
}

void ProvisioningRequiredScreen::draw()
{
  display.noTopBar();

  display.u8g2.setFont(u8g2_font_logisoso16_tr);
  display.u8g2.setDrawColor(1);

  display.drawCenteredText(20, "Provisioning");
  display.drawCenteredText(40, "Required");

  // display.u8g2.setFont(u8g2_font_6x10_tf);
  // display.drawCenteredText(58, "Device must be provisioned");
}

void ProvisioningRequiredScreen::update()
{
  // No button handling - no way to leave this screen
  // The screen is intentionally locked until provisioning is complete
}

void ProvisioningRequiredScreen::onEnter()
{
  // Called when screen is entered
}