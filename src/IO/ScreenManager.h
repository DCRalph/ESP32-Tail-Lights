#pragma once

#include "config.h"
#ifdef ENABLE_DISPLAY
#include "IO/GPIO.h"
#include "IO/Display.h"
#include <vector>

// class Screen;
class Screen2;

// screen architecture v2
class ScreenManager
{
private:
  const Screen2 *currentScreen = nullptr;
  const Screen2 *pendingScreen = nullptr;
  std::vector<const Screen2 *> screenHistory;

  void updateHistory(const Screen2 *screen);

public:
  void init(void);

  void update(void);
  void draw(void);

  const Screen2 *getCurrentScreen(void);

  void setScreen(const Screen2 *screen);
  bool applyPendingScreenChange();

  void back(void);
  void clearHistory(void);
  bool goToHistoryIndex(size_t index);
};

struct Screen2
{
  // Screen2(const char *name, void (*draw)(), void (*update)(), void (*onEnter)(), void (*onExit)());
  const String name;
  const String topBarText;
  void (*draw)();
  void (*update)();
  void (*onEnter)();
  void (*onExit)();
};

extern ScreenManager screenManager;

#endif