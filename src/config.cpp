#include "config.h"

DeviceInfo::DeviceInfo()
{
  debugEnabled = false;
  oledEnabled = false;
  memset(macAddress, 0, sizeof(macAddress));
  serialNumber = 0;
  hardwareVersion = 0;
}

void DeviceInfo::print()
{
  Serial.println("DeviceInfo:");
  Serial.printf("Debug Enabled: %s\n", debugEnabled ? "true" : "false");
  Serial.printf("OLED Enabled: %s\n", oledEnabled ? "true" : "false");
  Serial.printf("Mac Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                macAddress[0], macAddress[1], macAddress[2],
                macAddress[3], macAddress[4], macAddress[5]);
  Serial.printf("Serial Number: %d\n", serialNumber);
  Serial.printf("Hardware Version: %d\n", hardwareVersion);
  Serial.println();
}

Preferences preferences;

DeviceInfo deviceInfo;

SemaphoreHandle_t fastledMutex;

LEDConfig::LEDConfig()
{
  // Constructor initializes with default values as set in header
  headlightsEnabled = true;
  taillightsEnabled = true;
  underglowEnabled = false;
  interiorEnabled = false;

  headlightLedCount = 50;
  taillightLedCount = 50;
  underglowLedCount = 50;
  interiorLedCount = 50;

  // headlightPin = OUTPUT_LED_1_PIN;
  // taillightPin = OUTPUT_LED_2_PIN;
  // underglowPin = OUTPUT_LED_3_PIN;
  // interiorPin = OUTPUT_LED_4_PIN;

  headlightFlipped = false;
  taillightFlipped = false;
  underglowFlipped = false;
  interiorFlipped = false;
}

void LEDConfig::print()
{
  Serial.println("LED Configuration:");
  Serial.printf("Headlights: %s, Count: %d, Pin: %d, Flipped: %s\n",
                headlightsEnabled ? "ENABLED" : "DISABLED",
                headlightLedCount, 0, // headlightPin,
                headlightFlipped ? "YES" : "NO");
  Serial.printf("Taillights: %s, Count: %d, Pin: %d, Flipped: %s\n",
                taillightsEnabled ? "ENABLED" : "DISABLED",
                taillightLedCount, 0, // taillightPin,
                taillightFlipped ? "YES" : "NO");
  Serial.printf("Underglow: %s, Count: %d, Pin: %d, Flipped: %s\n",
                underglowEnabled ? "ENABLED" : "DISABLED",
                underglowLedCount, 0, // underglowPin,
                underglowFlipped ? "YES" : "NO");
  Serial.printf("Interior: %s, Count: %d, Pin: %d, Flipped: %s\n",
                interiorEnabled ? "ENABLED" : "DISABLED",
                interiorLedCount, 0, // interiorPin,
                interiorFlipped ? "YES" : "NO");
  Serial.println();
}

LEDConfig ledConfig;

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

void saveLEDConfig()
{
  preferences.putBytes("ledConfig", &ledConfig, sizeof(LEDConfig));
  Serial.println("[INFO] [CONFIG] LED config saved to preferences");
}

void loadLEDConfig()
{
  size_t schLen = preferences.getBytesLength("ledConfig");
  if (schLen == sizeof(LEDConfig))
  {
    preferences.getBytes("ledConfig", &ledConfig, sizeof(LEDConfig));
    Serial.println("[INFO] [CONFIG] LED config loaded from preferences");
  }
  else
  {
    Serial.println("[INFO] [CONFIG] No valid LED config found in preferences, using defaults");
    saveLEDConfig(); // Save defaults
  }
}
