// application.cpp

#include "Application.h"
#include "config.h"
#include "IO/Wireless.h"
#include "IO/LED/LEDStripManager.h"
#include "Sync/SyncManager.h"
#include "IO/StatusLed.h"
#include "IO/TimeProfiler.h"
#include <math.h>

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

  appInitialized = false;
}

/*
 * Begin: Initializes the LED strip, effects, wireless, etc.
 */
void Application::begin()
{
  if (appInitialized)
    return;

  appInitialized = true;

  mode = static_cast<ApplicationMode>(preferences.getUInt("mode", 0));
  prevMode = mode == ApplicationMode::NORMAL ? ApplicationMode::OFF : ApplicationMode::NORMAL;

  LEDStripManager *ledManager = LEDStripManager::getInstance();
  ledManager->begin();

  ledManager->setBrightness(255);

  ledManager->startTask();

  if (ledConfig.headlightsEnabled)
  {
    LEDStripConfig headlights(
        LEDStripType::HEADLIGHT,
        new LEDStrip(ledConfig.headlightLedCount, OUTPUT_LED_1_PIN),
        "Headlights");
    headlights.strip->setFliped(ledConfig.headlightFlipped);
    headlights.strip->setEnabled(ledConfig.headlightsEnabled);

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
    taillights.strip->setEnabled(ledConfig.taillightsEnabled);

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
    underglow.strip->setEnabled(ledConfig.underglowEnabled);
    LEDSegment *underglowSegmentL = new LEDSegment(underglow.strip, "Underglow-Left", 0, 102);    // 0 - 101
    LEDSegment *underglowSegmentF = new LEDSegment(underglow.strip, "Underglow-Front", 102, 96);  // 102 - 197
    LEDSegment *underglowSegmentR = new LEDSegment(underglow.strip, "Underglow-Right", 198, 102); // 198 - 299

    ledManager->addLEDStrip(underglow);

    underglow.strip->getBuffer()[0] = Color(255, 0, 0);
    delay(500);
    underglow.strip->getBuffer()[0] = Color(0, 0, 0);

    // underglow.strip->getBuffer()[101] = Color(255, 0, 0);

    // underglow.strip->getBuffer()[197] = Color(255, 0, 0);

    // underglow.strip->getBuffer()[299] = Color(255, 0, 0);
    // underglow.strip->getBuffer()[300] = Color(0, 0, 255);

    // for (int i = 0; i < underglowSegmentL->getNumLEDs(); i++) // quick test segments
    //   underglowSegmentL->getBuffer()[i] = Color(255, 0, 0);
    // for (int i = 0; i < underglowSegmentF->getNumLEDs(); i++)
    //   underglowSegmentF->getBuffer()[i] = Color(0, 255, 0);
    // for (int i = 0; i < underglowSegmentR->getNumLEDs(); i++)
    //   underglowSegmentR->getBuffer()[i] = Color(0, 0, 255);
    // while (true)
    //   ;
  }

  if (ledConfig.interiorEnabled)
  {
    LEDStripConfig interior(
        LEDStripType::INTERIOR,
        new LEDStrip(ledConfig.interiorLedCount, OUTPUT_LED_4_PIN),
        "Interior");
    interior.strip->setFliped(ledConfig.interiorFlipped);
    interior.strip->setEnabled(ledConfig.interiorEnabled);

    ledManager->addLEDStrip(interior);

    interior.strip->getBuffer()[0] = Color(255, 0, 0);
    delay(500);
    interior.strip->getBuffer()[0] = Color(0, 0, 0);
  }

  setupEffects();
  setupSequences();
  setupWireless();
  setupBLE();

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

#endif

  // unlockSequence->trigger(); // car is most likely unlocked when device is powered on
  unlockSequence->setActive(false);

  // serviceLightsEffect->setActive(true);
  // serviceLightsEffect->setMode(ServiceLightsMode::STROBE);
}

ApplicationMode Application::getMode()
{
  return mode;
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
  if (!appInitialized)
    return;

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
    SyncManager::getInstance()->setSyncMode(SyncMode::SOLO);
  }
  break;
  }

  if (mode != prevMode)
  {
    prevMode = mode;
  }

  timeProfiler.stop("updateMode");

  // Update SyncManager
  timeProfiler.start("updateSync", TimeUnit::MICROSECONDS);
  SyncManager *syncMgr = SyncManager::getInstance();
  syncMgr->loop();

  timeProfiler.stop("updateSync");

  // Update BLE
  timeProfiler.start("updateBLE", TimeUnit::MICROSECONDS);
  BLEManager *bleManager = BLEManager::getInstance();
  bleManager->loop();
  timeProfiler.stop("updateBLE");

  // Update and draw LED effects.
  timeProfiler.start("updateEffects", TimeUnit::MICROSECONDS);
  LEDStripManager::getInstance()->updateEffects();
  timeProfiler.stop("updateEffects");

  // LEDStripManager::getInstance()->draw();

  timeProfiler.stop("appLoop");
}

void Application::enableNormalMode()
{
  mode = ApplicationMode::NORMAL;
  preferences.putUInt("mode", static_cast<int>(mode));

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
}

void Application::handleRemoteEffects()
{
  SyncManager *syncMgr = SyncManager::getInstance();
  bool isSyncing = syncMgr->isInGroup() && syncMgr->getGroupInfo().members.size() > 1;
  bool isMaster = syncMgr->isGroupMaster();

  if (isMaster && syncMgr->isEffectSyncEnabled())
  {
    EffectSyncState effectState = {};

    effectState.rgbSyncData = rgbEffect->getSyncData();
    effectState.nightRiderSyncData = nightriderEffect->getSyncData();
    effectState.policeSyncData = policeEffect->getSyncData();
    effectState.solidColorSyncData = solidColorEffect->getSyncData();
    effectState.colorFadeSyncData = colorFadeEffect->getSyncData();
    effectState.commitSyncData = commitEffect->getSyncData();

    syncMgr->setEffectSyncState(effectState);
  }
}

void Application::handleSyncedEffects(const EffectSyncState &effectState)
{
  rgbEffect->setSyncData(effectState.rgbSyncData);
  nightriderEffect->setSyncData(effectState.nightRiderSyncData);
  policeEffect->setSyncData(effectState.policeSyncData);
  solidColorEffect->setSyncData(effectState.solidColorSyncData);
  colorFadeEffect->setSyncData(effectState.colorFadeSyncData);
  commitEffect->setSyncData(effectState.commitSyncData);
  serviceLightsEffect->setSyncData(effectState.serviceLightsSyncData);
}

void Application::setupBLE()
{
  Serial.println("Application: Setting up BLE...");

  BLEManager *bleManager = BLEManager::getInstance();
  bleManager->setApplication(this);
  bleManager->begin();

  Serial.println("Application: BLE setup complete");
}
