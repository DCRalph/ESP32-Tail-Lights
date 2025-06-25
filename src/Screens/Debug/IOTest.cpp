#include "IOTest.h"
#ifdef ENABLE_DISPLAY
#include "IO/Display.h"
#include "IO/GPIO.h"
#include "IO/ScreenManager.h"

namespace IOTestScreenNamespace
{
  // State variables
  static int lastButtonClicks = 0;

  // onEnter function
  void ioTestScreenOnEnter()
  {
    lastButtonClicks = 0;
  }

  // onExit function
  void ioTestScreenOnExit()
  {
    // Clean up allocated memory
  }

  // draw function
  void ioTestScreenDraw()
  {
    // Update button states
    BtnSel.Update();
    BtnPrev.Update();
    BtnNext.Update();
    BtnBoot.Update();

    if (BtnSel.clicks != 0)
    {
      lastButtonClicks = BtnSel.clicks;
    }

    display.u8g2.setFont(u8g2_font_logisoso16_tf);
    display.u8g2.setDrawColor(1);

    // Display button states
    char buffer[64];
    sprintf(buffer, "Sel: %d : %d", BtnSel.depressed ? 1 : 0, lastButtonClicks);
    display.u8g2.drawStr(0, 20, buffer);

    sprintf(buffer, "Prev: %d", BtnPrev.depressed ? 1 : 0);
    display.u8g2.drawStr(0, 38, buffer);

    sprintf(buffer, "Next: %d", BtnNext.depressed ? 1 : 0);
    display.u8g2.drawStr(0, 56, buffer);

    sprintf(buffer, "Boot: %d", BtnBoot.depressed ? 1 : 0);
    display.u8g2.drawStr(0, 74, buffer);

#ifdef ENABLE_HV_INPUTS
    // Display voltage inputs
    sprintf(buffer, "V: %.2fV", (float)voltage.analogRead() * 3.3f / 4095.0f);
    display.u8g2.drawStr(0, 92, buffer);

    sprintf(buffer, "In1: %d In2: %d", input1.read() ? 1 : 0, input2.read() ? 1 : 0);
    display.u8g2.drawStr(0, 110, buffer);
#endif
  }

  // update function
  void ioTestScreenUpdate()
  {
    // Update all buttons
    BtnSel.Update();
    BtnPrev.Update();
    BtnNext.Update();
    BtnBoot.Update();

    // Single click on SEL button to go back
    if (BtnSel.clicks == 1)
    {
      screenManager.back();
    }

    // Triple click to reset button click counter
    if (BtnSel.clicks == 3)
    {
      lastButtonClicks = 0;
    }
  }

} // namespace IOTestScreenNamespace

// Define the IOTestScreen Screen2 instance
const Screen2 IOTestScreen = {
    F("IO Test"),
    F("IO Test"),
    IOTestScreenNamespace::ioTestScreenDraw,
    IOTestScreenNamespace::ioTestScreenUpdate,
    IOTestScreenNamespace::ioTestScreenOnEnter,
    IOTestScreenNamespace::ioTestScreenOnExit};

#endif