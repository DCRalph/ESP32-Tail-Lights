#pragma once

#include "config.h"
#include "IO/ScreenManager.h"

enum ShutdownState
{
  Countdown,
  Shutdown_Warning,
  Shutdown
};

extern const Screen2 ShutdownScreen;