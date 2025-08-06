// main.cpp

// #include "secrets.h"
#include "config.h"

#include "IO/GPIO.h"
#include "IO/Wireless.h"
#include "Application.h"
#include "SerialMenu.h"
#include "IO/StatusLed.h"
#include "IO/Battery.h"
#ifdef ENABLE_DISPLAY
#include "IO/Display.h"
#endif
#include "IO/ScreenManager.h"
#include "IO/TimeProfiler.h"

#include "Screens/StartUp.h"
#include "Screens/Home.h"

Application *app;

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(10);

  fastledMutex = xSemaphoreCreateMutex();

  GpIO::initIO();
  statusLeds.begin();
  statusLeds.setBrightness(20);
  statusLed1.begin(&statusLeds, statusLeds.getLedPtr(0));
  statusLed2.begin(&statusLeds, statusLeds.getLedPtr(1));
  statusLeds.startShowTask();

  statusLed1.setMode(RGB_MODE::Overide);
  statusLed2.setMode(RGB_MODE::Overide);

  statusLed1.setOverideColor(255, 0, 0);
  statusLed2.setOverideColor(255, 0, 0);

  // Initialize random seed with multiple entropy sources
  uint64_t macVal = ESP.getEfuseMac();
  uint32_t millisVal = millis();
  uint32_t microsVal = micros();

  // Combine entropy sources for initial seed
  uint32_t initialSeed = (uint32_t)(macVal ^ (macVal >> 32)) ^ millisVal ^ microsVal;
  randomSeed(initialSeed);

  preferences.begin("esp", false);
  loadDeviceInfo();
  loadLEDConfig();

  timeProfiler.begin();

#ifdef ENABLE_DISPLAY
  if (deviceInfo.oledEnabled)
  {
    display.init();
    screenManager.init();

    screenManager.setScreen(&StartUpScreen);
    startUpScreenSetStage(1);
    display.display();
  }
  else
  {
    Serial.println("[INFO] [CONFIG] OLED display disabled, skipping display initialization");
  }
#endif

  WiFi.mode(WIFI_AP_STA);

  statusLed1.setOverideColor(0, 255, 0);
  statusLed2.setOverideColor(0, 255, 0);

  long bootCount = preferences.getLong("bootCount", 0);
  bootCount++;
  preferences.putLong("bootCount", bootCount);

  Serial.println("[INFO] [CONFIG] Boot count: " + String(bootCount));

  // Update startup screen if OLED is enabled
#ifdef ENABLE_DISPLAY
  if (deviceInfo.oledEnabled)
  {
    startUpScreenSetStage(2);
    display.display();
  }
#endif

  wireless.setup();

  app = Application::getInstance();

  if (!app)
  {
    Serial.println("Failed to create Application instance.");
    return;
  }

  app->begin();

  // Update screen display if OLED is enabled
#ifdef ENABLE_DISPLAY
  if (deviceInfo.oledEnabled)
  {
    startUpScreenSetStage(3);
    display.display();
  }
#endif

  statusLed1.setOverideColor(0, 0, 255);
  statusLed2.setOverideColor(0, 0, 255);

  vTaskDelay(300 / portTICK_PERIOD_MS);

  statusLed1.goBackSteps(1);
  statusLed2.goBackSteps(1);

  // Set home screen if OLED is enabled
#ifdef ENABLE_DISPLAY
  if (deviceInfo.oledEnabled)
  {
    screenManager.setScreen(&HomeScreen);
    display.display();
  }
#endif

  initSerialMenu();
}

unsigned long batteryLoopMs = 0;
unsigned long lastApp = 0;
unsigned long lastDraw = 0;

void loop()
{
  unsigned long currentTime = millis();
  timeProfiler.start("mainLoop", TimeUnit::MICROSECONDS);
  timeProfiler.increment("mainLoopFps");

  wireless.loop(); // does nothing

  if (millis() - batteryLoopMs > 1000)
  {
    batteryLoopMs = millis();
    timeProfiler.start("batteryUpdate", TimeUnit::MICROSECONDS);
    batteryUpdate(); // ~130 us
    timeProfiler.stop("batteryUpdate");
  }

  if (millis() - lastApp > (10))
  {
    lastApp = millis();
    app->loop(); // ~1000 us most of the time ~5000 sometimes
  }

  if (millis() - lastDraw > 35)
  {
    lastDraw = millis();

    timeProfiler.start("btnUpdate", TimeUnit::MICROSECONDS);
    BtnBoot.Update();
    BtnPrev.Update();
    BtnSel.Update();
    BtnNext.Update();
    timeProfiler.stop("btnUpdate");

    app->btnLoop();

    // Only update display if screen is enabled
#ifdef ENABLE_DISPLAY
    if (deviceInfo.oledEnabled)
    {
      display.display(); //  ~22000 us
    }
#endif
  }

  if (Serial.available() > 0)
  {
    String input = Serial.readString();
    processMenuInput(input); // idk probably fuck all
  }

  timeProfiler.stop("mainLoop"); // ~ 1300 us avg, slowest ~24000 us
}
