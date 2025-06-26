/****************************************************
 * colorBufferMenu.cpp
 *
 * Implementation of the color buffer menu system.
 * This menu allows viewing color values from LED strip
 * buffers and status LED buffers for debugging purposes.
 ****************************************************/

#include "colorBufferMenu.h"
#include "mainMenu.h"
#include "SerialMenu.h"
#include <Arduino.h>
#include "IO/StatusLed.h"
#include "IO/LED/LEDStripManager.h"
#include "IO/LED/Types.h"

/****************************************************
 * colorBufferMenu object
 ****************************************************/
SerialMenu colorBufferMenu = {
    "Color Buffer Debug",
    &mainMenu,
    printColorBufferMenu,
    handleColorBufferMenuInput};

/****************************************************
 * printColorBufferMenu()
 ****************************************************/
void printColorBufferMenu(const SerialMenu &menu)
{
  Serial.println(String("\nBreadcrumb: ") + buildBreadcrumbString(&menu));
  Serial.println("╔══════════════════════════════════════════════════════════════╗");
  Serial.println("║                   COLOR BUFFER DEBUG MENU                   ║");
  Serial.println("╚══════════════════════════════════════════════════════════════╝");
  Serial.println();

  Serial.println("=== COLOR BUFFER DEBUG MENU ===");
  Serial.println("1) Show Status LED Colors");
  Serial.println("2) Show All LED Strip Buffers");
  Serial.println("3) Show Headlight Buffer");
  Serial.println("4) Show Taillight Buffer");
  Serial.println("5) Show Underglow Buffer");
  Serial.println("6) Show Interior Buffer");
  Serial.println("7) Show All Buffers (Status + Strips)");
  Serial.println("8) Continuous Monitor Mode (1 sec intervals)");
  Serial.println("b) Back to Main Menu");
  Serial.println("Press Enter to re-print this menu");
  Serial.println();
}

/****************************************************
 * handleColorBufferMenuInput()
 ****************************************************/
bool handleColorBufferMenuInput(SerialMenu &menu, const String &input)
{
  if (input == "1")
  {
    printStatusLEDBuffers();
    return true;
  }
  else if (input == "2")
  {
    printLEDStripBuffers();
    return true;
  }
  else if (input == "3")
  {
    printSpecificLEDStripBuffer("Headlight");
    return true;
  }
  else if (input == "4")
  {
    printSpecificLEDStripBuffer("Taillight");
    return true;
  }
  else if (input == "5")
  {
    printSpecificLEDStripBuffer("Underglow");
    return true;
  }
  else if (input == "6")
  {
    printSpecificLEDStripBuffer("Interior");
    return true;
  }
  else if (input == "7")
  {
    Serial.println("=== COMPLETE BUFFER DUMP ===");
    printStatusLEDBuffers();
    Serial.println();
    printLEDStripBuffers();
    return true;
  }
  else if (input == "8")
  {
    Serial.println("=== CONTINUOUS MONITOR MODE ===");
    Serial.println("Press any key to stop monitoring...");
    Serial.println();

    unsigned long lastUpdate = 0;
    while (!Serial.available())
    {
      if (millis() - lastUpdate > 1000)
      {
        lastUpdate = millis();
        Serial.println("--- Update: " + String(millis()) + " ms ---");
        printStatusLEDBuffers();
        Serial.println();
      }
      delay(50);
    }

    // Clear the input buffer
    while (Serial.available())
    {
      Serial.read();
    }

    Serial.println("Monitoring stopped.");
    return true;
  }
  else if (input == "b")
  {
    setMenu(&mainMenu);
    return true;
  }

  // If not recognized
  return false;
}

/****************************************************
 * printStatusLEDBuffers()
 ****************************************************/
void printStatusLEDBuffers()
{
  Serial.println("=== STATUS LED BUFFERS ===");

  // Get status LED buffer data through the global statusLeds object
  CRGB *led1Ptr = statusLeds.getLedPtr(0);
  CRGB *led2Ptr = statusLeds.getLedPtr(1);

  if (led1Ptr && led2Ptr)
  {
    Serial.println("[Status LED 1]");
    Serial.print("  RGB: ");
    printColorValue(led1Ptr->r, led1Ptr->g, led1Ptr->b);
    Serial.print("  Brightness: ");
    Serial.println(statusLeds.getBrightness());

    Serial.println("[Status LED 2]");
    Serial.print("  RGB: ");
    printColorValue(led2Ptr->r, led2Ptr->g, led2Ptr->b);
    Serial.print("  Brightness: ");
    Serial.println(statusLeds.getBrightness());
  }
  else
  {
    Serial.println("Error: Could not access status LED buffers");
  }
}

/****************************************************
 * printLEDStripBuffers()
 ****************************************************/
void printLEDStripBuffers()
{
  Serial.println("=== LED STRIP BUFFERS ===");

  LEDStripManager *ledManager = LEDStripManager::getInstance();
  if (!ledManager)
  {
    Serial.println("Error: LED Strip Manager not available");
    return;
  }

  // Check each strip type
  LEDStripType stripTypes[] = {
      LEDStripType::HEADLIGHT,
      LEDStripType::TAILLIGHT,
      LEDStripType::UNDERGLOW,
      LEDStripType::INTERIOR};

  const char *stripNames[] = {
      "Headlight",
      "Taillight",
      "Underglow",
      "Interior"};

  for (int i = 0; i < 4; i++)
  {
    if (ledManager->isStripEnabled(stripTypes[i]))
    {
      Serial.println("[" + String(stripNames[i]) + " Strip]");

      Color *buffer = ledManager->getStripBuffer(stripTypes[i]);
      uint16_t ledCount = ledManager->getStripLEDCount(stripTypes[i]);
      LEDStrip *strip = ledManager->getStrip(stripTypes[i]);

      if (buffer && strip)
      {
        Serial.println("  LED Count: " + String(ledCount));
        Serial.println("  Brightness: " + String(strip->getBrightness()));
        Serial.println("  FPS: " + String(strip->getFPS()));
        Serial.println("  Flipped: " + String(strip->getFliped() ? "Yes" : "No"));

        // Print first 10 LEDs (or all if less than 10)
        uint16_t printCount = min(ledCount, (uint16_t)10);
        Serial.println("  First " + String(printCount) + " LEDs:");

        for (uint16_t j = 0; j < printCount; j++)
        {
          Serial.print("    LED " + String(j) + ": ");
          printColorValue(buffer[j].r, buffer[j].g, buffer[j].b, buffer[j].w);
        }

        if (ledCount > 10)
        {
          Serial.println("    ... (" + String(ledCount - 10) + " more LEDs)");
        }
      }
      else
      {
        Serial.println("  Error: Could not access buffer");
      }
      Serial.println();
    }
    else
    {
      Serial.println("[" + String(stripNames[i]) + " Strip] - DISABLED");
    }
  }
}

/****************************************************
 * printSpecificLEDStripBuffer()
 ****************************************************/
void printSpecificLEDStripBuffer(const String &stripName)
{
  Serial.println("=== " + stripName + " STRIP BUFFER ===");

  LEDStripManager *ledManager = LEDStripManager::getInstance();
  if (!ledManager)
  {
    Serial.println("Error: LED Strip Manager not available");
    return;
  }

  LEDStripType stripType = LEDStripType::NONE;
  if (stripName.equalsIgnoreCase("Headlight"))
    stripType = LEDStripType::HEADLIGHT;
  else if (stripName.equalsIgnoreCase("Taillight"))
    stripType = LEDStripType::TAILLIGHT;
  else if (stripName.equalsIgnoreCase("Underglow"))
    stripType = LEDStripType::UNDERGLOW;
  else if (stripName.equalsIgnoreCase("Interior"))
    stripType = LEDStripType::INTERIOR;

  if (stripType == LEDStripType::NONE)
  {
    Serial.println("Error: Unknown strip type");
    return;
  }

  if (!ledManager->isStripEnabled(stripType))
  {
    Serial.println("Strip is DISABLED");
    return;
  }

  Color *buffer = ledManager->getStripBuffer(stripType);
  uint16_t ledCount = ledManager->getStripLEDCount(stripType);
  LEDStrip *strip = ledManager->getStrip(stripType);

  if (buffer && strip)
  {
    Serial.println("LED Count: " + String(ledCount));
    Serial.println("Brightness: " + String(strip->getBrightness()));
    Serial.println("FPS: " + String(strip->getFPS()));
    Serial.println("Flipped: " + String(strip->getFliped() ? "Yes" : "No"));
    Serial.println();

    Serial.println("All LED Colors:");
    for (uint16_t i = 0; i < ledCount; i++)
    {
      Serial.print("LED " + String(i) + ": ");
      printColorValue(buffer[i].r, buffer[i].g, buffer[i].b, buffer[i].w);
    }
  }
  else
  {
    Serial.println("Error: Could not access buffer");
  }
}

/****************************************************
 * printColorValue()
 ****************************************************/
void printColorValue(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
  Serial.print("R:");
  Serial.print(String(r).length() == 1 ? "00" : String(r).length() == 2 ? "0"
                                                                        : "");
  Serial.print(r);
  Serial.print(" G:");
  Serial.print(String(g).length() == 1 ? "00" : String(g).length() == 2 ? "0"
                                                                        : "");
  Serial.print(g);
  Serial.print(" B:");
  Serial.print(String(b).length() == 1 ? "00" : String(b).length() == 2 ? "0"
                                                                        : "");
  Serial.print(b);

  if (w > 0)
  {
    Serial.print(" W:");
    Serial.print(String(w).length() == 1 ? "00" : String(w).length() == 2 ? "0"
                                                                          : "");
    Serial.print(w);
  }

  Serial.print(" [#");
  if (r < 16)
    Serial.print("0");
  Serial.print(r, HEX);
  if (g < 16)
    Serial.print("0");
  Serial.print(g, HEX);
  if (b < 16)
    Serial.print("0");
  Serial.print(b, HEX);
  Serial.println("]");
}