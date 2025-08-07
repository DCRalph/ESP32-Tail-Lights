/****************************************************
 * ledConfigMenu.cpp
 *
 * Implementation of the LED configuration menu system.
 * This menu allows configuration of LED strip parameters
 * including lengths, pins, enable states, and flip settings.
 ****************************************************/

#include "ledConfigMenu.h"
#include "mainMenu.h"
#include "SerialMenu.h"
#include "config.h"
#include <Arduino.h>

/****************************************************
 * ledConfigMenu object
 ****************************************************/
SerialMenu ledConfigMenu = {
    "LED Configuration",
    &mainMenu,
    printLEDConfigMenu,
    handleLEDConfigMenuInput};

/****************************************************
 * printLEDConfigMenu()
 ****************************************************/
void printLEDConfigMenu(const SerialMenu &menu)
{
  Serial.println(String(F("\nBreadcrumb: ")) + buildBreadcrumbString(&menu));
  Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
  Serial.println(F("║                    LED STRIP CONFIGURATION                  ║"));
  Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
  Serial.println();

  // Show current LED configuration
  ledConfig.print();

  Serial.println(F("=== LED CONFIGURATION MENU ==="));
  Serial.println(F("1) Show Current Configuration"));
  Serial.println(F("2) Set Headlight LED Count"));
  Serial.println(F("3) Toggle Headlight Enable/Disable"));
  Serial.println(F("4) Toggle Headlight Flip"));
  Serial.println(F("5) Set Taillight LED Count"));
  Serial.println(F("6) Toggle Taillight Enable/Disable"));
  Serial.println(F("7) Toggle Taillight Flip"));
  Serial.println(F("8) Set Underglow LED Count"));
  Serial.println(F("9) Toggle Underglow Enable/Disable"));
  Serial.println(F("10) Toggle Underglow Flip"));
  Serial.println(F("11) Set Interior LED Count"));
  Serial.println(F("12) Toggle Interior Enable/Disable"));
  Serial.println(F("13) Toggle Interior Flip"));
  Serial.println(F("14) Set All Strip Lengths"));
  Serial.println(F("15) Enable/Disable All Strips"));
  Serial.println(F("16) Reset to Defaults"));
  Serial.println(F("17) Save Configuration"));
  Serial.println(F("s) Show Configuration"));
  Serial.println(F("b) Back to Main Menu"));
  Serial.println(F("Press Enter to re-print this menu"));
  Serial.println();
}

/****************************************************
 * handleLEDConfigMenuInput()
 ****************************************************/
bool handleLEDConfigMenuInput(SerialMenu &menu, const String &input)
{
  if (input == F("1") || input == F("s"))
  {
    showLEDConfig();
    return true;
  }
  else if (input == F("2"))
  {
    // Set Headlight LED Count
    configureStripLength("Headlight", &ledConfig.headlightLedCount);
    return true;
  }
  else if (input == F("3"))
  {
    // Toggle Headlight Enable/Disable
    toggleStripEnabled("Headlight", &ledConfig.headlightsEnabled);
    return true;
  }
  else if (input == F("4"))
  {
    // Toggle Headlight Flip
    toggleStripFlipped("Headlight", &ledConfig.headlightFlipped);
    return true;
  }
  else if (input == F("5"))
  {
    // Set Taillight LED Count
    configureStripLength("Taillight", &ledConfig.taillightLedCount);
    return true;
  }
  else if (input == F("6"))
  {
    // Toggle Taillight Enable/Disable
    toggleStripEnabled("Taillight", &ledConfig.taillightsEnabled);
    return true;
  }
  else if (input == F("7"))
  {
    // Toggle Taillight Flip
    toggleStripFlipped("Taillight", &ledConfig.taillightFlipped);
    return true;
  }
  else if (input == F("8"))
  {
    // Set Underglow LED Count
    configureStripLength("Underglow", &ledConfig.underglowLedCount);
    return true;
  }
  else if (input == F("9"))
  {
    // Toggle Underglow Enable/Disable
    toggleStripEnabled("Underglow", &ledConfig.underglowEnabled);
    return true;
  }
  else if (input == F("10"))
  {
    // Toggle Underglow Flip
    toggleStripFlipped("Underglow", &ledConfig.underglowFlipped);
    return true;
  }
  else if (input == F("11"))
  {
    // Set Interior LED Count
    configureStripLength("Interior", &ledConfig.interiorLedCount);
    return true;
  }
  else if (input == F("12"))
  {
    // Toggle Interior Enable/Disable
    toggleStripEnabled("Interior", &ledConfig.interiorEnabled);
    return true;
  }
  else if (input == F("13"))
  {
    // Toggle Interior Flip
    toggleStripFlipped("Interior", &ledConfig.interiorFlipped);
    return true;
  }
  else if (input == F("14"))
  {
    // Set all strip lengths
    String lengthStr = promptUserInput(F("Enter LED count for all strips (1-1000):"), 15000);
    if (lengthStr.length() > 0)
    {
      uint16_t length = lengthStr.toInt();
      if (length > 0 && length <= 1000)
      {
        ledConfig.headlightLedCount = length;
        ledConfig.taillightLedCount = length;
        ledConfig.underglowLedCount = length;
        ledConfig.interiorLedCount = length;
        Serial.println("All strip lengths set to " + String(length));
        saveLEDConfig();
      }
      else
      {
        Serial.println(F("Invalid length. Must be between 1 and 1000."));
      }
    }
    return true;
  }
  else if (input == F("15"))
  {
    // Enable/Disable all strips
    Serial.println(F("1) Enable All Strips"));
    Serial.println(F("2) Disable All Strips"));
    String choice = promptUserInput(F("Enter choice:"), 10000);

    if (choice == F("1"))
    {
      ledConfig.headlightsEnabled = true;
      ledConfig.taillightsEnabled = true;
      ledConfig.underglowEnabled = true;
      ledConfig.interiorEnabled = true;
      Serial.println(F("All strips enabled"));
      saveLEDConfig();
    }
    else if (choice == F("2"))
    {
      ledConfig.headlightsEnabled = false;
      ledConfig.taillightsEnabled = false;
      ledConfig.underglowEnabled = false;
      ledConfig.interiorEnabled = false;
      Serial.println(F("All strips disabled"));
      saveLEDConfig();
    }
    return true;
  }
  else if (input == F("16"))
  {
    // Reset to defaults
    Serial.println(F("WARNING: This will reset all LED configuration to defaults!"));
    String confirm = promptUserInput(F("Type 'RESET' to confirm:"), 15000);

    if (confirm == F("RESET"))
    {
      ledConfig = LEDConfig(); // Reset to default values
      saveLEDConfig();
      Serial.println(F("LED configuration reset to defaults"));
    }
    else
    {
      Serial.println(F("Reset cancelled"));
    }
    return true;
  }
  else if (input == F("17"))
  {
    saveLEDConfig();
    Serial.println(F("LED configuration saved! Restarting..."));
    ESP.restart();
    return true;
  }
  else if (input == F("b"))
  {
    setMenu(&mainMenu);
    return true;
  }

  return false;
}

/****************************************************
 * Helper functions
 ****************************************************/
void showLEDConfig()
{
  Serial.println();
  ledConfig.print();
}

void configureStripLength(const String &stripName, uint16_t *lengthPtr)
{
  String lengthStr = promptUserInput("Enter LED count for " + stripName + " (1-1000):", 15000);

  if (lengthStr.length() > 0)
  {
    uint16_t length = lengthStr.toInt();
    if (length > 0 && length <= 1000)
    {
      *lengthPtr = length;
      Serial.println(stripName + F(" LED count set to ") + String(length));
      saveLEDConfig();
    }
    else
    {
      Serial.println(F("Invalid length. Must be between 1 and 1000."));
    }
  }
}

void configureStripPin(const String &stripName, uint8_t *pinPtr)
{
  String pinStr = promptUserInput("Enter pin number for " + stripName + " (0-50):", 15000);

  if (pinStr.length() > 0)
  {
    uint8_t pin = pinStr.toInt();
    if (pin <= 50)
    {
      *pinPtr = pin;
      Serial.println(stripName + F(" pin set to ") + String(pin));
      saveLEDConfig();
    }
    else
    {
      Serial.println(F("Invalid pin. Must be between 0 and 50."));
    }
  }
}

void toggleStripEnabled(const String &stripName, bool *enabledPtr)
{
  *enabledPtr = !(*enabledPtr);
  Serial.println(stripName + F(" ") + (*enabledPtr ? F("ENABLED") : F("DISABLED")));
  saveLEDConfig();
}

void toggleStripFlipped(const String &stripName, bool *flippedPtr)
{
  *flippedPtr = !(*flippedPtr);
  Serial.println(stripName + F(" flip setting: ") + (*flippedPtr ? F("FLIPPED") : F("NORMAL")));
  saveLEDConfig();
}