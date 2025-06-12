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
  float currentPos;
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

struct __attribute__((packed)) EffectSyncState
{
  RGBSyncData rgbSyncData;
  NightRiderSyncData nightRiderSyncData;
  PoliceSyncData policeSyncData;
  SolidColorSyncData solidColorSyncData;

  void print();
};