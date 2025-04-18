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

extern Preferences preferences;

#define S3_DEV
// #define S2_CAR

// #define ENABLE_HV_INPUTS

// WiFi

#define ESP_NOW_CHANNEL 1
#define CHANNEL 1
// #define DEBUG_ESP_NOW
// #define ESPNOW_NO_DISABLE_WIFI

#define DEBUG_SYNC

#ifdef S3_DEV
#define LED_PIN 21
#endif

#ifdef S2_CAR
#define LED_PIN 15

#define INPUT_1_PIN 1
#define INPUT_2_PIN 2
#define INPUT_3_PIN 3
#define INPUT_4_PIN 4
#define INPUT_5_PIN 5
#define INPUT_6_PIN 6
#endif

#define ENABLE_HEADLIGHTS
#define ENABLE_TAILLIGHTS
// #define ENABLE_UNDERGLOW
// #define ENABLE_INTERIOR

#ifdef ENABLE_HEADLIGHTS
#define HEADLIGHT_LED_COUNT 120
#define HEADLIGHT_LED_PIN 8
// #define HEADLIGHT_FLIPED
#endif

#ifdef ENABLE_TAILLIGHTS
#define TAILLIGHT_LED_COUNT 120
#define TAILLIGHT_LED_PIN 6
#endif

#ifdef ENABLE_UNDERGLOW
#define UNDERGLOW_LED_COUNT 120
// #define UNDERGLOW_LED_PIN
#endif

#ifdef ENABLE_INTERIOR
#define INTERIOR_LED_COUNT 60
// #define INTERIOR_LED_PIN
#endif