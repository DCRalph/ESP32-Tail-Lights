#include "Shutdown.h"
#ifdef ENABLE_DISPLAY
#include "IO/Display.h"
#include "IO/GPIO.h"
#include "IO/ScreenManager.h"

namespace ShutdownScreenNamespace
{
  // State variables
  static ShutdownState state;
  static unsigned long startTime = 0;
  static long countdown = 1500;
  static uint8_t progress;

  // onEnter function
  void shutdownScreenOnEnter()
  {
    state = ShutdownState::Countdown;
    startTime = millis();
  }

  // onExit function
  void shutdownScreenOnExit()
  {
    // Clean up allocated memory
  }

  // draw function
  void shutdownScreenDraw()
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

  // update function
  void shutdownScreenUpdate()
  {
    if (BtnSel.clicks != 0)
    {
      screenManager.back();
    }
    if (state == ShutdownState::Countdown)
    {
      if (millis() - startTime > countdown)
      {
        state = ShutdownState::Shutdown_Warning;
      }
    }

    if (state == ShutdownState::Shutdown_Warning)
    {
      if (millis() - startTime > countdown + 500)
      {
        state = ShutdownState::Shutdown;
      }
    }

    if (state == ShutdownState::Shutdown)
    {
      // Shutdown
      Serial.println("Shutting down...");
    }
  }

  // Helper functions
  void setState(ShutdownState _state)
  {
    state = _state;
  }

  ShutdownState getState()
  {
    return state;
  }

} // namespace ShutdownScreenNamespace

// Define the ShutdownScreen Screen2 instance
const Screen2 ShutdownScreen = {
    F("Shutdown"),
    F("Shutdown"),
    ShutdownScreenNamespace::shutdownScreenDraw,
    ShutdownScreenNamespace::shutdownScreenUpdate,
    ShutdownScreenNamespace::shutdownScreenOnEnter,
    ShutdownScreenNamespace::shutdownScreenOnExit};

#endif