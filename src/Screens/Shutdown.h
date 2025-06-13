#pragma once

#include "config.h"
#include "IO/Display.h"
#include "IO/GPIO.h"

enum ShutdownState
{
  Countdown,
  Shutdown_Warning,
  Shutdown
};

class ShutdownScreen : public Screen
{
private:
  ShutdownState state;
  unsigned long startTime = 0;
  long countdown = 1500;
  uint8_t progress;

public:
  ShutdownScreen(String _name);

  void draw() override;
  void update() override;
  void onEnter() override;

  void setState(ShutdownState _state);
  ShutdownState getState();
};

ShutdownScreen::ShutdownScreen(String _name) : Screen(_name)
{
  state = ShutdownState::Countdown;
}

void ShutdownScreen::onEnter()
{
  state = ShutdownState::Countdown;
  startTime = millis();
}

void ShutdownScreen::setState(ShutdownState _state)
{
  state = _state;
}

ShutdownState ShutdownScreen::getState()
{
  return state;
}

void ShutdownScreen::draw()
{
  display.noTopBar();

  switch (state)
  {
  case ShutdownState::Countdown:

    display.u8g2.setFont(u8g2_font_logisoso16_tr);
    display.drawCenteredText(20, "Shutting down");

    display.u8g2.setFont(u8g2_font_koleeko_tf);
    display.drawCenteredText(44, "Press to cancel");

    progress = map(millis() - startTime, 0, countdown, 100, 0);
    progress = constrain(progress, 0, 100);

    display.u8g2.drawFrame(0, 48, 127, 16);
    display.u8g2.drawBox(2, 50, map(progress, 0, 100, 0, 123), 12);

    break;

  case ShutdownState::Shutdown_Warning:
    display.u8g2.setFont(u8g2_font_logisoso16_tr);
    display.u8g2.setDrawColor(1);

    display.drawCenteredText(30, "Shutting down");

    break;

  case ShutdownState::Shutdown:
    display.u8g2.setFont(u8g2_font_logisoso16_tr);
    display.u8g2.setDrawColor(1);

    display.drawCenteredText(30, "Shutting down");

    break;

  default:
    break;
  }
}

void ShutdownScreen::update()
{
  if (BtnSel.clicks != 0)
  {
    screenManager.back();
  }
  if (state == ShutdownState::Countdown)
  {
    if (millis() - startTime > countdown)
    {
      setState(ShutdownState::Shutdown_Warning);
    }
  }

  if (state == ShutdownState::Shutdown_Warning)
  {
    if (millis() - startTime > countdown + 500)
    {
      setState(ShutdownState::Shutdown);
    }
  }

  if (state == ShutdownState::Shutdown)
  {
    // Shutdown
    Serial.println("Shutting down...");
  }
}