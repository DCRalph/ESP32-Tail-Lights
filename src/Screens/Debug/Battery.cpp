#include "Battery.h"
#include "IO/Display.h"
#include "IO/GPIO.h"
#include "IO/Battery.h"
#include "IO/ScreenManager.h"

namespace BatteryScreenNamespace
{
  // onEnter function
  void batteryScreenOnEnter()
  {
    // Called when screen is entered
  }

  // onExit function
  void batteryScreenOnExit()
  {
    // Clean up allocated memory
  }

  // draw function
  void batteryScreenDraw()
  {
    display.u8g2.setFont(u8g2_font_logisoso16_tf);
    display.u8g2.setDrawColor(1);

    char buffer[32];
    sprintf(buffer, "%.2fV. %d%", batteryGetVoltage(), batteryGetPercentage());
    display.u8g2.drawStr(0, 28, buffer);

    sprintf(buffer, "%.2fV. %d%", batteryGetVoltageSmooth(), batteryGetPercentageSmooth());
    display.u8g2.drawStr(0, 46, buffer);
  }

  // update function
  void batteryScreenUpdate()
  {
    if (BtnSel.clicks != 0)
    {
      screenManager.back();
    }
  }

} // namespace BatteryScreenNamespace

// Define the BatteryScreen Screen2 instance
const Screen2 BatteryScreen = {
    F("Battery"),
    F("Battery"),
    BatteryScreenNamespace::batteryScreenDraw,
    BatteryScreenNamespace::batteryScreenUpdate,
    BatteryScreenNamespace::batteryScreenOnEnter,
    BatteryScreenNamespace::batteryScreenOnExit};