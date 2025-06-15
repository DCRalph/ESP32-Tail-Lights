// application.cpp

#include "Application.h"
#include "config.h"
#include "IO/Wireless.h"
#include "IO/LED/LEDStripManager.h"
#include "Sync/SyncManager.h"
#include "IO/StatusLed.h"
#include "IO/TimeProfiler.h"

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
  accOnInput = HVInput(&input1, HV_HIGH_ACTIVE);
  leftIndicatorInput = HVInput(&input2, HV_HIGH_ACTIVE);
  rightIndicatorInput = HVInput(&input3, HV_HIGH_ACTIVE);

  headlightInput = HVInput(&input4, HV_HIGH_ACTIVE);

  brakeInput = HVInput(&input5, HV_HIGH_ACTIVE);
  reverseInput = HVInput(&input6, HV_HIGH_ACTIVE);

#endif

  // Initialize effect pointers to nullptr
  leftIndicatorEffect = nullptr;
  rightIndicatorEffect = nullptr;
  rgbEffect = nullptr;
  nightriderEffect = nullptr;
  taillightEffect = nullptr;
  policeEffect = nullptr;
  pulseWaveEffect = nullptr;
  auroraEffect = nullptr;

  // Initialize sequence pointers to nullptr
  unlockSequence = nullptr;
  lockSequence = nullptr;
  RGBFlickSequence = nullptr;
  nightRiderFlickSequence = nullptr;

  brakeTapSequence3 = nullptr;
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
  taillightEffect = nullptr;
  policeEffect = nullptr;
  pulseWaveEffect = nullptr;
  auroraEffect = nullptr;

  delete taillightEffect;
  delete policeEffect;
  delete headlightEffect;
  delete leftIndicatorEffect;
  delete rightIndicatorEffect;
  delete rgbEffect;
  delete nightriderEffect;
  delete pulseWaveEffect;
  delete auroraEffect;

  delete unlockSequence;
  delete lockSequence;
  delete RGBFlickSequence;
  delete nightRiderFlickSequence;

  delete brakeTapSequence3;
}

/*
 * Begin: Initializes the LED strip, effects, wireless, etc.
 */
void Application::begin()
{

  mode = static_cast<ApplicationMode>(preferences.getUInt("mode", 0));
  prevMode = mode == ApplicationMode::NORMAL ? ApplicationMode::OFF : ApplicationMode::NORMAL;

  LEDStripManager *ledManager = LEDStripManager::getInstance();
  ledManager->begin();

  ledManager->startTask();

  if (ledConfig.headlightsEnabled)
  {
    LEDStripConfig headlights(
        LEDStripType::HEADLIGHT,
        new LEDStrip(ledConfig.headlightLedCount, OUTPUT_LED_1_PIN),
        "Headlights");
    headlights.strip->setFliped(ledConfig.headlightFlipped);

    ledManager->addLEDStrip(headlights);

    headlights.strip->getBuffer()[0] = Color(255, 0, 0);
    delay(500);
    headlights.strip->getBuffer()[0] = Color(0, 0, 0);
  }

  if (ledConfig.taillightsEnabled)
  {
    LEDStripConfig taillights(
        LEDStripType::TAILLIGHT,
        new LEDStrip(ledConfig.taillightLedCount, OUTPUT_LED_2_PIN),
        "Taillights");
    taillights.strip->setFliped(ledConfig.taillightFlipped);

    ledManager->addLEDStrip(taillights);

    taillights.strip->getBuffer()[0] = Color(255, 0, 0);
    delay(500);
    taillights.strip->getBuffer()[0] = Color(0, 0, 0);
  }

  if (ledConfig.underglowEnabled)
  {
    LEDStripConfig underglow(
        LEDStripType::UNDERGLOW,
        new LEDStrip(ledConfig.underglowLedCount, OUTPUT_LED_3_PIN),
        "Underglow");
    underglow.strip->setFliped(ledConfig.underglowFlipped);

    ledManager->addLEDStrip(underglow);

    underglow.strip->getBuffer()[0] = Color(255, 0, 0);
    delay(500);
    underglow.strip->getBuffer()[0] = Color(0, 0, 0);
  }

  if (ledConfig.interiorEnabled)
  {
    LEDStripConfig interior(
        LEDStripType::INTERIOR,
        new LEDStrip(ledConfig.interiorLedCount, OUTPUT_LED_4_PIN),
        "Interior");
    interior.strip->setFliped(ledConfig.interiorFlipped);

    ledManager->addLEDStrip(interior);

    interior.strip->getBuffer()[0] = Color(255, 0, 0);
    delay(500);
    interior.strip->getBuffer()[0] = Color(0, 0, 0);
  }

  setupEffects();
  setupSequences();
  setupWireless();

  // Initialize SyncManager
  SyncManager *syncMgr = SyncManager::getInstance();
  syncMgr->begin();

#ifdef ENABLE_SYNC
  // Set up callbacks for sync events
  syncMgr->setDeviceDiscoveredCallback([this](const DiscoveredDevice &device)
                                       { Serial.println("Application: Device discovered - ID: 0x" + String(device.deviceId, HEX)); });

  syncMgr->setGroupFoundCallback([this](const GroupAdvert &advert)
                                 { Serial.println("Application: Group found - ID: 0x" + String(advert.groupId, HEX)); });

  syncMgr->setGroupCreatedCallback([this](const GroupInfo &group)
                                   { Serial.println("Application: Group created - ID: 0x" + String(group.groupId, HEX)); });

  syncMgr->setGroupJoinedCallback([this](const GroupInfo &group)
                                  { Serial.println("Application: Joined group - ID: 0x" + String(group.groupId, HEX)); });

  syncMgr->setGroupLeftCallback([this]()
                                { Serial.println("Application: Left group"); });

  syncMgr->setTimeSyncCallback([this](uint32_t syncedTime)
                               {
                                 Serial.println("Application: Time synchronized - synced time: " + String(syncedTime));
                                 // TODO: Use synchronized time for effect timing coordination
                               });

  syncMgr->setEffectSyncCallback([this](const EffectSyncState &effectState)
                                 {
                                  Serial.println("Application: Effect sync received");
                                  handleSyncedEffects(effectState); });

  Serial.println("Application: Auto-join enabled - devices will automatically pair");
#endif

  // set brightness to 100
  ledManager->setBrightness(255);
}

void Application::updateInputs()
{
  accOnInput.update();
  leftIndicatorInput.update();
  rightIndicatorInput.update();
  headlightInput.update();
  brakeInput.update();
  reverseInput.update();
}

/*
 * update():
 * Main loop update.
 */
void Application::loop()
{

  timeProfiler.increment("appFps");
  timeProfiler.start("appLoop", TimeUnit::MICROSECONDS);

  // Update input states.
  timeProfiler.start("updateInputs", TimeUnit::MICROSECONDS);
  updateInputs();
  timeProfiler.stop("updateInputs");

  // handle remote dissconnection
  if (lastRemotePing != 0 && millis() - lastRemotePing > 2000)
  {
    if (mode == ApplicationMode::TEST || mode == ApplicationMode::REMOTE)
    {
      lastRemotePing = 0;
      mode = ApplicationMode::NORMAL;

      // disable all effects
      LEDEffect::disableAllEffects();
    }
  }

  // handle brake tap sequence. always check if this is triggered
  brakeTapSequence3->setInput(brakeInput.get());
  brakeTapSequence3->loop();

  timeProfiler.start("updateMode", TimeUnit::MICROSECONDS);
  switch (mode)
  {
  case ApplicationMode::NORMAL:
    handleNormalEffects();
    break;

  case ApplicationMode::TEST:
  {
    // handleTestEffects();
    handleNormalEffects();
  }
  break;

  case ApplicationMode::REMOTE:
    handleRemoteEffects();
    break;

  case ApplicationMode::OFF:
  {
    // turn off all effects
    LEDEffect::disableAllEffects();
  }
  break;
  }

  if (mode != prevMode)
  {
    prevMode = mode;
    switch (mode)
    {
    case ApplicationMode::NORMAL:
      statusLed1.setColor(0, 255, 0);
      // statusLeds.show();
      break;
    case ApplicationMode::TEST:
      statusLed1.setColor(255, 0, 255);
      // statusLeds.show();
      break;
    case ApplicationMode::REMOTE:
      statusLed1.setColor(0, 0, 255);
      // statusLeds.show();
      break;
    case ApplicationMode::OFF:
      statusLed1.setColor(255, 0, 0);
      // statusLeds.show();
      break;
    }
  }

  timeProfiler.stop("updateMode");

  // Update SyncManager
  timeProfiler.start("updateSync", TimeUnit::MICROSECONDS);
  SyncManager *syncMgr = SyncManager::getInstance();
  syncMgr->loop();

  // Update synchronized LED blinking
  static uint32_t lastSyncedUpdate = 0;
  if (millis() - lastSyncedUpdate > 20)
  {
    lastSyncedUpdate = millis();
    syncMgr->updateSyncedLED();
  }

  timeProfiler.stop("updateSync");

  // Update and draw LED effects.
  timeProfiler.start("updateEffects", TimeUnit::MICROSECONDS);
  LEDStripManager::getInstance()->updateEffects();
  timeProfiler.stop("updateEffects");

  // LEDStripManager::getInstance()->draw();

  timeProfiler.stop("appLoop");
}

void Application::btnLoop()
{

  if (BtnBoot.clicks == 1)
  {
    // cycle through modes
    mode = static_cast<ApplicationMode>((static_cast<uint8_t>(mode) + 1) % NUM_MODES);
    preferences.putUInt("mode", static_cast<uint8_t>(mode));
  }
}

void Application::enableNormalMode()
{
  mode = ApplicationMode::NORMAL;
  preferences.putUInt("mode", static_cast<uint8_t>(mode));

  // clear all overrides
  accOnInput.clearOverride();
  leftIndicatorInput.clearOverride();
  rightIndicatorInput.clearOverride();
  headlightInput.clearOverride();
  brakeInput.clearOverride();
  reverseInput.clearOverride();
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
  LEDEffect::disableAllEffects();
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

void Application::handleSyncedEffects(const EffectSyncState &effectState)
{
  rgbEffect->setSyncData(effectState.rgbSyncData);
  nightriderEffect->setSyncData(effectState.nightRiderSyncData);
  policeEffect->setSyncData(effectState.policeSyncData);
  solidColorEffect->setSyncData(effectState.solidColorSyncData);
}
