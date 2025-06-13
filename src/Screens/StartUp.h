#pragma once

#include "config.h"
#include "IO/Display.h"
#include "IO/GPIO.h"

enum StartUpState
{
  StartUp,
};

class StartUpScreen : public Screen
{
private:
  StartUpState state;
  int stage;

public:
  StartUpScreen(String _name);

  void draw() override;
  void update() override;

  void setState(StartUpState _state);
  StartUpState getState();

  void setStage(int _stage);
  int getStage();
};

StartUpScreen::StartUpScreen(String _name) : Screen(_name)
{
  state = StartUpState::StartUp;
  stage = 0;
}

void StartUpScreen::setState(StartUpState _state)
{
  state = _state;
}

StartUpState StartUpScreen::getState()
{
  return state;
}

void StartUpScreen::setStage(int _stage)
{
  stage = _stage;
}

int StartUpScreen::getStage()
{
  return stage;
}

void StartUpScreen::draw()
{
  display.noTopBar();

  switch (state)
  {
  case StartUpState::StartUp:
    display.u8g2.setFont(u8g2_font_logisoso16_tr);
    display.u8g2.setDrawColor(1);

    display.drawCenteredText(30, "Starting up...");
    display.drawCenteredText(54, "Stage " + String(stage));

    break;

  default:
    break;
  }
}

void StartUpScreen::update()
{
}