#pragma once

#include "config.h"
#include "IO/ScreenManager.h"

enum StartUpState
{
  StartUp,
};

extern const Screen2 StartUpScreen;

void startUpScreenSetStage(int _stage);
