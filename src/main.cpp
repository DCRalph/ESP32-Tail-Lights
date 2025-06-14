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
#include "IO/TimeProfiler.h"

#include "Screens/StartUp.h"
#include "Screens/Home.h"
#include "Screens/ProvisioningRequired.h"

Application *app;

// Screen disable/enable feature
bool screenEnabled = true;

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

  timeProfiler.begin();

  // Initialize display and screen manager only if OLED is enabled
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

  WiFi.mode(WIFI_AP_STA);

  statusLed1.setOverideColor(0, 255, 0);
  statusLed2.setOverideColor(0, 255, 0);

  long bootCount = preferences.getLong("bootCount", 0);
  bootCount++;
  preferences.putLong("bootCount", bootCount);

  Serial.println("[INFO] [CONFIG] Boot count: " + String(bootCount));

  // Update startup screen if OLED is enabled
  if (deviceInfo.oledEnabled)
  {
    startUpScreenSetStage(2);
    display.display();
  }

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
  }
  else
  {
    Serial.println("[INFO] [CONFIG] Device not provisioned, limiting functionality");

    statusLed1.setPulsingColor(0xFF0000); // Red to indicate provisioning mode
    statusLed2.setPulsingColor(0xFF0000); // Red to indicate provisioning mode
    statusLed1.setMode(RGB_MODE::Pulsing);
    statusLed2.setMode(RGB_MODE::Pulsing);
  }

  // Update screen display if OLED is enabled
  if (deviceInfo.oledEnabled)
  {
    startUpScreenSetStage(3);
    display.display();
  }

  if (isDeviceProvisioned())
  {
    statusLed1.setOverideColor(0, 0, 255);
    statusLed2.setOverideColor(0, 0, 255);

    vTaskDelay(300 / portTICK_PERIOD_MS);

    statusLed1.goBackSteps(1);
    statusLed2.goBackSteps(1);

    // Set home screen if OLED is enabled
    if (deviceInfo.oledEnabled)
    {
      screenManager.setScreen(&HomeScreen);
      display.display();
    }
  }
  else
  {
    // Show provisioning required screen and keep red status LEDs
    if (deviceInfo.oledEnabled)
    {
      screenManager.setScreen(&ProvisioningRequiredScreen);
      display.display();
    }
  }

  initSerialMenu();
}

unsigned long batteryLoopMs = 0;
unsigned long lastDraw = 0;

void loop()
{
  unsigned long currentTime = millis();
  timeProfiler.start("mainLoop", TimeUnit::MICROSECONDS);
  timeProfiler.increment("mainLoopFps");

  wireless.loop(); // does nothing

  // if (millis() - batteryLoopMs > 500)
  // {
  //   batteryLoopMs = millis();
  //   timeProfiler.start("batteryUpdate", TimeUnit::MICROSECONDS);
  //   batteryUpdate(); // ~130 us
  //   timeProfiler.stop("batteryUpdate");
  // }

  if (isDeviceProvisioned())
    app->loop(); // ~1000 us most of the time ~5000 sometimes

  // if (millis() - lastDraw > 35)
  // {
  //   lastDraw = millis();

  //   timeProfiler.start("btnUpdate", TimeUnit::MICROSECONDS);
  //   BtnBoot.Update();
  //   BtnPrev.Update();
  //   BtnSel.Update();
  //   BtnNext.Update();
  //   timeProfiler.stop("btnUpdate");

  //   // Check for boot button long press to re-enable screen
  //   if (BtnBoot.clicks == -1)
  //   {
  //     if (!screenEnabled)
  //     {
  //       screenEnabled = true;
  //       BtnBoot.clicks = 0;
  //     }
  //   }

  //   // Only update display if screen is enabled
  //   if (screenEnabled)
  //   {
  //     display.display(); //  ~22000 us
  //   }
  // }

  if (Serial.available() > 0)
  {
    String input = Serial.readString();
    processMenuInput(input); // idk probably fuck all
  }

  timeProfiler.stop("mainLoop"); // ~ 1300 us avg, slowest ~24000 us
}
