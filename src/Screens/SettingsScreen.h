#pragma once

#include "config.h"
#include "IO/Display.h"
#include "IO/GPIO.h"
#include "IO/Menu.h"
#include "IO/LED/LEDStripManager.h"
#include "IO/StatusLed.h"

class SettingsScreen : public Screen
{
public:
  SettingsScreen(String _name);

  Menu menu = Menu(MenuSize::Medium);
  MenuItemBack backItem;

  // Device Info section
  MenuItemSubmenu deviceInfoUIItem = MenuItemSubmenu("Device Info", &deviceInfoMenu);
  Menu deviceInfoMenu = Menu(MenuSize::Small);
  MenuItemBack deviceInfoBackItem;
  MenuItem deviceIdItem = MenuItem("ID: ---");
  MenuItem deviceMacItem = MenuItem("MAC: ---");
  MenuItem serialNumberItem = MenuItem("Serial: ---");
  MenuItem hardwareVersionItem = MenuItem("HW Ver: ---");
  MenuItem firmwareVersionItem = MenuItem("FW Ver: v1.0.0");
  MenuItem uptimeItem = MenuItem("Uptime: ---");
  MenuItem freeHeapItem = MenuItem("Free Heap: ---");

  // System Settings section
  MenuItemSubmenu systemSettingsUIItem = MenuItemSubmenu("System", &systemSettingsMenu);
  Menu systemSettingsMenu = Menu(MenuSize::Small);
  MenuItemBack systemSettingsBackItem;
  MenuItemToggle debugModeItem = MenuItemToggle("Debug Mode", &debugModeEnabled, true);
  MenuItemAction rebootItem = MenuItemAction("Reboot", 1, [&]()
                                             { this->performReboot(); });

  // LED Settings section
  MenuItemSubmenu ledSettingsUIItem = MenuItemSubmenu("LED Config", &ledSettingsMenu);
  Menu ledSettingsMenu = Menu(MenuSize::Small);
  MenuItemBack ledSettingsBackItem;
  MenuItemNumber<uint16_t> headlightCountItem = MenuItemNumber<uint16_t>("H.Count", &headlightLedCount, 1, 300, 1);
  MenuItemNumber<uint16_t> taillightCountItem = MenuItemNumber<uint16_t>("T.Count", &taillightLedCount, 1, 300, 1);
  MenuItemNumber<uint16_t> underglowCountItem = MenuItemNumber<uint16_t>("U.Count", &underglowLedCount, 1, 300, 1);
  MenuItemNumber<uint16_t> interiorCountItem = MenuItemNumber<uint16_t>("I.Count", &interiorLedCount, 1, 300, 1);

  // Input Settings section
  MenuItemSubmenu inputSettingsUIItem = MenuItemSubmenu("Inputs", &inputSettingsMenu);
  Menu inputSettingsMenu = Menu(MenuSize::Small);
  MenuItemBack inputSettingsBackItem;
  MenuItem voltageItem = MenuItem("Voltage: ---");
  MenuItem input1StatusItem = MenuItem("Input 1: ---");
  MenuItem input2StatusItem = MenuItem("Input 2: ---");
  MenuItem input3StatusItem = MenuItem("Input 3: ---");
  MenuItem input4StatusItem = MenuItem("Input 4: ---");
  MenuItem input5StatusItem = MenuItem("Input 5: ---");
  MenuItem input6StatusItem = MenuItem("Input 6: ---");
  MenuItem input7StatusItem = MenuItem("Input 7: ---");
  MenuItem input8StatusItem = MenuItem("Input 8: ---");

  // Battery/Power section
  MenuItemSubmenu powerSettingsUIItem = MenuItemSubmenu("Power", &powerSettingsMenu);
  Menu powerSettingsMenu = Menu(MenuSize::Small);
  MenuItemBack powerSettingsBackItem;
  MenuItem batteryVoltageItem = MenuItem("Bat Volt: ---");

  void draw() override;
  void update() override;
  void onEnter() override;

private:
  // Settings state variables
  bool debugModeEnabled = false;
  uint16_t headlightLedCount = 0;
  uint16_t taillightLedCount = 0;
  uint16_t underglowLedCount = 0;
  uint16_t interiorLedCount = 0;

  // Timing for updates
  uint64_t lastInfoUpdate = 0;

  // Helper methods
  void updateDeviceInfo();
  void updateSystemStatus();
  void updateInputStatus();
  void updatePowerStatus();

  void performReboot();
  String formatUptime(uint32_t uptimeMs);
  String formatVoltage(float voltage);
};

SettingsScreen::SettingsScreen(String _name) : Screen(_name)
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
  deviceInfoUIItem.addFunc(1, [&]()
                           { updateDeviceInfo(); });

  systemSettingsUIItem.addFunc(1, [&]()
                               { updateSystemStatus(); });

  inputSettingsUIItem.addFunc(1, [&]()
                              { updateInputStatus(); });

  powerSettingsUIItem.addFunc(1, [&]()
                              { updatePowerStatus(); });

  // Set up change callbacks
  debugModeItem.setOnChange([&]()
                            {
                              deviceInfo.debugEnabled = debugModeEnabled;
                              saveDeviceInfo();
                              display.showNotification("Debug mode updated", 1000); });
}

void SettingsScreen::draw()
{
  menu.draw();
}

void SettingsScreen::update()
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

void SettingsScreen::onEnter()
{

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

// Helper methods implementation
void SettingsScreen::updateDeviceInfo()
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

void SettingsScreen::updateSystemStatus()
{
  debugModeEnabled = deviceInfo.debugEnabled;
}

void SettingsScreen::updateInputStatus()
{
  voltageItem.setName("Voltage: " + formatVoltage(batteryGetVoltage()));

#ifdef ENABLE_HV_INPUTS
  // input1StatusItem.setName("Input 1: " + String(input1.read() ? "HIGH" : "LOW"));
  // input2StatusItem.setName("Input 2: " + String(input2.read() ? "HIGH" : "LOW"));
  // input3StatusItem.setName("Input 3: " + String(input3.read() ? "HIGH" : "LOW"));
  // input4StatusItem.setName("Input 4: " + String(input4.read() ? "HIGH" : "LOW"));
  // input5StatusItem.setName("Input 5: " + String(input5.read() ? "HIGH" : "LOW"));
  // input6StatusItem.setName("Input 6: " + String(input6.read() ? "HIGH" : "LOW"));

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

void SettingsScreen::updatePowerStatus()
{
  batteryVoltageItem.setName("Bat Volt: " + formatVoltage(batteryGetVoltage()));
}

void SettingsScreen::performReboot()
{
  display.showNotification("Rebooting...", 2000);
  delay(1500);
  restart();
}

String SettingsScreen::formatUptime(uint32_t uptimeMs)
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

String SettingsScreen::formatVoltage(float voltage)
{
  // Voltage is float in volts
  char buffer[16];
  sprintf(buffer, "%.2fV", voltage);
  return String(buffer);
}