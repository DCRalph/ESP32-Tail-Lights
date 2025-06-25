#include "ProvisioningRequired.h"
#ifdef ENABLE_DISPLAY
#include "IO/Display.h"
#include "IO/GPIO.h"

namespace ProvisioningRequiredScreenNamespace
{
  // onEnter function
  void provisioningRequiredScreenOnEnter()
  {
    // Called when screen is entered
  }

  // onExit function
  void provisioningRequiredScreenOnExit()
  {
    // Clean up allocated memory
  }

  // draw function
  void provisioningRequiredScreenDraw()
  {
    display.noTopBar();

    display.u8g2.setFont(u8g2_font_logisoso16_tr);
    display.u8g2.setDrawColor(1);

    display.drawCenteredText(20, "Provisioning");
    display.drawCenteredText(40, "Required");

    // display.u8g2.setFont(u8g2_font_6x10_tf);
    // display.drawCenteredText(58, "Device must be provisioned");
  }

  // update function
  void provisioningRequiredScreenUpdate()
  {
    // No button handling - no way to leave this screen
    // The screen is intentionally locked until provisioning is complete
  }

} // namespace ProvisioningRequiredScreenNamespace

// Define the ProvisioningRequiredScreen Screen2 instance
const Screen2 ProvisioningRequiredScreen = {
    F("Provisioning Required"),
    F("Provisioning Required"),
    ProvisioningRequiredScreenNamespace::provisioningRequiredScreenDraw,
    ProvisioningRequiredScreenNamespace::provisioningRequiredScreenUpdate,
    ProvisioningRequiredScreenNamespace::provisioningRequiredScreenOnEnter,
    ProvisioningRequiredScreenNamespace::provisioningRequiredScreenOnExit};

#endif