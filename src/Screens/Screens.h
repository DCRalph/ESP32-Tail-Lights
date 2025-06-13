#pragma once

// Include all screen headers
#include "StartUp.h"
#include "Home.h"
#include "Shutdown.h"
#include "ApplicationScreen.h"
#include "SyncScreen.h"
#include "SettingsScreen.h"
#include "Debug/Debug.h"
#include "Debug/IOTest.h"
#include "Debug/Battery.h"

// Declare screen instances
extern StartUpScreen startUpScreen;
extern HomeScreen homeScreen;
extern ShutdownScreen shutdownScreen;
extern ApplicationScreen applicationScreen;
extern SyncScreen syncScreen;
extern SettingsScreen settingsScreen;
extern DebugScreen debugScreen;
extern IOTestScreen ioTestScreen;
extern BatteryScreen batteryScreen;