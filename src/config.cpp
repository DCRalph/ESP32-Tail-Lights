#include "config.h"

DeviceInfo::DeviceInfo()
{
  provisioned = false;
  debugEnabled = false;
  memset(macAddress, 0, sizeof(macAddress));
  serialNumber = 0;
  hardwareVersion = 0;
}

void DeviceInfo::print()
{
  Serial.println("DeviceInfo:");
  Serial.printf("Provisioned: %s\n", provisioned ? "true" : "false");
  Serial.printf("Debug Enabled: %s\n", debugEnabled ? "true" : "false");
  Serial.printf("Mac Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                macAddress[0], macAddress[1], macAddress[2],
                macAddress[3], macAddress[4], macAddress[5]);
  Serial.printf("Serial Number: %d\n", serialNumber);
  Serial.printf("Hardware Version: %d\n", hardwareVersion);
  Serial.println();
}

Preferences preferences;

DeviceInfo deviceInfo;

void restart()
{
  Serial.println("[INFO] [CONFIG] Restarting...");
  ESP.restart();
}

String formatBytes(size_t bytes, bool _short)
{
  if (bytes < 1024)
  {
    if (_short)
    {
      // Since bytes is an integer, leave it as is.
      return String(bytes);
    }
    else
    {
      return String(bytes) + " B";
    }
  }
  else if (bytes < (1024UL * 1024UL))
  {
    // Force floating point math by casting bytes to double.
    double kb = ((double)bytes) / 1024.0;
    if (_short)
    {
      return String(kb, 1) + "K";
    }
    else
    {
      return String(kb, 2) + " KB";
    }
  }
  else
  {
    double mb = ((double)bytes) / (1024.0 * 1024.0);
    if (_short)
    {
      return String(mb, 1) + "M";
    }
    else
    {
      return String(mb, 2) + " MB";
    }
  }
}

void saveDeviceInfo()
{
  preferences.putBytes("deviceInfo", &deviceInfo, sizeof(DeviceInfo));
}

void loadDeviceInfo()
{
  size_t schLen = preferences.getBytesLength("deviceInfo");
  if (schLen == sizeof(DeviceInfo))
  {
    preferences.getBytes("deviceInfo", &deviceInfo, sizeof(DeviceInfo));
    Serial.println("[INFO] [CONFIG] Device info loaded from preferences");
  }
  else
  {
    Serial.println("[INFO] [CONFIG] No valid device info found in preferences, using defaults");
  }

  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  memcpy(deviceInfo.macAddress, baseMac, 6);
  saveDeviceInfo();
}

bool isDeviceProvisioned()
{
  return deviceInfo.provisioned;
}
