#include "ScreenManager.h"
#include "esp_log.h"

static const char *TAG = "SCREEN_MANAGER";

void ScreenManager::init(void)
{
  pendingScreen = nullptr;
  ESP_LOGI(TAG, "Initialized");
}

void ScreenManager::update(void)
{
  if (currentScreen && currentScreen->update)
    currentScreen->update();
  else
    ESP_LOGW(TAG, "No screen to update");

  if (applyPendingScreenChange())
    update();
}

void ScreenManager::draw(void)
{
  if (currentScreen && currentScreen->draw)
    currentScreen->draw();
  else
    ESP_LOGW(TAG, "No screen to draw");
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
      ESP_LOGI(TAG, "From: %s To: %s", currentScreen->name, pendingScreen->name);
    else
      ESP_LOGI(TAG, "To: %s", pendingScreen->name);

    if (currentScreen && currentScreen->onExit)
      currentScreen->onExit();

    currentScreen = pendingScreen;
    pendingScreen = nullptr;

    ESP_LOGI(TAG, ">> %s", currentScreen->name);

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
  ESP_LOGI(TAG, "Screen history:");
  for (size_t i = 0; i < screenHistory.size(); i++)
  {
    ESP_LOGI(TAG, "[%d] %s", i, screenHistory[i]->name);
  }

  if (screenHistory.size() > 1)
  {
    screenHistory.pop_back();
    pendingScreen = screenHistory.back();
    ESP_LOGI(TAG, "Going back to: %s", pendingScreen->name);
  }
  else
  {
    ESP_LOGI(TAG, "Cannot go back - no screen history available");
  }
}

void ScreenManager::clearHistory(void)
{
  ESP_LOGI(TAG, "Clearing screen history");

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
    ESP_LOGW(TAG, "Attempted to add null screen to history");
    return;
  }

  if (screenHistory.empty() || screenHistory.back() != screen)
  {
    ESP_LOGI(TAG, "Adding screen to history: %s", screen->name);
    screenHistory.push_back(screen);

    if (screenHistory.size() > 10)
      screenHistory.erase(screenHistory.begin());
  }
}

bool ScreenManager::goToHistoryIndex(size_t index)
{
  if (index < screenHistory.size())
  {
    ESP_LOGI(TAG, "Navigating to history index %d: %s",
             index, screenHistory[index]->name);

    if (index != screenHistory.size() - 1)
    {
      pendingScreen = screenHistory[index];
      screenHistory.resize(index + 1);

      return true;
    }

    ESP_LOGI(TAG, "Already at requested screen");
    return false;
  }

  ESP_LOGW(TAG, "Invalid history index: %d, max: %d",
           index, screenHistory.size() ? screenHistory.size() - 1 : 0);
  return false;
}

ScreenManager screenManager;
