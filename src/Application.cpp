// application.cpp

#include "Application.h"
#include "config.h"
#include "IO/Wireless.h" // Your wireless library
#include "FastLED.h"
#include "IO/LED/LEDStripManager.h"
#include "Sync/SyncManager.h"

//----------------------------------------------------------------------------

Application *Application::getInstance()
{
  static Application instance;
  return &instance;
}

//----------------------------------------------------------------------------
/*
 * Application constructor. Initialize pointers and set defaults.
 */
Application::Application()
{
// Initialize HVInput instances.
#ifdef ENABLE_HV_INPUTS
  accOnInput = new HVInput(&input1, HV_HIGH_ACTIVE);
  leftIndicatorInput = new HVInput(&input2, HV_HIGH_ACTIVE);
  rightIndicatorInput = new HVInput(&input3, HV_HIGH_ACTIVE);
  externalControlInput = new HVInput(&input4, HV_HIGH_ACTIVE);

// #ifdef ENABLE_HEADLIGHTS
//   highBeamInput = new HVInput(&input5, HV_HIGH_ACTIVE);
//   lowBeamInput = new HVInput(&input6, HV_HIGH_ACTIVE);
// #endif

#ifdef ENABLE_TAILLIGHTS
  brakeInput = new HVInput(&input5, HV_HIGH_ACTIVE);
  reverseInput = new HVInput(&input6, HV_HIGH_ACTIVE);
#endif
#else
  accOnInput = nullptr;
  leftIndicatorInput = nullptr;
  rightIndicatorInput = nullptr;
  externalControlInput = nullptr;

#ifdef ENABLE_HEADLIGHTS
  highBeamInput = nullptr;
  lowBeamInput = nullptr;
#endif

#ifdef ENABLE_TAILLIGHTS
  brakeInput = nullptr;
  reverseInput = nullptr;
#endif
#endif

  // Initialize effect pointers to nullptr
  leftIndicatorEffect = nullptr;
  rightIndicatorEffect = nullptr;
  rgbEffect = nullptr;
  nightriderEffect = nullptr;
  taillightStartupEffect = nullptr;
  headlightStartupEffect = nullptr;
  policeEffect = nullptr;
  pulseWaveEffect = nullptr;
  auroraEffect = nullptr;

  // Initialize sequence pointers to nullptr
  unlockSequence = nullptr;
  lockSequence = nullptr;
  RGBFlickSequence = nullptr;
  nightRiderFlickSequence = nullptr;

#ifdef ENABLE_TAILLIGHTS
  brakeTapSequence3 = nullptr;
#endif
}

/*
 * Application destructor. Clean up dynamic allocations.
 * This is not necessary for this project, but is good practice.
 */
Application::~Application()
{
  leftIndicatorEffect = nullptr;
  rightIndicatorEffect = nullptr;
  rgbEffect = nullptr;
  nightriderEffect = nullptr;
  taillightStartupEffect = nullptr;
  headlightStartupEffect = nullptr;
  policeEffect = nullptr;
  pulseWaveEffect = nullptr;
  auroraEffect = nullptr;

  delete brakeEffect;
  delete reverseLightEffect;
  delete policeEffect;
  delete headlightEffect;
  delete leftIndicatorEffect;
  delete rightIndicatorEffect;
  delete rgbEffect;
  delete nightriderEffect;
  delete taillightStartupEffect;
  delete headlightStartupEffect;
  delete pulseWaveEffect;
  delete auroraEffect;

  delete unlockSequence;
  delete lockSequence;
  delete RGBFlickSequence;
  delete nightRiderFlickSequence;

#ifdef ENABLE_TAILLIGHTS
  delete brakeTapSequence3;
#endif

#ifdef ENABLE_HEADLIGHTS
  // No headlight-specific sequences to delete yet
#endif

  // Clean up HVInput instances
#ifdef ENABLE_HV_INPUTS
  delete accOnInput;
  delete leftIndicatorInput;
  delete rightIndicatorInput;
  delete externalControlInput;

// #ifdef ENABLE_HEADLIGHTS
//   delete highBeamInput;
//   delete lowBeamInput;
// #endif

#ifdef ENABLE_TAILLIGHTS
  delete brakeInput;
  delete reverseInput;
#endif
#endif
}

/*
 * Begin: Initializes the LED strip, effects, wireless, etc.
 */
void Application::begin()
{

  mode = static_cast<ApplicationMode>(preferences.getUInt("mode", 0));

  LEDStripManager *ledManager = LEDStripManager::getInstance();
  ledManager->begin();

#ifdef ENABLE_HEADLIGHTS
  LEDStripConfig headlights(
      LEDStripType::HEADLIGHT,
      new LEDStrip(HEADLIGHT_LED_COUNT),
      "Headlights");
  FastLED.addLeds<WS2812, HEADLIGHT_LED_PIN, GRB>(headlights.strip->getFastLEDBuffer(), HEADLIGHT_LED_COUNT);
#ifdef HEADLIGHT_FLIPED
  headlights.strip->setFliped(true);
#endif
  ledManager->addLEDStrip(headlights);

  // Flash a test LED to indicate startup.
  headlights.strip->getFastLEDBuffer()[0] = CRGB(255, 0, 0);
  FastLED.show();
  delay(500);
  headlights.strip->getFastLEDBuffer()[0] = CRGB(0, 0, 0);
  FastLED.show();

#endif

#ifdef ENABLE_TAILLIGHTS
  LEDStripConfig taillights(
      LEDStripType::TAILLIGHT,
      new LEDStrip(TAILLIGHT_LED_COUNT),
      "Taillights");
  FastLED.addLeds<WS2812, TAILLIGHT_LED_PIN, GRB>(taillights.strip->getFastLEDBuffer(), TAILLIGHT_LED_COUNT);
  ledManager->addLEDStrip(taillights);

  taillights.strip->getFastLEDBuffer()[0] = CRGB(255, 0, 0);
  FastLED.show();
  delay(500);
  taillights.strip->getFastLEDBuffer()[0] = CRGB(0, 0, 0);
  FastLED.show();
#endif

#ifdef ENABLE_UNDERGLOW
  LEDStripConfig underglow(
      LEDStripType::UNDERGLOW,
      new LEDStrip(UNDERGLOW_LED_COUNT),
      "Underglow");
  FastLED.addLeds<WS2812, UNDERGLOW_LED_PIN, GRB>(underglow.strip->getFastLEDBuffer(), UNDERGLOW_LED_COUNT);
  ledManager->addLEDStrip(underglow);
#endif

#ifdef ENABLE_INTERIOR
  LEDStripConfig interior(
      LEDStripType::INTERIOR,
      new LEDStrip(INTERIOR_LED_COUNT),
      "Interior");
  FastLED.addLeds<WS2812, INTERIOR_LED_PIN, GRB>(interior.strip->getFastLEDBuffer(), INTERIOR_LED_COUNT);
  ledManager->addLEDStrip(interior);
#endif

  setupEffects();
  setupSequences();
  setupWireless();

  // Initialize SyncManager
  SyncManager *syncMgr = SyncManager::getInstance();
  syncMgr->begin();

#ifdef ENABLE_SYNC
  // Set the callback for effect state changes from sync
  syncMgr->setEffectChangeCallback([this](const EffectSyncState &state)
                                   {
    // Only apply sync changes if in NORMAL mode and ACC is on
    if (mode == ApplicationMode::NORMAL) {
      leftIndicatorEffect->setActive(state.leftIndicator);
      rightIndicatorEffect->setActive(state.rightIndicator);
      rgbEffect->setActive(state.rgb);
      nightriderEffect->setActive(state.nightrider);
      taillightStartupEffect->setActive(state.startup);
      headlightStartupEffect->setActive(state.startup);
      policeEffect->setActive(state.police);
      pulseWaveEffect->setActive(state.pulseWave);
      auroraEffect->setActive(state.aurora);
    } });
#endif

  // #ifndef ENABLE_HV_INPUTS
  //   enableTestMode();
  //   // rgbEffect->setActive(true);
  //   headlightStartupEffect->setActive(true);
  // #endif

  // set brightness to 100
  ledManager->setBrightness(255);

  // clear all buffers and fastled.show
  FastLED.clear();
  FastLED.show();
}

void Application::updateInputs()
{
#ifdef ENABLE_HV_INPUTS
  // Simply call update() on each HVInput instance
  updateInput(accOnInput);
  updateInput(leftIndicatorInput);
  updateInput(rightIndicatorInput);
  updateInput(externalControlInput);

// #ifdef ENABLE_HEADLIGHTS
//   updateInput(highBeamInput);
//   updateInput(lowBeamInput);
// #endif

#ifdef ENABLE_TAILLIGHTS
  updateInput(brakeInput);
  updateInput(reverseInput);
#endif
#endif
}

/*
 * update():
 * Main loop update.
 */
void Application::loop()
{
  // Update input states.
  updateInputs();

  // handle remote dissconnection
  if (lastRemotePing != 0 && millis() - lastRemotePing > 2000)
  {
    if (mode == ApplicationMode::TEST || mode == ApplicationMode::REMOTE)
    {
      lastRemotePing = 0;
      mode = ApplicationMode::NORMAL;
      // disable all effects
      leftIndicatorEffect->setActive(false);
      rightIndicatorEffect->setActive(false);
      rgbEffect->setActive(false);
      nightriderEffect->setActive(false);
      taillightStartupEffect->setActive(false);
      headlightStartupEffect->setOff();

      headlightEffect->setActive(false);
      headlightEffect->setSplit(false);
      headlightEffect->setColor(false, false, false);

      brakeEffect->setActive(false);
      brakeEffect->setIsReversing(false);
      reverseLightEffect->setActive(false);
    }
  }

  // handle brake tap sequence. always check if this is triggered
#ifdef ENABLE_TAILLIGHTS
  brakeTapSequence3->setInput(getInput(brakeInput));
  brakeTapSequence3->loop();
#endif

  switch (mode)
  {
  case ApplicationMode::NORMAL:
    handleNormalEffects();
    break;

  case ApplicationMode::TEST:
  {
    handleTestEffects();
  }
  break;

  case ApplicationMode::REMOTE:
    handleRemoteEffects();
    break;

  case ApplicationMode::OFF:
  {
    // turn off all effects
    leftIndicatorEffect->setActive(false);
    rightIndicatorEffect->setActive(false);
    rgbEffect->setActive(false);
    nightriderEffect->setActive(false);
    taillightStartupEffect->setActive(false);
    headlightStartupEffect->setOff();

    headlightEffect->setActive(false);
    headlightEffect->setSplit(false);
    headlightEffect->setColor(false, false, false);

    brakeEffect->setActive(false);
    brakeEffect->setIsReversing(false);
    reverseLightEffect->setActive(false);
  }
  break;
  }

  // Update SyncManager
  SyncManager *syncMgr = SyncManager::getInstance();
  syncMgr->loop();

#ifdef ENABLE_SYNC
  // If in normal mode and we have ACC on, sync our effect states
  if (mode == ApplicationMode::NORMAL || mode == ApplicationMode::REMOTE)
  {
    EffectSyncState state;
    state.leftIndicator = leftIndicatorEffect->isActive();
    state.rightIndicator = rightIndicatorEffect->isActive();
    state.rgb = rgbEffect->isActive();
    state.nightrider = nightriderEffect->isActive();
    state.startup = taillightStartupEffect->isActive() || headlightStartupEffect->isActive();
    state.police = policeEffect->isActive();
    state.pulseWave = pulseWaveEffect->isActive();
    state.aurora = auroraEffect->isActive();

    syncMgr->updateEffectStates(state);
  }
#endif

  // Update and draw LED effects.
  LEDStripManager::getInstance()->updateEffects();
  LEDStripManager::getInstance()->draw();
}

void Application::enableNormalMode()
{
  mode = ApplicationMode::NORMAL;
  preferences.putUInt("mode", static_cast<uint8_t>(mode));
}

void Application::enableTestMode()
{
  mode = ApplicationMode::TEST;
  preferences.putUInt("mode", static_cast<uint8_t>(mode));
}

void Application::enableRemoteMode()
{
  mode = ApplicationMode::REMOTE;
  preferences.putUInt("mode", static_cast<uint8_t>(mode));
}

void Application::enableOffMode()
{
  mode = ApplicationMode::OFF;
  preferences.putUInt("mode", static_cast<uint8_t>(mode));

  // turn off all effects
  leftIndicatorEffect->setActive(false);
  rightIndicatorEffect->setActive(false);
  rgbEffect->setActive(false);
  nightriderEffect->setActive(false);
  taillightStartupEffect->setActive(false);
  headlightStartupEffect->setOff();

  headlightEffect->setActive(false);
  headlightEffect->setSplit(false);
  headlightEffect->setColor(false, false, false);

  brakeEffect->setActive(false);
  brakeEffect->setIsReversing(false);
  reverseLightEffect->setActive(false);

  pulseWaveEffect->setActive(false);
  auroraEffect->setActive(false);
}

void Application::handleTestEffects()
{

  // Regular test mode controls (commented out)
  // reverseLightEffect->setActive(true);
  // headlightEffect->setActive(true);
  // headlightEffect->setSplit(false);
  // brakeEffect->setIsReversing(true);
  // brakeEffect->setActive(io0.read());
  // leftIndicatorEffect->setActive(io0.read());
  // rightIndicatorEffect->setActive(io0.read());
  // reverseLightEffect->setActive(io0.read());
  // rgbEffect->setActive(io0.read());
  // startupEffect->setActive(io0.read());
}

void Application::handleRemoteEffects()
{
}
