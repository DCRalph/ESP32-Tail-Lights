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
  float commitSpeed;           // Speed of commits in LED positions per second
  float trailLength;           // Length of the fading trail in LED units
  float commitInterval;        // Time between commits in seconds
  uint8_t headR, headG, headB; // Color of the commit head
  float timeSinceLastCommit;   // Time since last commit was spawned
  bool active;

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

  void print();
};