// application.h

#pragma once

#include "config.h"
#include "FastLED.h"
#include "IO/LED/LEDManager.h"
#include "IO/LED/Effects/BrakeLightEffect.h"
#include "IO/LED/Effects/IndicatorEffect.h"
#include "IO/LED/Effects/ReverseLightEffect.h"
#include "IO/LED/Effects/RGBEffect.h"
#include "IO/LED/Effects/NightRiderEffect.h"
#include "IO/LED/Effects/StartupEffect.h"

#include "Sequences/SequenceBase.h"
#include "Sequences/BothIndicatorsSequence.h"
#include "Sequences/IndicatorFlickSequence.h"
#include "Sequences/BrakeTapSequence.h"

#include "IO/GPIO.h"
#include "IO/Wireless.h"

enum class ApplicationMode
{
  NORMAL,
  TEST,
  REMOTE,
  OFF
};

class Application
{
public:
  static Application *getInstance();

  Application();
  ~Application();

  // Call once to initialize the system.
  void begin();

  // Main update function to be called from loop()
  void update();

  // Set testMode externally.
  void enableNormalMode();
  void enableTestMode();
  void enableRemoteMode();
  void enableOffMode();

  // Global pointer to the custom LED manager.
  CRGB leds[NUM_LEDS];

  LEDManager *ledManager;

private:
  // Input pointers.

#ifdef ENABLE_HV_INPUTS
  GpIO *accOn;          // 12v ACC
  GpIO *brake;          // Brake
  GpIO *leftIndicator;  // Left indicator
  GpIO *rightIndicator; // Right indicator
  GpIO *reverse;        // Reverse
#endif

  bool accOnState;
  bool lastAccOnState;
  bool brakeState;
  bool leftIndicatorState;
  bool rightIndicatorState;
  bool reverseState;

  void updateInputs();

  uint64_t lastAccOn;

  // Effect instances.
  BrakeLightEffect *brakeEffect;
  IndicatorEffect *leftIndicatorEffect;
  IndicatorEffect *rightIndicatorEffect;
  ReverseLightEffect *reverseLightEffect;
  RGBEffect *rgbEffect;
  NightRiderEffect *nightriderEffect;
  StartupEffect *startupEffect;

  // Sequences
  BothIndicatorsSequence *unlockSequence;
  BothIndicatorsSequence *lockSequence;
  IndicatorFlickSequence *RGBFlickSequence;
  IndicatorFlickSequence *nightRiderFlickSequence;
  BrakeTapSequence *brakeTapSequence3;

  // Application Mode
  ApplicationMode mode;

  // Internal method to handle effect selection based on inputs.
  void handleNormalEffects();
  void handleRemoteEffects();

  uint64_t lastRemotePing;

  static void drawLEDs();
};
