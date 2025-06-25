#include "StartUp.h"
#ifdef ENABLE_DISPLAY

#include "IO/Display.h"
#include "IO/GPIO.h"

namespace StartUpScreenNamespace
{
  // State variables
  static StartUpState state;
  static int stage;

  // onEnter function
  void startUpScreenOnEnter()
  {
    state = StartUpState::StartUp;
    stage = 0;
  }

  // onExit function
  void startUpScreenOnExit()
  {
    // Clean up allocated memory
  }

  // draw function
  void startUpScreenDraw()
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

  // update function
  void startUpScreenUpdate()
  {
    // No update logic in original
  }

  // Helper functions
  void setState(StartUpState _state)
  {
    state = _state;
  }

  StartUpState getState()
  {
    return state;
  }

  void setStage(int _stage)
  {
    stage = _stage;
  }

  int getStage()
  {
    return stage;
  }

} // namespace StartUpScreenNamespace

void startUpScreenSetStage(int _stage)
{
  StartUpScreenNamespace::setStage(_stage);
}

// Define the StartUpScreen Screen2 instance
const Screen2 StartUpScreen = {
    F("StartUp"),
    F("StartUp"),
    StartUpScreenNamespace::startUpScreenDraw,
    StartUpScreenNamespace::startUpScreenUpdate,
    StartUpScreenNamespace::startUpScreenOnEnter,
    StartUpScreenNamespace::startUpScreenOnExit};

#endif