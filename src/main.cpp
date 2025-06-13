// main.cpp

// #include "secrets.h"
#include "config.h"

#include "IO/GPIO.h"
#include "IO/Wireless.h"
#include "Application.h"
#include "SerialMenu.h"
#include "IO/StatusLed.h"

Application *app;

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(10);

  // Initialize random seed with multiple entropy sources
  uint64_t macVal = ESP.getEfuseMac();
  uint32_t millisVal = millis();
  uint32_t microsVal = micros();

  // Combine entropy sources for initial seed
  uint32_t initialSeed = (uint32_t)(macVal ^ (macVal >> 32)) ^ millisVal ^ microsVal;
  randomSeed(initialSeed);

  preferences.begin("esp", false);
  loadDeviceInfo();

  WiFi.mode(WIFI_AP_STA);

  GpIO::initIO();
  statusLeds.begin();
  statusLeds.setBrightness(255);
  statusLed1.begin(&statusLeds, statusLeds.getLedPtr(0));
  statusLed2.begin(&statusLeds, statusLeds.getLedPtr(1));


  long bootCount = preferences.getLong("bootCount", 0);
  bootCount++;
  preferences.putLong("bootCount", bootCount);

  Serial.println("[INFO] [CONFIG] Boot count: " + String(bootCount));

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

    xTaskCreatePinnedToCore(
        [](void *pvParameters)
        {
          statusLed1.setMode(RGB_MODE::Rainbow);
          statusLed2.setMode(RGB_MODE::Rainbow);

          led.On();
          vTaskDelay(1000 / portTICK_PERIOD_MS);
          led.Off();

          vTaskDelay(1000 / portTICK_PERIOD_MS);

          statusLed1.goBackSteps(1);
          statusLed2.goBackSteps(1);

          vTaskDelete(NULL);
        },
        "startup_blink", 10000, NULL, 1, NULL, 1);
  }
  else
  {
    Serial.println("[INFO] [CONFIG] Device not provisioned, limiting functionality");

    statusLed1.setPulsingColor(0xFF0000); // Red to indicate provisioning mode
    statusLed2.setPulsingColor(0xFF0000); // Red to indicate provisioning mode
    statusLed1.setMode(RGB_MODE::Pulsing);
    statusLed2.setMode(RGB_MODE::Pulsing);
  }

  initSerialMenu();
}

void loop()
{
  unsigned long currentTime = millis();
  wireless.loop();

  if (isDeviceProvisioned())
  {
    app->loop();
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
