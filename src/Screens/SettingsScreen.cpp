#include "SettingsScreen.h"
#include "IO/Display.h"
#include "IO/GPIO.h"
#include "IO/Menu.h"
#include "IO/LED/LEDStripManager.h"
#include "IO/StatusLed.h"
#include "IO/ScreenManager.h"

namespace SettingsScreenNamespace
{
  // Helper method forward declarations
  void updateDeviceInfo();
  void updateSystemStatus();
  void updateInputStatus();
  void updatePowerStatus();
  void performReboot();
  String formatUptime(uint32_t uptimeMs);
  String formatVoltage(float voltage);

  // Settings state variables
  static bool debugModeEnabled = false;
  static uint16_t headlightLedCount = 0;
  static uint16_t taillightLedCount = 0;
  static uint16_t underglowLedCount = 0;
  static uint16_t interiorLedCount = 0;

  // Timing for updates
  static uint64_t lastInfoUpdate = 0;

  // Menu instances
  static Menu menu = Menu(MenuSize::Medium);
  static Menu deviceInfoMenu = Menu(MenuSize::Small);
  static Menu systemSettingsMenu = Menu(MenuSize::Small);
  static Menu ledSettingsMenu = Menu(MenuSize::Small);
  static Menu inputSettingsMenu = Menu(MenuSize::Small);
  static Menu powerSettingsMenu = Menu(MenuSize::Small);

  // Main menu items
  static MenuItemBack backItem;
  static MenuItemSubmenu deviceInfoUIItem = MenuItemSubmenu("Device Info", &deviceInfoMenu);
  static MenuItemSubmenu systemSettingsUIItem = MenuItemSubmenu("System", &systemSettingsMenu);
  static MenuItemSubmenu ledSettingsUIItem = MenuItemSubmenu("LED Config", &ledSettingsMenu);
  static MenuItemSubmenu inputSettingsUIItem = MenuItemSubmenu("Inputs", &inputSettingsMenu);
  static MenuItemSubmenu powerSettingsUIItem = MenuItemSubmenu("Power", &powerSettingsMenu);

  // Device Info submenu items
  static MenuItemBack deviceInfoBackItem;
  static MenuItem deviceIdItem = MenuItem("ID: ---");
  static MenuItem deviceMacItem = MenuItem("MAC: ---");
  static MenuItem serialNumberItem = MenuItem("Serial: ---");
  static MenuItem hardwareVersionItem = MenuItem("HW Ver: ---");
  static MenuItem firmwareVersionItem = MenuItem("FW Ver: v1.0.0");
  static MenuItem uptimeItem = MenuItem("Uptime: ---");
  static MenuItem freeHeapItem = MenuItem("Free Heap: ---");

  // System Settings submenu items
  static MenuItemBack systemSettingsBackItem;
  static MenuItemToggle debugModeItem = MenuItemToggle("Debug Mode", &debugModeEnabled, true);
  static MenuItemAction rebootItem = MenuItemAction("Reboot", 1, []()
                                                    { performReboot(); });

  // LED Settings submenu items
  static MenuItemBack ledSettingsBackItem;
  static MenuItemNumber<uint16_t> headlightCountItem = MenuItemNumber<uint16_t>("H.Count", &headlightLedCount, 1, 300, 1);
  static MenuItemNumber<uint16_t> taillightCountItem = MenuItemNumber<uint16_t>("T.Count", &taillightLedCount, 1, 300, 1);
  static MenuItemNumber<uint16_t> underglowCountItem = MenuItemNumber<uint16_t>("U.Count", &underglowLedCount, 1, 300, 1);
  static MenuItemNumber<uint16_t> interiorCountItem = MenuItemNumber<uint16_t>("I.Count", &interiorLedCount, 1, 300, 1);

  // Input Settings submenu items
  static MenuItemBack inputSettingsBackItem;
  static MenuItem voltageItem = MenuItem("Voltage: ---");
  static MenuItem input1StatusItem = MenuItem("Input 1: ---");
  static MenuItem input2StatusItem = MenuItem("Input 2: ---");
  static MenuItem input3StatusItem = MenuItem("Input 3: ---");
  static MenuItem input4StatusItem = MenuItem("Input 4: ---");
  static MenuItem input5StatusItem = MenuItem("Input 5: ---");
  static MenuItem input6StatusItem = MenuItem("Input 6: ---");
  static MenuItem input7StatusItem = MenuItem("Input 7: ---");
  static MenuItem input8StatusItem = MenuItem("Input 8: ---");

  // Power Settings submenu items
  static MenuItemBack powerSettingsBackItem;
  static MenuItem batteryVoltageItem = MenuItem("Bat Volt: ---");

  // onEnter function
  void settingsScreenOnEnter()
  {
    // Setup main settings menu
    menu.addMenuItem(&backItem);
    menu.addMenuItem(&deviceInfoUIItem);
    menu.addMenuItem(&systemSettingsUIItem);
    menu.addMenuItem(&ledSettingsUIItem);
    menu.addMenuItem(&inputSettingsUIItem);
    menu.addMenuItem(&powerSettingsUIItem);

    // Setup Device Info submenu
    deviceInfoMenu.addMenuItem(&deviceInfoBackItem);
    deviceInfoMenu.addMenuItem(&deviceIdItem);
    deviceInfoMenu.addMenuItem(&deviceMacItem);
    deviceInfoMenu.addMenuItem(&serialNumberItem);
    deviceInfoMenu.addMenuItem(&hardwareVersionItem);
    deviceInfoMenu.addMenuItem(&firmwareVersionItem);
    deviceInfoMenu.addMenuItem(&uptimeItem);
    deviceInfoMenu.addMenuItem(&freeHeapItem);
    deviceInfoMenu.setParentMenu(&menu);

    // Setup System Settings submenu
    systemSettingsMenu.addMenuItem(&systemSettingsBackItem);
    systemSettingsMenu.addMenuItem(&debugModeItem);
    systemSettingsMenu.addMenuItem(&rebootItem);
    systemSettingsMenu.setParentMenu(&menu);

    // Setup LED Settings submenu
    ledSettingsMenu.addMenuItem(&ledSettingsBackItem);
    ledSettingsMenu.addMenuItem(&headlightCountItem);
    ledSettingsMenu.addMenuItem(&taillightCountItem);
    ledSettingsMenu.addMenuItem(&underglowCountItem);
    ledSettingsMenu.addMenuItem(&interiorCountItem);
    ledSettingsMenu.setParentMenu(&menu);

    // Setup Input Settings submenu
    inputSettingsMenu.addMenuItem(&inputSettingsBackItem);
    inputSettingsMenu.addMenuItem(&voltageItem);
    inputSettingsMenu.addMenuItem(&input1StatusItem);
    inputSettingsMenu.addMenuItem(&input2StatusItem);
    inputSettingsMenu.addMenuItem(&input3StatusItem);
    inputSettingsMenu.addMenuItem(&input4StatusItem);
    inputSettingsMenu.addMenuItem(&input5StatusItem);
    inputSettingsMenu.addMenuItem(&input6StatusItem);
    inputSettingsMenu.addMenuItem(&input7StatusItem);
    inputSettingsMenu.addMenuItem(&input8StatusItem);
    inputSettingsMenu.setParentMenu(&menu);

    // Setup Power Settings submenu
    powerSettingsMenu.addMenuItem(&powerSettingsBackItem);
    powerSettingsMenu.addMenuItem(&batteryVoltageItem);
    powerSettingsMenu.setParentMenu(&menu);

    // Set up callbacks for entering submenus
    deviceInfoUIItem.addFunc(1, []()
                             { updateDeviceInfo(); });
    systemSettingsUIItem.addFunc(1, []()
                                 { updateSystemStatus(); });
    inputSettingsUIItem.addFunc(1, []()
                                { updateInputStatus(); });
    powerSettingsUIItem.addFunc(1, []()
                                { updatePowerStatus(); });

    // Set up change callbacks
    debugModeItem.setOnChange([]()
                              {
      deviceInfo.debugEnabled = debugModeEnabled;
      saveDeviceInfo();
      display.showNotification("Debug mode updated", 1000); });

    // Initialize data
    updateDeviceInfo();
    updateSystemStatus();

#ifdef HEADLIGHT_LED_COUNT
    headlightLedCount = HEADLIGHT_LED_COUNT;
#endif
#ifdef TAILLIGHT_LED_COUNT
    taillightLedCount = TAILLIGHT_LED_COUNT;
#endif
#ifdef UNDERGLOW_LED_COUNT
    underglowLedCount = UNDERGLOW_LED_COUNT;
#endif
#ifdef INTERIOR_LED_COUNT
    interiorLedCount = INTERIOR_LED_COUNT;
#endif
  }

  // onExit function
  void settingsScreenOnExit()
  {
    // Clean up allocated memory
  }

  // draw function
  void settingsScreenDraw()
  {
    menu.draw();
  }

  // update function
  void settingsScreenUpdate()
  {
    menu.update();

    // Update info periodically
    if (millis() - lastInfoUpdate > 1000)
    {
      lastInfoUpdate = millis();

      // Update dynamic info if we're in the relevant submenus
      Menu *currentMenu = menu.getActiveSubmenu();
      if (currentMenu == &deviceInfoMenu)
      {
        updateDeviceInfo();
      }
      else if (currentMenu == &inputSettingsMenu)
      {
        updateInputStatus();
      }
      else if (currentMenu == &powerSettingsMenu)
      {
        updatePowerStatus();
      }
    }
  }

  // Helper method implementations
  void updateDeviceInfo()
  {
    deviceIdItem.setName("ID: " + String(deviceInfo.serialNumber));

    String macStr = "";
    for (int i = 0; i < 6; i++)
    {
      if (deviceInfo.macAddress[i] < 16)
        macStr += "0";
      macStr += String(deviceInfo.macAddress[i], HEX);
      if (i < 5)
        macStr += ":";
    }
    macStr.toUpperCase();
    deviceMacItem.setName("MAC: " + macStr);

    serialNumberItem.setName("Serial: " + String(deviceInfo.serialNumber));
    hardwareVersionItem.setName("HW Ver: " + String(deviceInfo.hardwareVersion));
    uptimeItem.setName("Uptime: " + formatUptime(millis()));
    freeHeapItem.setName("Free Heap: " + formatBytes(ESP.getFreeHeap(), true));
  }

  void updateSystemStatus()
  {
    debugModeEnabled = deviceInfo.debugEnabled;
  }

  void updateInputStatus()
  {
    voltageItem.setName("Voltage: " + formatVoltage(batteryGetVoltage()));

#ifdef ENABLE_HV_INPUTS
    input1StatusItem.setName("Input 1: " + String(input1.read() ? "HIGH" : "LOW"));
    input2StatusItem.setName("Input 2: " + String(input2.read() ? "HIGH" : "LOW"));
    input3StatusItem.setName("Input 3: " + String(input3.read() ? "HIGH" : "LOW"));
    input4StatusItem.setName("Input 4: " + String(input4.read() ? "HIGH" : "LOW"));
    input5StatusItem.setName("Input 5: " + String(input5.read() ? "HIGH" : "LOW"));
    input6StatusItem.setName("Input 6: " + String(input6.read() ? "HIGH" : "LOW"));
#else
    input1StatusItem.setHidden(true);
    input2StatusItem.setHidden(true);
    input3StatusItem.setHidden(true);
    input4StatusItem.setHidden(true);
    input5StatusItem.setHidden(true);
    input6StatusItem.setHidden(true);
    input7StatusItem.setHidden(true);
    input8StatusItem.setHidden(true);
#endif
  }

  void updatePowerStatus()
  {
    batteryVoltageItem.setName("Bat Volt: " + formatVoltage(batteryGetVoltage()));
  }

  void performReboot()
  {
    display.showNotification("Rebooting...", 2000);
    delay(1500);
    restart();
  }

  String formatUptime(uint32_t uptimeMs)
  {
    uint32_t seconds = uptimeMs / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    uint32_t days = hours / 24;

    if (days > 0)
    {
      return String(days) + "d " + String(hours % 24) + "h";
    }
    else if (hours > 0)
    {
      return String(hours) + "h " + String(minutes % 60) + "m";
    }
    else if (minutes > 0)
    {
      return String(minutes) + "m " + String(seconds % 60) + "s";
    }
    else
    {
      return String(seconds) + "s";
    }
  }

  String formatVoltage(float voltage)
  {
    // Voltage is float in volts
    char buffer[16];
    sprintf(buffer, "%.2fV", voltage);
    return String(buffer);
  }

} // namespace SettingsScreenNamespace

// Define the SettingsScreen Screen2 instance
const Screen2 SettingsScreen = {
    F("Settings"),
    F("Settings"),
    SettingsScreenNamespace::settingsScreenDraw,
    SettingsScreenNamespace::settingsScreenUpdate,
    SettingsScreenNamespace::settingsScreenOnEnter,
    SettingsScreenNamespace::settingsScreenOnExit};