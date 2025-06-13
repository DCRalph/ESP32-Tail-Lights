#include "ScreenManager.h"

ScreenManager::ScreenManager()
{
  currentScreen = -1;
}

void ScreenManager::init()
{
  Serial.println("\t[INFO] [SCREEN MANAGER] Initialized");
}

void ScreenManager::update(void)
{
  if (currentScreen == -1)
  {
    return;
  }
  screens[currentScreen]->update();
}

void ScreenManager::draw(void)
{
  if (currentScreen == -1)
  {
    return;
  }
  screens[currentScreen]->draw();
}

void ScreenManager::setScreen(int screen)
{
  if (screen == currentScreen)
  {
    Serial.println("[WARN] [SCREEN MANAGER] Screen already set");
    return;
  }

  if (currentScreen != -1)
  {
    screens[currentScreen]->onExit();
  }

  currentScreen = screen;

  Serial.println("[INFO] [SCREEN MANAGER] >> " + screens[screen]->name);

  updateHistory();

  screens[screen]->onEnter();
}

void ScreenManager::setScreen(String screenName)
{
  for (std::vector<Screen *>::size_type i = 0; i < screens.size(); i++)
  {
    if (screens[i]->name == screenName)
    {
      setScreen(i);
      return;
    }
  }
  Serial.println("[WARN] [SCREEN MANAGER] Screen not found");
}

void ScreenManager::addScreen(Screen *screen)
{
  screens.push_back(screen);

  String msg = "\t[INFO] [SCREEN MANAGER] " + screen->name + " added";
  Serial.println(msg);
}

void ScreenManager::removeScreen(int screen)
{
  screens.erase(screens.begin() + screen);
}

void ScreenManager::removeScreen(String screenName)
{
  for (std::vector<Screen *>::size_type i = 0; i < screens.size(); i++)
  {
    if (screens[i]->name == screenName)
    {
      screens.erase(screens.begin() + i);
      return;
    }
  }
  Serial.println("[WARN] [SCREEN MANAGER] Screen not found");
}

Screen *ScreenManager::getCurrentScreen(void)
{
  if (currentScreen == -1)
  {
    return nullptr;
  }
  return screens[currentScreen];
}

Screen *ScreenManager::getScreen(String screenName)
{
  for (std::vector<Screen *>::size_type i = 0; i < screens.size(); i++)
  {
    if (screens[i]->name == screenName)
    {
      return screens[i];
    }
  }
  Serial.println("[WARN] [SCREEN MANAGER] Screen not found");
  return nullptr;
}

void ScreenManager::back(void)
{
  if (screenHistory.size() > 1)
  {
    screenHistory.pop_back();

    screens[currentScreen]->onExit();

    currentScreen = screenHistory.back();

    Serial.println("[INFO] [SCREEN MANAGER] << " + screens[currentScreen]->name);

    screens[currentScreen]->onEnter();
  }
}

void ScreenManager::clearHistory(void)
{
  screenHistory.clear();
}

void ScreenManager::updateHistory(void)
{
  screenHistory.push_back(currentScreen);

  if (screenHistory.size() > 10)
  {
    screenHistory.erase(screenHistory.begin());
  }

  // for (int i = 0; i < screenHistory.size(); i++)
  // {
  //   Serial.print(screenHistory[i]);
  //   Serial.print(" ");
  // }
  // Serial.print(" ");

  // for (int i = 0; i < screenHistory.size(); i++)
  // {
  //   if (screenHistory[i] == currentScreen)
  //   {
  //     screenHistory.erase(screenHistory.begin() + i);
  //   }
  // }

  // for (int i = 0; i < screenHistory.size(); i++)
  // {
  //   Serial.print(screenHistory[i]);
  //   Serial.print(" ");
  // }
}

ScreenManager screenManager;
