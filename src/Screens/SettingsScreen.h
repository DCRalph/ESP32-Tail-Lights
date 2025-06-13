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
  MenuItemNumber<uint8_t> brightnessItem = MenuItemNumber<uint8_t>("Brightness", &brightnessValue, 0, 255, 10);
  MenuItemAction factoryResetItem = MenuItemAction("Factory Reset", 3, [&]()
                                                   { this->performFactoryReset(); });
  MenuItemAction rebootItem = MenuItemAction("Reboot", 1, [&]()
                                             { this->performReboot(); });

  // WiFi Settings section
  MenuItemSubmenu wifiSettingsUIItem = MenuItemSubmenu("WiFi", &wifiSettingsMenu);
  Menu wifiSettingsMenu = Menu(MenuSize::Small);
  MenuItemBack wifiSettingsBackItem;
  MenuItem wifiStatusItem = MenuItem("Status: ---");
  MenuItem wifiChannelItem = MenuItem("Channel: " + String(ESP_NOW_CHANNEL));
  MenuItemAction wifiScanItem = MenuItemAction("Scan Networks", 1, [&]()
                                               { this->performWifiScan(); });

  // LED Settings section
  MenuItemSubmenu ledSettingsUIItem = MenuItemSubmenu("LED Config", &ledSettingsMenu);
  Menu ledSettingsMenu = Menu(MenuSize::Small);
  MenuItemBack ledSettingsBackItem;
  MenuItemNumber<uint16_t> headlightCountItem = MenuItemNumber<uint16_t>("H.Count", &headlightLedCount, 1, 300, 1);
  MenuItemNumber<uint16_t> taillightCountItem = MenuItemNumber<uint16_t>("T.Count", &taillightLedCount, 1, 300, 1);
  MenuItemToggle headlightFlippedItem = MenuItemToggle("H.Flipped", &headlightFlipped, true);

  // Input Settings section
  MenuItemSubmenu inputSettingsUIItem = MenuItemSubmenu("Inputs", &inputSettingsMenu);
  Menu inputSettingsMenu = Menu(MenuSize::Small);
  MenuItemBack inputSettingsBackItem;
  MenuItem input1StatusItem = MenuItem("Input 1: ---");
  MenuItem input2StatusItem = MenuItem("Input 2: ---");
  MenuItem input3StatusItem = MenuItem("Input 3: ---");
  MenuItem input4StatusItem = MenuItem("Input 4: ---");
  MenuItem input5StatusItem = MenuItem("Input 5: ---");
  MenuItem input6StatusItem = MenuItem("Input 6: ---");
  MenuItem voltageItem = MenuItem("Voltage: ---");

  // Battery/Power section
  MenuItemSubmenu powerSettingsUIItem = MenuItemSubmenu("Power", &powerSettingsMenu);
  Menu powerSettingsMenu = Menu(MenuSize::Small);
  MenuItemBack powerSettingsBackItem;
  MenuItem batteryVoltageItem = MenuItem("Bat Volt: ---");
  MenuItem powerConsumptionItem = MenuItem("Power: ---");
  MenuItemNumber<uint16_t> lowVoltageThresholdItem = MenuItemNumber<uint16_t>("Low V Thr", &lowVoltageThreshold, 1000, 1600, 10);
  MenuItemToggle lowPowerModeItem = MenuItemToggle("Low Power", &lowPowerModeEnabled, true);

  // Provisioning section
  MenuItemSubmenu provisioningUIItem = MenuItemSubmenu("Provisioning", &provisioningMenu);
  Menu provisioningMenu = Menu(MenuSize::Small);
  MenuItemBack provisioningBackItem;
  MenuItem provisionedStatusItem = MenuItem("Status: ---");
  MenuItemAction forceProvisioningItem = MenuItemAction("Force Prov", 1, [&]()
                                                        { this->forceProvisioning(); });
  MenuItemAction completeProvisioningItem = MenuItemAction("Complete", 1, [&]()
                                                           { this->completeProvisioning(); });

  void draw() override;
  void update() override;
  void onEnter() override;

private:
  // Settings state variables
  bool debugModeEnabled = false;
  uint8_t brightnessValue = 255;
  uint16_t headlightLedCount = HEADLIGHT_LED_COUNT;
  uint16_t taillightLedCount = TAILLIGHT_LED_COUNT;
  bool headlightFlipped = false;
  bool statusLedsEnabled = true;
  uint16_t lowVoltageThreshold = 1200; // 12.0V in centivolt
  bool lowPowerModeEnabled = false;

  // Timing for updates
  uint64_t lastInfoUpdate = 0;

  // Helper methods
  void updateDeviceInfo();
  void updateSystemStatus();
  void updateWifiStatus();
  void updateInputStatus();
  void updatePowerStatus();
  void updateProvisioningStatus();
  void loadSettings();
  void saveSettings();
  void performFactoryReset();
  void performReboot();
  void performWifiScan();
  void forceProvisioning();
  void completeProvisioning();
  String formatUptime(uint32_t uptimeMs);
  String formatVoltage(uint16_t voltage);
};

SettingsScreen::SettingsScreen(String _name) : Screen(_name)
{
  // Setup main settings menu
  menu.addMenuItem(&backItem);
  menu.addMenuItem(&deviceInfoUIItem);
  menu.addMenuItem(&systemSettingsUIItem);
  menu.addMenuItem(&wifiSettingsUIItem);
  menu.addMenuItem(&ledSettingsUIItem);
  menu.addMenuItem(&inputSettingsUIItem);
  menu.addMenuItem(&powerSettingsUIItem);
  menu.addMenuItem(&provisioningUIItem);

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
  systemSettingsMenu.addMenuItem(&brightnessItem);
  systemSettingsMenu.addMenuItem(&factoryResetItem);
  systemSettingsMenu.addMenuItem(&rebootItem);
  systemSettingsMenu.setParentMenu(&menu);

  // Setup WiFi Settings submenu
  wifiSettingsMenu.addMenuItem(&wifiSettingsBackItem);
  wifiSettingsMenu.addMenuItem(&wifiStatusItem);
  wifiSettingsMenu.addMenuItem(&wifiChannelItem);
  wifiSettingsMenu.addMenuItem(&wifiScanItem);
  wifiSettingsMenu.setParentMenu(&menu);

  // Setup LED Settings submenu
  ledSettingsMenu.addMenuItem(&ledSettingsBackItem);
  ledSettingsMenu.addMenuItem(&headlightCountItem);
  ledSettingsMenu.addMenuItem(&taillightCountItem);
  ledSettingsMenu.addMenuItem(&headlightFlippedItem);
  ledSettingsMenu.setParentMenu(&menu);

  // Setup Input Settings submenu
  inputSettingsMenu.addMenuItem(&inputSettingsBackItem);
  inputSettingsMenu.addMenuItem(&input1StatusItem);
  inputSettingsMenu.addMenuItem(&input2StatusItem);
  inputSettingsMenu.addMenuItem(&input3StatusItem);
  inputSettingsMenu.addMenuItem(&input4StatusItem);
  inputSettingsMenu.addMenuItem(&input5StatusItem);
  inputSettingsMenu.addMenuItem(&input6StatusItem);
  inputSettingsMenu.addMenuItem(&voltageItem);
  inputSettingsMenu.setParentMenu(&menu);

  // Setup Power Settings submenu
  powerSettingsMenu.addMenuItem(&powerSettingsBackItem);
  powerSettingsMenu.addMenuItem(&batteryVoltageItem);
  powerSettingsMenu.addMenuItem(&powerConsumptionItem);
  powerSettingsMenu.addMenuItem(&lowVoltageThresholdItem);
  powerSettingsMenu.addMenuItem(&lowPowerModeItem);
  powerSettingsMenu.setParentMenu(&menu);

  // Setup Provisioning submenu
  provisioningMenu.addMenuItem(&provisioningBackItem);
  provisioningMenu.addMenuItem(&provisionedStatusItem);
  provisioningMenu.addMenuItem(&forceProvisioningItem);
  provisioningMenu.addMenuItem(&completeProvisioningItem);
  provisioningMenu.setParentMenu(&menu);

  // Set up callbacks for entering submenus
  deviceInfoUIItem.addFunc(1, [&]()
                           { updateDeviceInfo(); });

  systemSettingsUIItem.addFunc(1, [&]()
                               { updateSystemStatus(); });

  wifiSettingsUIItem.addFunc(1, [&]()
                             { updateWifiStatus(); });

  inputSettingsUIItem.addFunc(1, [&]()
                              { updateInputStatus(); });

  powerSettingsUIItem.addFunc(1, [&]()
                              { updatePowerStatus(); });

  provisioningUIItem.addFunc(1, [&]()
                             { updateProvisioningStatus(); });

  // Set up change callbacks
  debugModeItem.setOnChange([&]()
                            {
                              deviceInfo.debugEnabled = debugModeEnabled;
                              saveDeviceInfo();
                              display.showNotification("Debug mode updated", 1000); });

  brightnessItem.setOnChange([&]()
                             {
                               LEDStripManager::getInstance()->setBrightness(brightnessValue);
                               statusLeds.setBrightness(brightnessValue);
                               preferences.putUChar("brightness", brightnessValue);
                               display.showNotification("Brightness updated", 1000); });

  headlightCountItem.setOnChange([&]()
                                 {
                                   preferences.putUShort("h_led_count", headlightLedCount);
                                   display.showNotification("Headlight count updated", 1000); });

  taillightCountItem.setOnChange([&]()
                                 {
                                   preferences.putUShort("t_led_count", taillightLedCount);
                                   display.showNotification("Taillight count updated", 1000); });

  headlightFlippedItem.setOnChange([&]()
                                   {
                                     preferences.putBool("h_flipped", headlightFlipped);
                                     display.showNotification("Headlight flip updated", 1000); });

  lowVoltageThresholdItem.setOnChange([&]()
                                      {
                                        preferences.putUShort("low_v_thr", lowVoltageThreshold);
                                        display.showNotification("Low voltage threshold updated", 1000); });

  lowPowerModeItem.setOnChange([&]()
                               {
                                 preferences.putBool("low_power", lowPowerModeEnabled);
                                 display.showNotification("Low power mode updated", 1000); });

  // Enable fast update for brightness slider
  brightnessItem.setFastUpdate(true);

  // Load settings on initialization
  loadSettings();
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
    else if (currentMenu == &wifiSettingsMenu)
    {
      updateWifiStatus();
    }
  }
}

void SettingsScreen::onEnter()
{
  loadSettings();
  updateDeviceInfo();
  updateSystemStatus();
  updateWifiStatus();
  updateProvisioningStatus();
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

void SettingsScreen::updateWifiStatus()
{
  bool isConnected = false;
  wifiStatusItem.setName("Status: " + String(isConnected ? "Connected" : "Disconnected"));
}

void SettingsScreen::updateInputStatus()
{
#ifdef ENABLE_HV_INPUTS
  input1StatusItem.setName("Input 1: " + String(input1.read() ? "HIGH" : "LOW"));
  input2StatusItem.setName("Input 2: " + String(input2.read() ? "HIGH" : "LOW"));
  input3StatusItem.setName("Input 3: " + String(input3.read() ? "HIGH" : "LOW"));
  input4StatusItem.setName("Input 4: " + String(input4.read() ? "HIGH" : "LOW"));
  input5StatusItem.setName("Input 5: " + String(input5.read() ? "HIGH" : "LOW"));
  input6StatusItem.setName("Input 6: " + String(input6.read() ? "HIGH" : "LOW"));

  // Read voltage if available
  if (1)
  {
    voltageItem.setName("Voltage: " + formatVoltage(500));
  }
  else
  {
    voltageItem.setName("Voltage: N/A");
  }
#else
  input1StatusItem.setHidden(true);
  input2StatusItem.setHidden(true);
  input3StatusItem.setHidden(true);
  input4StatusItem.setHidden(true);
  input5StatusItem.setHidden(true);
  input6StatusItem.setHidden(true);
  voltageItem.setHidden(true);
#endif
}

void SettingsScreen::updatePowerStatus()
{
  // This would depend on if you have battery monitoring implemented
  batteryVoltageItem.setName("Bat Volt: N/A");
  powerConsumptionItem.setName("Power: N/A");
}

void SettingsScreen::updateProvisioningStatus()
{
  provisionedStatusItem.setName("Status: " + String(deviceInfo.provisioned ? "Provisioned" : "Not Provisioned"));

  // Hide complete provisioning if already provisioned
  completeProvisioningItem.setHidden(deviceInfo.provisioned);
}

void SettingsScreen::loadSettings()
{
  debugModeEnabled = deviceInfo.debugEnabled;
  brightnessValue = preferences.getUChar("brightness", 255);
  headlightLedCount = preferences.getUShort("h_led_count", HEADLIGHT_LED_COUNT);
  taillightLedCount = preferences.getUShort("t_led_count", TAILLIGHT_LED_COUNT);
  headlightFlipped = preferences.getBool("h_flipped", false);
  statusLedsEnabled = preferences.getBool("status_leds", true);
  lowVoltageThreshold = preferences.getUShort("low_v_thr", 1200);
  lowPowerModeEnabled = preferences.getBool("low_power", false);
}

void SettingsScreen::saveSettings()
{
  preferences.putUChar("brightness", brightnessValue);
  preferences.putUShort("h_led_count", headlightLedCount);
  preferences.putUShort("t_led_count", taillightLedCount);
  preferences.putBool("h_flipped", headlightFlipped);
  preferences.putBool("status_leds", statusLedsEnabled);
  preferences.putUShort("low_v_thr", lowVoltageThreshold);
  preferences.putBool("low_power", lowPowerModeEnabled);
}

void SettingsScreen::performFactoryReset()
{
  display.showNotification("Factory reset...", 2000);

  // Clear all preferences
  preferences.clear();

  // Reset device info
  deviceInfo.provisioned = false;
  deviceInfo.debugEnabled = false;
  saveDeviceInfo();

  delay(2000);
  restart();
}

void SettingsScreen::performReboot()
{
  display.showNotification("Rebooting...", 1500);
  delay(1500);
  restart();
}

void SettingsScreen::performWifiScan()
{
  display.showNotification("Scanning WiFi...", 1500);

  int networks = WiFi.scanNetworks();

  if (networks > 0)
  {
    display.showNotification("Found " + String(networks) + " networks", 2000);
  }
  else
  {
    display.showNotification("No networks found", 1500);
  }
}

void SettingsScreen::forceProvisioning()
{
  display.showNotification("Forcing provisioning...", 1500);
  deviceInfo.provisioned = false;
  saveDeviceInfo();
  delay(1500);
  restart();
}

void SettingsScreen::completeProvisioning()
{
  if (!deviceInfo.provisioned)
  {
    deviceInfo.provisioned = true;
    saveDeviceInfo();
    display.showNotification("Provisioning completed", 1500);
    updateProvisioningStatus();
  }
  else
  {
    display.showNotification("Already provisioned", 1500);
  }
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

String SettingsScreen::formatVoltage(uint16_t voltage)
{
  // Voltage is in centivolt (1/100V), so 1234 = 12.34V
  return String(voltage / 100) + "." + String((voltage % 100) / 10) + String(voltage % 10) + "V";
}