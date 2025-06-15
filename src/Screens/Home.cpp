#include "Home.h"

#include "IO/Display.h"
#include "IO/ScreenManager.h"

#include "IO/Menu.h"

#include "IO/GPIO.h"

#include "Screens/ApplicationScreen.h"
#include "Screens/SyncScreen.h"
#include "Screens/SettingsScreen.h"
#include "Screens/Debug/Debug.h"

namespace HomeScreenNamespace
{
  // Menu instance
  Menu menu = Menu(MenuSize::Medium);

  // Menu items
  MenuItemNavigate applicationMenuItem = MenuItemNavigate("Application", &ApplicationScreen);
  MenuItemNavigate syncMenuItem = MenuItemNavigate("Sync Manager", &SyncScreen);
  MenuItemNavigate settingsMenuItem = MenuItemNavigate("Settings", &SettingsScreen);

  MenuItemAction disableScreenItem = MenuItemAction(
      "Disable Screen", 1, []()
      {
        deviceInfo.oledEnabled = false;
        saveDeviceInfo();
        display.u8g2.clearBuffer();
        display.u8g2.sendBuffer();
        Serial.println("[INFO] [SCREEN] Screen disabled via menu"); });

  MenuItemAction powerOffItem = MenuItemAction(
      "Power Off", -1, []()
      {
        // Navigate to shutdown screen - assuming there's a shutdown screen
        // This will need to be adapted based on your actual screen setup
        // screenManager.setScreen("Shutdown");
      });

  // onEnter function
  void homeScreenOnEnter()
  {
    // Initialize the menu with menu items
    menu.addMenuItem(&applicationMenuItem);
    menu.addMenuItem(&syncMenuItem);
    menu.addMenuItem(&settingsMenuItem);
    menu.addMenuItem(&disableScreenItem);
    menu.addMenuItem(&powerOffItem);

    // Add route for settings as in the original code
    settingsMenuItem.addRoute(2, &DebugScreen);
  }

  // onExit function
  void homeScreenOnExit()
  {
    // Clean up allocated memory
  }

  // draw function
  void homeScreenDraw()
  {
    menu.draw();
  }

  // update function
  void homeScreenUpdate()
  {
    menu.update();
  }

} // namespace HomeScreenNamespace

// Define the HomeScreen Screen2 instance
const Screen2 HomeScreen = {
    F("Home"),
    F("Home"),
    HomeScreenNamespace::homeScreenDraw,
    HomeScreenNamespace::homeScreenUpdate,
    HomeScreenNamespace::homeScreenOnEnter,
    HomeScreenNamespace::homeScreenOnExit};