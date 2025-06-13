/****************************************************
 * provisioningMenu.cpp
 *
 * Implementation of the provisioning menu system.
 * This menu allows configuration of device parameters
 * before normal operation is enabled.
 ****************************************************/

#include "provisioningMenu.h"
#include "config.h"
#include "SerialMenu.h"
#include <Arduino.h>
#include "SerialMenus/mainMenu.h"

/****************************************************
 * provisioningMenu object
 ****************************************************/
SerialMenu provisioningMenu = {
    "Provisioning",
    nullptr,
    printProvisioningMenu,
    handleProvisioningMenuInput};

/****************************************************
 * printProvisioningMenu()
 ****************************************************/
void printProvisioningMenu(const SerialMenu &menu)
{
  Serial.println();
  Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
  Serial.println(F("║                      DEVICE PROVISIONING                    ║"));
  Serial.println(F("╠══════════════════════════════════════════════════════════════╣"));

  if (!deviceInfo.provisioned)
  {
    Serial.println(F("║  This device requires provisioning before it can be used.   ║"));
    Serial.println(F("║  Please configure the device parameters below.              ║"));
  }
  else
  {
    Serial.println(F("║  Device is provisioned. You can reconfigure it below.      ║"));
    Serial.println(F("║  Warning: Changes may require device restart.              ║"));
  }

  Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
  Serial.println();

  Serial.println(F("=== PROVISIONING MENU ==="));
  Serial.println(F("1) Show Current Status"));
  Serial.println(F("2) Set Serial Number"));
  Serial.println(F("3) Set Hardware Version"));
  Serial.println(F("4) Toggle Debug Mode"));

  if (!deviceInfo.provisioned)
  {
    Serial.println(F("5) Complete Provisioning"));
  }
  else
  {
    Serial.println(F("5) Mark as Unprovisioned"));
  }

  Serial.println(F("9) Factory Reset"));
  Serial.println(F("h) Help"));

  if (deviceInfo.provisioned)
  {
    Serial.println(F("b) Back to Main Menu"));
  }

  Serial.println(F("Press Enter to re-print this menu"));
  Serial.println();
}

/****************************************************
 * handleProvisioningMenuInput()
 ****************************************************/
bool handleProvisioningMenuInput(SerialMenu &menu, const String &input)
{
  if (input == F("1"))
  {
    showProvisioningStatus();
    return true;
  }
  else if (input == F("2"))
  {
    String serialStr = promptUserInput(F("Enter Serial Number (1-4294967295):"), 30000);

    if (serialStr.length() > 0)
    {
      uint32_t serialNum = serialStr.toInt();

      if (serialNum > 0)
      {
        setSerialNumber(serialNum);
        Serial.println(F("Serial number set successfully!"));
      }
      else
      {
        Serial.println(F("Invalid serial number. Must be greater than 0."));
      }
    }
    return true;
  }
  else if (input == F("3"))
  {
    String versionStr = promptUserInput(F("Enter Hardware Version (1-65535):"), 30000);

    if (versionStr.length() > 0)
    {
      uint16_t version = versionStr.toInt();

      if (version > 0)
      {
        setHardwareVersion(version);
        Serial.println(F("Hardware version set successfully!"));
      }
      else
      {
        Serial.println(F("Invalid hardware version. Must be greater than 0."));
      }
    }
    return true;
  }
  else if (input == F("4"))
  {
    deviceInfo.debugEnabled = !deviceInfo.debugEnabled;
    enableDebugMode(deviceInfo.debugEnabled);
    Serial.println(String(F("Debug mode ")) + (deviceInfo.debugEnabled ? "ENABLED" : "DISABLED"));
    return true;
  }
  else if (input == F("5"))
  {
    if (!deviceInfo.provisioned)
    {
      // Complete provisioning
      if (deviceInfo.serialNumber == 0 || deviceInfo.hardwareVersion == 0)
      {
        Serial.println(F("ERROR: Cannot complete provisioning!"));
        Serial.println(F("Serial number and hardware version must be set first."));
        return true;
      }

      String confirm = promptUserInput(F("Are you sure you want to complete provisioning? (y/N)"), 10000);
      confirm.toLowerCase();

      if (confirm.length() > 0 && (confirm == "y" || confirm == "yes"))
      {
        completeProvisioning();
        Serial.println(F("Device provisioning completed! Restarting..."));
        delay(2000);
        restart();
      }
      else
      {
        Serial.println(F("Provisioning cancelled."));
      }
    }
    else
    {
      // Mark as unprovisioned
      Serial.println(F("WARNING: This will mark the device as unprovisioned!"));
      String confirm = promptUserInput(F("Are you sure? (y/N)"), 10000);
      confirm.toLowerCase();

      if (confirm.length() > 0 && (confirm == "y" || confirm == "yes"))
      {
        deviceInfo.provisioned = false;
        saveDeviceInfo();
        Serial.println(F("Device marked as unprovisioned. Restarting..."));
        delay(2000);
        restart();
      }
      else
      {
        Serial.println(F("Operation cancelled."));
      }
    }
    return true;
  }
  else if (input == F("9"))
  {
    Serial.println(F("WARNING: This will erase all device configuration!"));
    String confirm = promptUserInput(F("Type 'FACTORY RESET' to confirm:"), 30000);

    if (confirm.length() > 0 && confirm == "FACTORY RESET")
    {
      factoryReset();
      Serial.println(F("Factory reset completed! Restarting..."));
      delay(2000);
      restart();
    }
    else
    {
      Serial.println(F("Incorrect confirmation. Factory reset cancelled."));
    }
    return true;
  }
  else if (input == F("h"))
  {
    Serial.println();
    Serial.println(F("=== PROVISIONING HELP ==="));
    Serial.println(F("This device must be provisioned before normal operation."));
    Serial.println(F("Required steps:"));
    Serial.println(F("1. Set a unique serial number for this device"));
    Serial.println(F("2. Set the hardware version"));
    Serial.println(F("3. Complete provisioning to enable normal operation"));
    Serial.println();
    Serial.println(F("Optional:"));
    Serial.println(F("- Enable debug mode for detailed logging"));
    Serial.println();
    Serial.println(F("The device MAC address is automatically set and cannot be changed."));
    Serial.println();
    return true;
  }
  else if (input == F("b"))
  {
    if (deviceInfo.provisioned)
    {
      Serial.println(F("Returning to main menu..."));
      setMenu(&mainMenu);
      return true;
    }
    else
    {
      Serial.println(F("Cannot return to main menu - device not provisioned"));
      return true;
    }
  }

  // Not recognized
  return false;
}

/****************************************************
 * Provisioning functions
 ****************************************************/
void startProvisioning()
{
  Serial.println(F("[INFO] [PROVISIONING] Starting provisioning mode"));
}

void setSerialNumber(uint32_t serialNumber)
{
  deviceInfo.serialNumber = serialNumber;
  saveDeviceInfo();
  Serial.println(String(F("[INFO] [PROVISIONING] Serial number set to: ")) + String(serialNumber));
}

void setHardwareVersion(uint16_t hardwareVersion)
{
  deviceInfo.hardwareVersion = hardwareVersion;
  saveDeviceInfo();
  Serial.println(String(F("[INFO] [PROVISIONING] Hardware version set to: ")) + String(hardwareVersion));
}

void enableDebugMode(bool enabled)
{
  deviceInfo.debugEnabled = enabled;
  saveDeviceInfo();
  Serial.println(String(F("[INFO] [PROVISIONING] Debug mode ")) + (enabled ? "enabled" : "disabled"));
}

void completeProvisioning()
{
  deviceInfo.provisioned = true;
  saveDeviceInfo();
  Serial.println(F("[INFO] [PROVISIONING] Device provisioning completed"));
}

void showProvisioningStatus()
{
  Serial.println();
  Serial.println(F("=== DEVICE STATUS ==="));
  Serial.println(String(F("Provisioned: ")) + (deviceInfo.provisioned ? "YES" : "NO"));
  Serial.println(String(F("Serial Number: ")) + (deviceInfo.serialNumber == 0 ? "NOT SET" : String(deviceInfo.serialNumber)));
  Serial.println(String(F("Hardware Version: ")) + (deviceInfo.hardwareVersion == 0 ? "NOT SET" : String(deviceInfo.hardwareVersion)));
  Serial.println(String(F("Debug Mode: ")) + (deviceInfo.debugEnabled ? "ENABLED" : "DISABLED"));
  Serial.printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                deviceInfo.macAddress[0], deviceInfo.macAddress[1], deviceInfo.macAddress[2],
                deviceInfo.macAddress[3], deviceInfo.macAddress[4], deviceInfo.macAddress[5]);
  Serial.println();

  if (!deviceInfo.provisioned)
  {
    Serial.println(F("STATUS: Device requires provisioning"));
    if (deviceInfo.serialNumber == 0)
    {
      Serial.println(F("  - Serial number must be set"));
    }
    if (deviceInfo.hardwareVersion == 0)
    {
      Serial.println(F("  - Hardware version must be set"));
    }
  }
  else
  {
    Serial.println(F("STATUS: Device is fully provisioned"));
  }
  Serial.println();
}

void factoryReset()
{
  Serial.println(F("[INFO] [PROVISIONING] Performing factory reset"));

  // Clear all preferences
  preferences.clear();

  // Reset device info to defaults
  deviceInfo = DeviceInfo();

  // Set MAC address from WiFi
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  memcpy(deviceInfo.macAddress, baseMac, 6);

  saveDeviceInfo();
  Serial.println(F("[INFO] [PROVISIONING] Factory reset completed"));
}