#include "Debug.h"
#ifdef ENABLE_DISPLAY
#include "IO/Display.h"
#include "IO/GPIO.h"
#include "IO/Menu.h"
#include "IO/ScreenManager.h"
#include "IO/TimeProfiler.h"

#include "Screens/Debug/IOTest.h"
#include "Screens/Debug/Battery.h"

namespace DebugScreenNamespace
{
  // State variables
  static long bootCount;
  static bool ledState = false;
  static bool otaSetupTmp = false;

  static uint64_t totalMem = 0;
  static uint64_t freeMem = 0;
  static uint64_t usedMem = 0;

  static String totalMemStr = "0";
  static String freeMemStr = "0";
  static String usedMemStr = "0";

  // TimeProfiler timing variables
  static uint32_t clearBufferTime = 0;
  static uint32_t screenManagerDrawTime = 0;
  static uint32_t drawTopBarTime = 0;
  static uint32_t sendBufferTime = 0;
  static uint32_t screenUpdateTime = 0;
  static uint32_t updateInputsTime = 0;
  static uint32_t updateModeTime = 0;
  static uint32_t updateSyncTime = 0;
  static uint32_t updateEffectsTime = 0;
  static uint32_t drawEffectsTime = 0;
  static uint32_t syncManagerLoopTime = 0;
  static uint32_t handleSyncPacketTime = 0;

  // Calls per second variables
  static uint32_t ledFps = 0;
  static uint32_t displayFps = 0;
  static uint32_t packetPps = 0;
  static uint32_t appFps = 0;
  static uint32_t mainLoopFps = 0;

  // Menu instance
  static Menu menu = Menu(MenuSize::Medium);

  // Menu items
  static MenuItemBack backItem;
  static MenuItemNavigate ioTestItem = MenuItemNavigate("IO Test", &IOTestScreen);
  static MenuItemNavigate batteryItem = MenuItemNavigate("Battery", &BatteryScreen);
  static MenuItemNumber<long> bootCountItem = MenuItemNumber<long>("Boot", &bootCount);
  static MenuItemString totalMemItem = MenuItemString("tmem", &totalMemStr);
  static MenuItemString freeMemItem = MenuItemString("fmem", &freeMemStr);
  static MenuItemString usedMemItem = MenuItemString("umem", &usedMemStr);

  // Display timing items
  static MenuItem displayTimingItem = MenuItem("OLED Timing");
  static MenuItemNumber<uint32_t> clearBufferTimeItem = MenuItemNumber<uint32_t>("clrb", &clearBufferTime);
  static MenuItemNumber<uint32_t> screenManagerDrawTimeItem = MenuItemNumber<uint32_t>("smdraw", &screenManagerDrawTime);
  static MenuItemNumber<uint32_t> drawTopBarTimeItem = MenuItemNumber<uint32_t>("dtb", &drawTopBarTime);
  static MenuItemNumber<uint32_t> sendBufferTimeItem = MenuItemNumber<uint32_t>("sendb", &sendBufferTime);
  static MenuItemNumber<uint32_t> screenUpdateTimeItem = MenuItemNumber<uint32_t>("su", &screenUpdateTime);

  // Application timing items
  static MenuItem applicationTimingItem = MenuItem("App Timing");
  static MenuItemNumber<uint32_t> updateInputsTimeItem = MenuItemNumber<uint32_t>("uinput", &updateInputsTime);
  static MenuItemNumber<uint32_t> updateModeTimeItem = MenuItemNumber<uint32_t>("ummode", &updateModeTime);
  static MenuItemNumber<uint32_t> updateSyncTimeItem = MenuItemNumber<uint32_t>("usync", &updateSyncTime);
  static MenuItemNumber<uint32_t> updateEffectsTimeItem = MenuItemNumber<uint32_t>("ueeffect", &updateEffectsTime);
  static MenuItemNumber<uint32_t> drawEffectsTimeItem = MenuItemNumber<uint32_t>("deffect", &drawEffectsTime);

  // Fps items
  static MenuItem fpsItem = MenuItem("FPS");
  static MenuItemNumber<uint32_t> ledFpsItem = MenuItemNumber<uint32_t>("led", &ledFps);
  static MenuItemNumber<uint32_t> displayFpsItem = MenuItemNumber<uint32_t>("dis", &displayFps);
  static MenuItemNumber<uint32_t> packetPpsItem = MenuItemNumber<uint32_t>("pkt", &packetPps);
  static MenuItemNumber<uint32_t> appFpsItem = MenuItemNumber<uint32_t>("app", &appFps);
  static MenuItemNumber<uint32_t> mainLoopFpsItem = MenuItemNumber<uint32_t>("ml", &mainLoopFps);

    // onEnter function
  void debugScreenOnEnter()
  {
    bootCount = preferences.getLong("bootCount", 0);

    // Setup menu
    menu.addMenuItem(&backItem);
    menu.addMenuItem(&ioTestItem);
    menu.addMenuItem(&batteryItem);
    menu.addMenuItem(&bootCountItem);
    menu.addMenuItem(&totalMemItem);
    menu.addMenuItem(&freeMemItem);
    menu.addMenuItem(&usedMemItem);

    // Display timing items
    menu.addMenuItem(&displayTimingItem);
    menu.addMenuItem(&clearBufferTimeItem);
    menu.addMenuItem(&screenManagerDrawTimeItem);
    menu.addMenuItem(&drawTopBarTimeItem);
    menu.addMenuItem(&sendBufferTimeItem);
    menu.addMenuItem(&screenUpdateTimeItem);

    // Application timing items
    menu.addMenuItem(&applicationTimingItem);
    menu.addMenuItem(&updateInputsTimeItem);
    menu.addMenuItem(&updateModeTimeItem);
    menu.addMenuItem(&updateSyncTimeItem);
    menu.addMenuItem(&updateEffectsTimeItem);
    menu.addMenuItem(&drawEffectsTimeItem);

    // FPS items
    menu.addMenuItem(&fpsItem);
    menu.addMenuItem(&ledFpsItem);
    menu.addMenuItem(&displayFpsItem);
    menu.addMenuItem(&packetPpsItem);
    menu.addMenuItem(&appFpsItem);
    menu.addMenuItem(&mainLoopFpsItem);
  }

  // onExit function
  void debugScreenOnExit()
  {
    // Clean up allocated memory
  }

  // draw function
  void debugScreenDraw()
  {
    menu.draw();
  }

  // update function
  void debugScreenUpdate()
  {
    menu.update();

    totalMem = ESP.getHeapSize();
    freeMem = ESP.getFreeHeap();
    usedMem = totalMem - freeMem;

    totalMemStr = formatBytes(totalMem, true);
    freeMemStr = formatBytes(freeMem, true);
    usedMemStr = formatBytes(usedMem, true);

    // Update timing data from TimeProfiler
    clearBufferTime = timeProfiler.getTimeUs("clearBuffer");
    screenManagerDrawTime = timeProfiler.getTimeUs("screenManagerDraw");
    drawTopBarTime = timeProfiler.getTimeUs("drawTopBar");
    sendBufferTime = timeProfiler.getTimeMs("sendBuffer");
    screenUpdateTime = timeProfiler.getTimeUs("screenUpdate");
    updateInputsTime = timeProfiler.getTimeUs("updateInputs");
    updateModeTime = timeProfiler.getTimeUs("updateMode");
    updateSyncTime = timeProfiler.getTimeUs("updateSync");
    updateEffectsTime = timeProfiler.getTimeUs("updateEffects");
    drawEffectsTime = timeProfiler.getTimeUs("drawEffects");
    syncManagerLoopTime = timeProfiler.getTimeUs("syncManagerLoop");
    handleSyncPacketTime = timeProfiler.getTimeUs("handleSyncPacket");

    // Update calls per second data from TimeProfiler
    ledFps = timeProfiler.getCallsPerSecond("ledFps");
    displayFps = timeProfiler.getCallsPerSecond("displayFps");
    packetPps = timeProfiler.getCallsPerSecond("packetPps");
    appFps = timeProfiler.getCallsPerSecond("appFps");
    mainLoopFps = timeProfiler.getCallsPerSecond("mainLoopFps");
  }

} // namespace DebugScreenNamespace

// Define the DebugScreen Screen2 instance
const Screen2 DebugScreen = {
    F("Debug"),
    F("Debug"),
    DebugScreenNamespace::debugScreenDraw,
    DebugScreenNamespace::debugScreenUpdate,
    DebugScreenNamespace::debugScreenOnEnter,
    DebugScreenNamespace::debugScreenOnExit};

#endif