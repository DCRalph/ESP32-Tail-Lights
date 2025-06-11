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

  unlockSequence->setInputs(getInput(accOnInput), getInput(leftIndicatorInput), getInput(rightIndicatorInput));
  lockSequence->setInputs(getInput(accOnInput), getInput(leftIndicatorInput), getInput(rightIndicatorInput));
  RGBFlickSequence->setInputs(getInput(accOnInput), getInput(leftIndicatorInput), getInput(rightIndicatorInput));
  nightRiderFlickSequence->setInputs(getInput(accOnInput), getInput(leftIndicatorInput), getInput(rightIndicatorInput));

  unlockSequence->loop();
  lockSequence->loop();
  RGBFlickSequence->loop();
  nightRiderFlickSequence->loop();

  if (isEnabled(accOnInput) && accOnInput.getLastActiveTime() != 0 && currentTime - accOnInput.getLastActiveTime() > 1 * 60 * 1000)
  {
    accOnInput.setLastActiveTime(0);
    unlockSequence->setActive(true);

    // turn off all effects
    LEDEffect::disableAllEffects();
  }

  if (isEnabled(accOnInput) && accOnInput.getLast() != accOnInput.get() && accOnInput.get() == false)
  {
    if (taillightEffect)
    {
      taillightEffect->setStartup();
    }
    // headlightStartupEffect->setStartup();
  }

  if (isEnabled(accOnInput) && accOnInput.get() == false)
  {
    // Since ACC is off, disable the other effects.
    leftIndicatorEffect->setActive(false);
    rightIndicatorEffect->setActive(false);

    // headlightEffect->setOff();
    // headlightEffect->setSplit(false);
    // headlightEffect->setColor(false, false, false);

    brakeEffect->setActive(false);
    brakeEffect->setIsReversing(false);
    reverseLightEffect->setActive(false);

    pulseWaveEffect->setActive(false);
    auroraEffect->setActive(false);
  }
  else
  {
    if (taillightEffect)
    {
      // Set to dim mode when car is on (as requested)
      taillightEffect->setDim();
    }
    headlightEffect->setCarOn();

    // Only apply physical input controls if we're the master
    // or we're not syncing with other devices
    if (!isSyncing || isMaster)
    {
      // And process the other effects normally.
      leftIndicatorEffect->setActive(getInput(leftIndicatorInput));
      rightIndicatorEffect->setActive(getInput(rightIndicatorInput));

#ifdef ENABLE_HEADLIGHTS
      // headlightEffect->setActive(getInput(highBeamInput));
#endif

#ifdef ENABLE_TAILLIGHTS
      // Keep separate brake and reverse effects
      brakeEffect->setActive(getInput(brakeInput));
      brakeEffect->setIsReversing(getInput(reverseInput) || reverseLightEffect->isAnimating());
      reverseLightEffect->setActive(getInput(reverseInput));
#endif

      // If we're the master, broadcast effect states to sync with other devices
      if (isMaster && syncMgr->isEffectSyncEnabled())
      {
        EffectSyncState effectState = {};
        effectState.nightRiderActive = nightriderEffect ? nightriderEffect->isActive() : 0;
        effectState.rgbActive = rgbEffect ? rgbEffect->isActive() : 0;
        effectState.policeActive = policeEffect ? policeEffect->isActive() : 0;

        // Set sync time for night rider if it just became active
        if (effectState.nightRiderActive && syncMgr->isTimeSynced())
        {
          effectState.nightRiderSyncTime = syncMgr->getSyncedTime();
        }

        syncMgr->setEffectSyncState(effectState);
      }
    }
    // If we're syncing but not the master, effect states will be controlled
    // by the SyncManager through its callback
  }
}