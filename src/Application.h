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
#ifdef HEAD_LIGHTS
#include "IO/LED/Effects/HeadlightEffect.h"
#endif

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

  // Main loop function to be called from loop()
  void loop();

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
  GpIO *accOn;           // 12v ACC
  GpIO *leftIndicator;   // Left indicator
  GpIO *rightIndicator;  // Right indicator
  GpIO *externalControl; // button to do things

#ifdef HEAD_LIGHTS
  GpIO *highBeam; // High beam
  GpIO *lowBeam;  // Low beam
#endif

#ifdef TAIL_LIGHTS
  GpIO *brake;   // Brake
  GpIO *reverse; // Reverse
#endif

#endif

  bool accOnState;
  bool lastAccOnState;
  bool leftIndicatorState;
  bool rightIndicatorState;
  bool externalControlState;

#ifdef HEAD_LIGHTS
  bool highBeamState;
  bool lowBeamState;
#endif

#ifdef TAIL_LIGHTS
  bool brakeState;
  bool reverseState;
#endif

  void updateInputs();

  uint64_t lastAccOn;

  // Effect instances.
  IndicatorEffect *leftIndicatorEffect;
  IndicatorEffect *rightIndicatorEffect;
  RGBEffect *rgbEffect;
  NightRiderEffect *nightriderEffect;
  StartupEffect *startupEffect;

#ifdef HEAD_LIGHTS
  HeadlightEffect *headlightEffect;
#endif

#ifdef TAIL_LIGHTS
  BrakeLightEffect *brakeEffect;
  ReverseLightEffect *reverseLightEffect;
#endif

  // Sequences
  BothIndicatorsSequence *unlockSequence;
  BothIndicatorsSequence *lockSequence;
  IndicatorFlickSequence *RGBFlickSequence;
  IndicatorFlickSequence *nightRiderFlickSequence;

#ifdef TAIL_LIGHTS
  BrakeTapSequence *brakeTapSequence3;
#endif

  // Application Mode
  ApplicationMode mode;

  // Internal method to handle effect selection based on inputs.
  void handleNormalEffects();
  void handleTestEffects();
  void handleRemoteEffects();

  uint64_t lastRemotePing;

  static void drawLEDs();
};
