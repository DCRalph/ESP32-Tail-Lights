#include "Application.h"
#include "IO/StatusLed.h"
#include "Sync/SyncManager.h"

void Application::btnLoop()
{
  if (!appInitialized)
    return;

  if (BtnBoot.clicks == 1)
  {
    // cycle through modes
    mode = static_cast<ApplicationMode>((static_cast<int>(mode) + 1) % NUM_MODES);
    preferences.putUInt("mode", static_cast<int>(mode));
  }

  if (BtnPrev.clicks == 1)
  {
    // cycle through modes
    SyncManager::getInstance()->setSyncMode(static_cast<SyncMode>((static_cast<int>(SyncManager::getInstance()->getSyncMode()) + 1) % 3));
  }

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

  SyncManager::getInstance()->updateSyncedLED();
}
