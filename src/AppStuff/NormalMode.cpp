#include "Application.h"
#include "Sync/SyncManager.h"

void Application::handleNormalEffects()
{
  unsigned long currentTime = millis();

#ifdef ENABLE_SYNC
  SyncManager *syncMgr = SyncManager::getInstance();
  bool isSyncing = syncMgr->isInGroup() && syncMgr->getGroupInfo().members.size() > 1;
  bool isMaster = syncMgr->isGroupMaster();
#else
  bool isSyncing = false;
  bool isMaster = false;
#endif

  unlockSequence->setInputs(accOnInput.get(), leftIndicatorInput.get(), rightIndicatorInput.get());
  lockSequence->setInputs(accOnInput.get(), leftIndicatorInput.get(), rightIndicatorInput.get());
  RGBFlickSequence->setInputs(accOnInput.get(), leftIndicatorInput.get(), rightIndicatorInput.get());
  nightRiderFlickSequence->setInputs(accOnInput.get(), leftIndicatorInput.get(), rightIndicatorInput.get());

  unlockSequence->loop();
  lockSequence->loop();
  RGBFlickSequence->loop();
  nightRiderFlickSequence->loop();

  policeEffect->setActive(false); // ensure police effect is off

  if (accOnInput.getLastActiveTime() != 0 && currentTime - accOnInput.getLastActiveTime() > 1 * 60 * 1000)
  {
    accOnInput.setLastActiveTime(0);
    unlockSequence->setActive(true);

    LEDEffect::disableAllEffects();
  }

  if (accOnInput.getLast() != accOnInput.get() && accOnInput.get() == false) // acc just went off
  {
    if (taillightEffect)
    {
      taillightEffect->setStartup();
    }
    // headlightStartupEffect->setStartup();
  }

  bool debugSync = false;
#ifdef DEBUG_SYNC
  debugSync = true;
#endif

  if (accOnInput.get() == false) // acc is off
  {
    // Since ACC is off, disable the other effects.
    leftIndicatorEffect->setActive(false);
    rightIndicatorEffect->setActive(false);

    brakeEffect->setActive(false);
    brakeEffect->setIsReversing(false);
    reverseLightEffect->setActive(false);
    policeEffect->setActive(false);

    pulseWaveEffect->setActive(false);
    auroraEffect->setActive(false);

    rgbEffect->setActive(false);
    nightriderEffect->setActive(false);
    solidColorEffect->setActive(false);
    colorFadeEffect->setActive(false);
    commitEffect->setActive(false);
  }

  if (accOnInput.getLast() != accOnInput.get() && accOnInput.get() == true) // acc just went on
  {
    if (taillightEffect)
    {
      taillightEffect->setDim();
      headlightEffect->setCarOn();
    }
  }

  if (accOnInput.get() == true) // acc is on
  {
    // Only apply physical input controls if we're the master
    // or we're not syncing with other devices
    if (!isSyncing || isMaster)
    {
      // And process the other effects normally.
      leftIndicatorEffect->setActive(leftIndicatorInput.get());
      rightIndicatorEffect->setActive(rightIndicatorInput.get());

      // Keep separate brake and reverse effects
      brakeEffect->setActive(brakeInput.get());
      brakeEffect->setIsReversing(reverseInput.get() || reverseLightEffect->isAnimating());
      reverseLightEffect->setActive(reverseInput.get());

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
  }
}