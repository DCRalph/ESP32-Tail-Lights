// application.cpp

#include "Application.h"
#include "config.h"
#include "IO/Wireless.h"
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
  accOnInput = HVInput(&input1, HV_HIGH_ACTIVE);
  leftIndicatorInput = HVInput(&input2, HV_HIGH_ACTIVE);
  rightIndicatorInput = HVInput(&input3, HV_HIGH_ACTIVE);

#ifdef ENABLE_HEADLIGHTS
  headlightInput = HVInput(&input5, HV_HIGH_ACTIVE);
#endif

#ifdef ENABLE_TAILLIGHTS
  brakeInput = HVInput(&input5, HV_HIGH_ACTIVE);
  reverseInput = HVInput(&input6, HV_HIGH_ACTIVE);
#endif
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

#ifdef ENABLE_TAILLIGHTS
  delete brakeTapSequence3;
#endif

#ifdef ENABLE_HEADLIGHTS
  // No headlight-specific sequences to delete yet
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
  FastLED.setBrightness(255);

#ifdef ENABLE_HEADLIGHTS
  LEDStripConfig headlights(
      LEDStripType::HEADLIGHT,
      new LEDStrip(HEADLIGHT_LED_COUNT, HEADLIGHT_LED_PIN),
      "Headlights");
#ifdef HEADLIGHT_FLIPED
  headlights.strip->setFliped(true);
#endif
  ledManager->addLEDStrip(headlights);

#endif

#ifdef ENABLE_TAILLIGHTS
  LEDStripConfig taillights(
      LEDStripType::TAILLIGHT,
      new LEDStrip(TAILLIGHT_LED_COUNT, TAILLIGHT_LED_PIN),
      "Taillights");
  ledManager->addLEDStrip(taillights);

  taillights.strip->getBuffer()[0] = Color(255, 0, 0);
  FastLED.show();
  delay(500);
  taillights.strip->getBuffer()[0] = Color(0, 0, 0);
  FastLED.show();
#endif

#ifdef ENABLE_UNDERGLOW
  LEDStripConfig underglow(
      LEDStripType::UNDERGLOW,
      new LEDStrip(UNDERGLOW_LED_COUNT, UNDERGLOW_LED_PIN),
      "Underglow");
  ledManager->addLEDStrip(underglow);
#endif

#ifdef ENABLE_INTERIOR
  LEDStripConfig interior(
      LEDStripType::INTERIOR,
      new LEDStrip(INTERIOR_LED_COUNT, INTERIOR_LED_PIN),
      "Interior");
  ledManager->addLEDStrip(interior);
#endif

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

  // Enable auto-join functionality
  syncMgr->enableAutoJoin(true);
  syncMgr->enableAutoCreate(false);
  // syncMgr->setAutoJoinTimeout(8000); // Wait 8 seconds before creating own group

  Serial.println("Application: Auto-join enabled - devices will automatically pair");
#endif

  // #ifndef ENABLE_HV_INPUTS
  //   enableTestMode();
  //   // rgbEffect->setActive(true);
  //   headlightStartupEffect->setActive(true);
  // #endif

  // set brightness to 100
  ledManager->setBrightness(255);
}

void Application::updateInputs()
{
#ifdef ENABLE_HV_INPUTS
  // Simply call update() on each HVInput instance
  accOnInput.update();
  leftIndicatorInput.update();
  rightIndicatorInput.update();
  headlightInput.update();
  brakeInput.update();
  reverseInput.update();
#endif
}

/*
 * update():
 * Main loop update.
 */
void Application::loop()
{
  // Update input states.
  uint64_t start = micros();
  updateInputs();
  stats.updateInputTime = micros() - start;

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
#ifdef ENABLE_TAILLIGHTS
  brakeTapSequence3->setInput(brakeInput.get());
  brakeTapSequence3->loop();
#endif

  start = micros();
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

  stats.updateModeTime = micros() - start;

  // Update SyncManager
  start = micros();
  SyncManager *syncMgr = SyncManager::getInstance();
  syncMgr->loop();

  // Update synchronized LED blinking
  syncMgr->updateSyncedLED();

  stats.updateSyncTime = micros() - start;

#ifdef ENABLE_SYNC
  // TODO: Implement effect synchronization on top of the core sync foundation
  // This is where you would:
  // 1. Collect current effect states
  // 2. Send effect updates to other devices (if master)
  // 3. Apply effect updates from master (if slave)
  // 4. Use synchronized time for coordinated effects

  // Example of how to check sync status:
  if (syncMgr->isInGroup())
  {
    // We have a group - check if we're syncing with others
    const GroupInfo &groupInfo = syncMgr->getGroupInfo();
    if (groupInfo.members.size() > 1)
    {
      // We have other devices in the network
      if (syncMgr->isGroupMaster())
      {
        // We're the master - we can broadcast effect states
        // TODO: Implement master effect broadcasting
      }
      else
      {
        // We're a slave - we should listen for effect states
        // TODO: Implement slave effect listening
      }
    }
  }
#endif

  // Update and draw LED effects.
  start = micros();
  LEDStripManager::getInstance()->updateEffects();
  stats.updateEffectsTime = micros() - start;

  start = micros();
  LEDStripManager::getInstance()->draw();
  stats.drawTime = micros() - start;

  loopsPerSecond++;
  if (millis() - lastLoopsTime >= 1000)
  {
    lastLoopsTime = millis();
    stats.loopsPerSecond = loopsPerSecond;
    loopsPerSecond = 0;
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
  // Apply synchronized effect states
  // This is called when we receive effect sync data from the master

  if (nightriderEffect)
  {
    bool wasActive = nightriderEffect->isActive();
    nightriderEffect->setActive(effectState.nightRiderActive);

    // If night rider is becoming active and we have sync time, use it for synchronization
    if (effectState.nightRiderActive && !wasActive && effectState.nightRiderSyncTime != 0)
    {
      // The sync time is when the master activated the effect
      // We need to calculate our position based on the time elapsed since then
      SyncManager *syncMgr = SyncManager::getInstance();
      if (syncMgr->isTimeSynced())
      {
        uint32_t currentSyncTime = syncMgr->getSyncedTime();
        uint32_t timeElapsed = currentSyncTime - effectState.nightRiderSyncTime;

        Serial.println("[EffectSync] Synchronizing NightRider - elapsed: " + String(timeElapsed) + "ms");

        // The NightRiderEffect will automatically sync using the getSyncedTime() method
        // since we modified it to use synchronized timing
      }
    }
  }

  if (rgbEffect)
  {
    rgbEffect->setActive(effectState.rgbActive);
  }

  if (policeEffect)
  {
    policeEffect->setActive(effectState.policeActive);
  }

  Serial.println("[EffectSync] Applied synced effects - NightRider: " +
                 String(effectState.nightRiderActive ? "ON" : "OFF") +
                 ", RGB: " + String(effectState.rgbActive ? "ON" : "OFF") +
                 ", Police: " + String(effectState.policeActive ? "ON" : "OFF"));
}
