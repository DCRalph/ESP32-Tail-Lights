#pragma once

#include <Arduino.h>

#include "IO/GPIO.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

#include <Preferences.h>

#define setBit(x, y, z) (x |= (z << y)) // Set a bit to a value
#define clearBit(x, y) (x &= ~(1 << y)) // Clear a bit
#define toggleBit(x, y) (x ^= (1 << y)) // Toggle a bit (0 -> 1, 1 -> 0) and return the new value
#define checkBit(x, y) ((x >> y) & 1)   // Check a bit and return 1 or 0

struct DeviceInfo
{
  bool debugEnabled = false;
  bool oledEnabled = false;

  uint8_t macAddress[6];
  uint32_t serialNumber;
  uint16_t hardwareVersion;

  DeviceInfo();
  void print();
};

struct LEDConfig
{
  // LED strip enable flags
  bool headlightsEnabled;
  bool taillightsEnabled;
  bool underglowEnabled;
  bool interiorEnabled;

  // LED strip lengths (default to 50 as requested)
  uint16_t headlightLedCount;
  uint16_t taillightLedCount;
  uint16_t underglowLedCount;
  uint16_t interiorLedCount;

  // LED strip pins (use defaults from defines)
  // uint8_t headlightPin;
  // uint8_t taillightPin;
  // uint8_t underglowPin;
  // uint8_t interiorPin;

  // LED strip flip settings
  bool headlightFlipped;
  bool taillightFlipped;
  bool underglowFlipped;
  bool interiorFlipped;

  LEDConfig();
  void print();
};

extern Preferences preferences;

extern DeviceInfo deviceInfo;
extern LEDConfig ledConfig;

// fastled mutex
extern SemaphoreHandle_t fastledMutex;

void restart();
String formatBytes(size_t bytes, bool _short = false);

void saveDeviceInfo();
void loadDeviceInfo();

void saveLEDConfig();
void loadLEDConfig();

#define S3_V1
// #define S3_DEV
// #define S2_CAR

// WiFi

#define ESP_NOW_CHANNEL 1
#define CHANNEL 1
// #define DEBUG_ESP_NOW
// #define ESPNOW_NO_DISABLE_WIFI

// #define DEBUG_SYNC
#define ENABLE_SYNC
// #define ENABLE_DISPLAY

#ifdef S3_V1
#define LED_PIN 40 // make optinal

#define ENABLE_HV_INPUTS

#define INPUT_BTN_BOOT_PIN 0
#define INPUT_BTN_PREV_PIN 21
#define INPUT_BTN_SEL_PIN 14
#define INPUT_BTN_NEXT_PIN 13

#define INPUT_VOLTAGE_PIN 2

#define INPUT_1_PIN 4
#define INPUT_2_PIN 5
#define INPUT_3_PIN 6
#define INPUT_4_PIN 7
#define INPUT_5_PIN 15
#define INPUT_6_PIN 16
#define INPUT_7_PIN 17
#define INPUT_8_PIN 18

#define OUTPUT_STATUS_LED_PIN 1

#define OUTPUT_LED_1_PIN 3
#define OUTPUT_LED_2_PIN 10
#define OUTPUT_LED_3_PIN 11
#define OUTPUT_LED_4_PIN 12
#endif

#ifdef S3_DEV
#define LED_PIN 21
#define OUTPUT_STATUS_LED_PIN 16

// #define ENABLE_HV_INPUTS

#endif

#ifdef S2_CAR
#define LED_PIN 15

#define ENABLE_HV_INPUTS

#define INPUT_1_PIN 1
#define INPUT_2_PIN 2
#define INPUT_3_PIN 3
#define INPUT_4_PIN 4
#define INPUT_5_PIN 5
#define INPUT_6_PIN 6
#endif

// #define ENABLE_HEADLIGHTS
// #define ENABLE_TAILLIGHTS
// // #define ENABLE_UNDERGLOW
// // #define ENABLE_INTERIOR

// #ifdef ENABLE_HEADLIGHTS
// #define HEADLIGHT_LED_COUNT 99
// #define HEADLIGHT_LED_PIN OUTPUT_LED_1_PIN
// // #define HEADLIGHT_FLIPED
// #endif

// #ifdef ENABLE_TAILLIGHTS
// #define TAILLIGHT_LED_COUNT 120
// #define TAILLIGHT_LED_PIN OUTPUT_LED_2_PIN
// #endif

// #ifdef ENABLE_UNDERGLOW
// #define UNDERGLOW_LED_COUNT 120
// #define UNDERGLOW_LED_PIN OUTPUT_LED_3_PIN
// #endif

// #ifdef ENABLE_INTERIOR
// #define INTERIOR_LED_COUNT 60
// #define INTERIOR_LED_PIN OUTPUT_LED_4_PIN
// #endif