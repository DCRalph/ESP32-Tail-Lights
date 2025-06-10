// main.cpp

// #include "secrets.h"
#include "config.h"

#include "IO/GPIO.h"
#include "IO/Wireless.h"
#include "Application.h"
#include "SerialMenu.h"

Application *app;

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(10);
  // delay(1000);

  WiFi.mode(WIFI_AP_STA);

  GpIO::initIO();

  preferences.begin("esp", false);

  long bootCount = preferences.getLong("bootCount", 0);
  bootCount++;
  preferences.putLong("bootCount", bootCount);

  Serial.println("[INFO] [CONFIG] Boot count: " + String(bootCount));

  wireless.setup();

  app = Application::getInstance();

  if (!app)
  {
    Serial.println("Failed to create Application instance.");
    return;
  }

  app->begin();
  // app->enableTestMode();

  xTaskCreatePinnedToCore(
      [](void *pvParameters)
      {
        led.On();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        led.Off();

        vTaskDelete(NULL);
      },
      "startup_blink", 10000, NULL, 1, NULL, 1);

  initSerialMenu();
}

// unsigned long lastSerialPrintTime = 0;

void loop()
{
  unsigned long currentTime = millis();
  wireless.loop();

  app->loop();

  try
  {
    if (Serial.available() > 0)
    {
      String input = Serial.readString();
      input.trim();

      // Pass to the menu system
      processMenuInput(input);
    }
  }
  catch (...)
  {
    Serial.println("[ERROR] Exception in serialTask");
  }

  // if (currentTime - lastSerialPrintTime >= 1000)
  // {
  //   lastSerialPrintTime = currentTime;
  //   Serial.println("Loop time: " + String(currentTime - lastSerialPrintTime));
  // }
}
