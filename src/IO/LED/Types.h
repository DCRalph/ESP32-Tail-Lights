#pragma once

#include <Arduino.h>

struct __attribute__((packed)) RGBSyncData
{
  float hueCenter;
  float hueEdge;
  float speed;
  float hueOffset;
  bool active;

  String print();
};
struct __attribute__((packed)) NightRiderSyncData
{
  float cycleTime;
  float tailLength;
  float progress;
  bool forward;
  bool active;

  String print();
};

enum class PoliceMode
{
  SLOW,
  FAST
};

struct __attribute__((packed)) PoliceSyncData
{
  bool active;
  PoliceMode mode;
  float flashProgress;
  float cycleProgress;
  uint16_t currentFlash;

  String print();
};

// Predefined colors enum
enum class SolidColorPreset
{
  OFF = 0,
  RED,
  GREEN,
  BLUE,
  WHITE,
  YELLOW,
  CYAN,
  MAGENTA,
  ORANGE,
  PURPLE,
  LIME,
  PINK,
  TEAL,
  INDIGO,
  GOLD,
  SILVER,
  CUSTOM
};

struct __attribute__((packed)) SolidColorSyncData
{
  bool active;
  SolidColorPreset colorPreset;
  uint8_t customR;
  uint8_t customG;
  uint8_t customB;

  String print();
};

struct __attribute__((packed)) ColorFadeSyncData
{
  bool active;
  float holdTime;            // Time to hold each color (seconds)
  float fadeTime;            // Time to fade between colors (seconds)
  float progress;            // Current progress through the cycle [0.0, 1.0]
  uint8_t currentColorIndex; // Index of the current color in the list
  bool inFadePhase;          // true if currently fading, false if holding

  String print();
};

struct __attribute__((packed)) CommitSyncData
{
  uint32_t commitSpeed;         // Speed of commits in LED positions per second * 1000 (fixed point)
  uint16_t trailLength;         // Length of the fading trail in LED units * 1000 (fixed point)
  uint32_t commitInterval;      // Time between commits in milliseconds
  uint8_t headR, headG, headB;  // Color of the commit head
  uint32_t timeSinceLastCommit; // Time since last commit was spawned in milliseconds
  bool active;

  String print();
};

enum class ServiceLightsMode
{
  SLOW,
  FAST,
  ALTERNATE,
  STROBE,
  SCROLL,
};

struct __attribute__((packed)) ServiceLightsSyncData
{
  bool active;
  ServiceLightsMode mode;
  float progress;                   // flashProgress for timing sync
  float cycleProgress;              // cycleProgress for pattern sync
  uint16_t currentFlash;            // current flash count for FAST mode
  float fastSpeed;                  // timing configuration for fast mode
  float slowSpeed;                  // timing configuration for slow/alternate/scroll modes
  uint16_t fastModeFlashesPerCycle; // number of flashes per cycle in fast mode
  uint8_t colorR, colorG, colorB;   // color synchronization

  String print();
};

struct __attribute__((packed)) EffectSyncState
{
  RGBSyncData rgbSyncData;
  NightRiderSyncData nightRiderSyncData;
  PoliceSyncData policeSyncData;
  SolidColorSyncData solidColorSyncData;
  ColorFadeSyncData colorFadeSyncData;
  CommitSyncData commitSyncData;
  ServiceLightsSyncData serviceLightsSyncData;

  void print();
};