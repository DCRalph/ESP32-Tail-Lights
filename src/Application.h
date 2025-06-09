// application.h

#pragma once

#include "config.h"
#include "IO/LED/LEDStrip.h"
#include "IO/LED/Effects/BrakeLightEffect.h"
#include "IO/LED/Effects/IndicatorEffect.h"
#include "IO/LED/Effects/ReverseLightEffect.h"
#include "IO/LED/Effects/RGBEffect.h"
#include "IO/LED/Effects/NightRiderEffect.h"
#include "IO/LED/Effects/TaillightStartupEffect.h"
#include "IO/LED/Effects/HeadlightEffect.h"
#include "IO/LED/Effects/HeadlightStartupEffect.h"
#include "IO/LED/Effects/PoliceEffect.h"
#include "IO/LED/Effects/PulseWaveEffect.h"
#include "IO/LED/Effects/AuroraEffect.h"

#include "Sequences/SequenceBase.h"
#include "Sequences/BothIndicatorsSequence.h"
#include "Sequences/IndicatorFlickSequence.h"
#include "Sequences/BrakeTapSequence.h"

#include "IO/GPIO.h"
#include "IO/Inputs.h"
#include "IO/Wireless.h"

static void updateInput(HVInput input)
{
  input.update();
}

static bool getInput(HVInput input)
{
  return input.get();
}

static bool isEnabled(HVInput input)
{
  return input.isEnabled();
}

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

  void setupEffects();
  void setupSequences();

  // Main loop function to be called from loop()
  void loop();

  // Set testMode externally.
  void enableNormalMode();
  void enableTestMode();
  void enableRemoteMode();
  void enableOffMode();

  // Global pointer to the custom LED manager.

private:
  // HVInput instances
  HVInput accOnInput;          // 12v ACC
  HVInput leftIndicatorInput;  // Left indicator
  HVInput rightIndicatorInput; // Right indicator
  HVInput headlightInput;      // Headlight
  HVInput brakeInput;          // Brake
  HVInput reverseInput;        // Reverse

  void updateInputs();
  void setupWireless();

  // Effect instances.
  IndicatorEffect *leftIndicatorEffect;
  IndicatorEffect *rightIndicatorEffect;
  RGBEffect *rgbEffect;
  NightRiderEffect *nightriderEffect;
  TaillightStartupEffect *taillightStartupEffect;
  HeadlightStartupEffect *headlightStartupEffect;

  HeadlightEffect *headlightEffect;

  BrakeLightEffect *brakeEffect;
  ReverseLightEffect *reverseLightEffect;
  PoliceEffect *policeEffect;

  // New effects
  PulseWaveEffect *pulseWaveEffect;
  AuroraEffect *auroraEffect;

  // Sequences
  BothIndicatorsSequence *unlockSequence;
  BothIndicatorsSequence *lockSequence;
  IndicatorFlickSequence *RGBFlickSequence;
  IndicatorFlickSequence *nightRiderFlickSequence;

#ifdef ENABLE_TAILLIGHTS
  BrakeTapSequence *brakeTapSequence3;
#endif

  // Application Mode
  ApplicationMode mode;

  // Internal method to handle effect selection based on inputs.
  void handleNormalEffects();
  void handleTestEffects();
  void handleRemoteEffects();

  uint64_t lastRemotePing;
};
