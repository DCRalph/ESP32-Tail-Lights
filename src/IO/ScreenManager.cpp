#include "ScreenManager.h"
#ifdef ENABLE_DISPLAY

static const char *TAG = "SCREEN_MANAGER";

void ScreenManager::init(void)
{
  pendingScreen = nullptr;
  Serial.println("[SCREEN_MANAGER] Initialized");
}

void ScreenManager::update(void)
{
  if (currentScreen && currentScreen->update)
    currentScreen->update();
  else
    Serial.println("[SCREEN_MANAGER] WARNING: No screen to update");

  if (applyPendingScreenChange())
    update();
}

void ScreenManager::draw(void)
{
  if (currentScreen && currentScreen->draw)
    currentScreen->draw();
  else
    Serial.println("[SCREEN_MANAGER] WARNING: No screen to draw");
}

const Screen2 *ScreenManager::getCurrentScreen(void)
{
  if (pendingScreen)
    return pendingScreen;
  return currentScreen;
}

bool ScreenManager::applyPendingScreenChange()
{
  if (pendingScreen != nullptr)
  {

    if (currentScreen)
    {
      Serial.print("[SCREEN_MANAGER] From: ");
      Serial.print(currentScreen->name);
      Serial.print(" To: ");
      Serial.println(pendingScreen->name);
    }
    else
    {
      Serial.print("[SCREEN_MANAGER] To: ");
      Serial.println(pendingScreen->name);
    }

    if (currentScreen && currentScreen->onExit)
      currentScreen->onExit();

    currentScreen = pendingScreen;
    pendingScreen = nullptr;

    Serial.print("[SCREEN_MANAGER] >> ");
    Serial.println(currentScreen->name);

    updateHistory(currentScreen);

    if (currentScreen && currentScreen->onEnter)
      currentScreen->onEnter();

    return true;
  }

  return false;
}

void ScreenManager::setScreen(const Screen2 *screen)
{
  pendingScreen = screen;
}

void ScreenManager::back(void)
{
  Serial.println("[SCREEN_MANAGER] Screen history:");
  for (size_t i = 0; i < screenHistory.size(); i++)
  {
    Serial.print("[SCREEN_MANAGER] [");
    Serial.print(i);
    Serial.print("] ");
    Serial.println(screenHistory[i]->name);
  }

  if (screenHistory.size() > 1)
  {
    screenHistory.pop_back();
    pendingScreen = screenHistory.back();
    Serial.print("[SCREEN_MANAGER] Going back to: ");
    Serial.println(pendingScreen->name);
  }
  else
  {
    Serial.println("[SCREEN_MANAGER] Cannot go back - no screen history available");
  }
}

void ScreenManager::clearHistory(void)
{
  Serial.println("[SCREEN_MANAGER] Clearing screen history");

  const Screen2 *current = nullptr;
  if (screenHistory.size() > 0)
  {
    current = screenHistory.back();
  }

  screenHistory.clear();

  if (current != nullptr)
    screenHistory.push_back(current);
}

void ScreenManager::updateHistory(const Screen2 *screen)
{
  if (screen == nullptr)
  {
    Serial.println("[SCREEN_MANAGER] WARNING: Attempted to add null screen to history");
    return;
  }

  if (screenHistory.empty() || screenHistory.back() != screen)
  {
    Serial.print("[SCREEN_MANAGER] Adding screen to history: ");
    Serial.println(screen->name);
    screenHistory.push_back(screen);

    if (screenHistory.size() > 10)
      screenHistory.erase(screenHistory.begin());
  }
}

bool ScreenManager::goToHistoryIndex(size_t index)
{
  if (index < screenHistory.size())
  {
    Serial.print("[SCREEN_MANAGER] Navigating to history index ");
    Serial.print(index);
    Serial.print(": ");
    Serial.println(screenHistory[index]->name);

    if (index != screenHistory.size() - 1)
    {
      pendingScreen = screenHistory[index];
      screenHistory.resize(index + 1);

      return true;
    }

    Serial.println("[SCREEN_MANAGER] Already at requested screen");
    return false;
  }

  Serial.print("[SCREEN_MANAGER] WARNING: Invalid history index: ");
  Serial.print(index);
  Serial.print(", max: ");
  Serial.println(screenHistory.size() ? screenHistory.size() - 1 : 0);
  return false;
}

ScreenManager screenManager;

#endif