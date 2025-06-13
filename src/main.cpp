// main.cpp

// #include "secrets.h"
#include "config.h"

#include "IO/GPIO.h"
#include "IO/Wireless.h"
#include "Application.h"
#include "SerialMenu.h"
#include "IO/StatusLed.h"
#include "IO/Display.h"
#include "IO/ScreenManager.h"

#include "Screens/Screens.h"

StartUpScreen startUpScreen("Start Up");
HomeScreen homeScreen("Home");
ShutdownScreen shutdownScreen("Shutdown");
ApplicationScreen applicationScreen("Application");
SyncScreen syncScreen("Sync");
SettingsScreen settingsScreen("Settings");
ProvisioningRequiredScreen provisioningRequiredScreen("Provisioning Required");
DebugScreen debugScreen("Debug");
IOTestScreen ioTestScreen("IO Test");
BatteryScreen batteryScreen("Battery");

Application *app;

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(10);

  GpIO::initIO();
  statusLeds.begin();
  statusLeds.setBrightness(50);
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

  display.init();
  screenManager.init();

  screenManager.addScreen(&startUpScreen);
  screenManager.addScreen(&homeScreen);
  screenManager.addScreen(&shutdownScreen);
  screenManager.addScreen(&applicationScreen);
  screenManager.addScreen(&syncScreen);
  screenManager.addScreen(&settingsScreen);
  screenManager.addScreen(&provisioningRequiredScreen);
  screenManager.addScreen(&debugScreen);
  screenManager.addScreen(&ioTestScreen);
  screenManager.addScreen(&batteryScreen);

  screenManager.setScreen("Start Up");
  ((StartUpScreen *)screenManager.getCurrentScreen())->setStage(1);
  display.display();

  WiFi.mode(WIFI_AP_STA);

  statusLed1.setOverideColor(0, 255, 0);
  statusLed2.setOverideColor(0, 255, 0);

  long bootCount = preferences.getLong("bootCount", 0);
  bootCount++;
  preferences.putLong("bootCount", bootCount);

  Serial.println("[INFO] [CONFIG] Boot count: " + String(bootCount));

  ((StartUpScreen *)screenManager.getCurrentScreen())->setStage(2);

  if (isDeviceProvisioned())
  {
    Serial.println("[INFO] [CONFIG] Device is provisioned, starting normal operation");

    wireless.setup();

    app = Application::getInstance();

    if (!app)
    {
      Serial.println("Failed to create Application instance.");
      return;
    }

    app->begin();

    // xTaskCreatePinnedToCore(
    //     [](void *pvParameters)
    //     {
    //       // statusLed1.setMode(RGB_MODE::Rainbow);
    //       // statusLed2.setMode(RGB_MODE::Rainbow);

    //       statusLed1.setMode(RGB_MODE::Overide);
    //       statusLed2.setMode(RGB_MODE::Overide);

    //       statusLed1.setOverideColor(255, 0, 0);
    //       statusLed2.setOverideColor(255, 0, 0);

    //       vTaskDelay(300 / portTICK_PERIOD_MS);

    //       statusLed1.setOverideColor(0, 255, 0);
    //       statusLed2.setOverideColor(0, 255, 0);

    //       vTaskDelay(300 / portTICK_PERIOD_MS);

    //       statusLed1.setOverideColor(0, 0, 255);
    //       statusLed2.setOverideColor(0, 0, 255);

    //       vTaskDelay(300 / portTICK_PERIOD_MS);

    //       statusLed1.setOverideColor(0, 0, 0);
    //       statusLed2.setOverideColor(0, 0, 0);

    //       statusLed1.goBackSteps(1);
    //       statusLed2.goBackSteps(1);

    //       vTaskDelete(NULL);
    //     },
    //     "startup_blink", 10000, NULL, 1, NULL, 1);
  }
  else
  {
    Serial.println("[INFO] [CONFIG] Device not provisioned, limiting functionality");

    statusLed1.setPulsingColor(0xFF0000); // Red to indicate provisioning mode
    statusLed2.setPulsingColor(0xFF0000); // Red to indicate provisioning mode
    statusLed1.setMode(RGB_MODE::Pulsing);
    statusLed2.setMode(RGB_MODE::Pulsing);
  }

  ((StartUpScreen *)screenManager.getCurrentScreen())->setStage(3);

  if (isDeviceProvisioned())
  {
    statusLed1.setOverideColor(0, 0, 255);
    statusLed2.setOverideColor(0, 0, 255);

    vTaskDelay(300 / portTICK_PERIOD_MS);

    statusLed1.goBackSteps(1);
    statusLed2.goBackSteps(1);

    screenManager.setScreen("Home");
  }
  else
  {
    // Show provisioning required screen and keep red status LEDs
    screenManager.setScreen("Provisioning Required");
  }

  initSerialMenu();
}

unsigned long batteryLoopMs = 0;
unsigned long lastDraw = 0;

void loop()
{
  unsigned long currentTime = millis();
  wireless.loop();

  if (isDeviceProvisioned())
  {
    app->loop();
  }

  if (millis() - batteryLoopMs > 500)
  {
    batteryLoopMs = millis();
    batteryUpdate();
  }

  if (millis() - lastDraw > 25)
  {
    lastDraw = millis();

    BtnBoot.Update();
    BtnPrev.Update();
    BtnSel.Update();
    BtnNext.Update();

    display.display();
  }

  try
  {
    if (Serial.available() > 0)
    {
      String input = Serial.readString();
      processMenuInput(input);
    }
  }
  catch (...)
  {
    Serial.println("[ERROR] Exception in serialTask");
  }
}
