/****************************************************
 * hvInputMenu.cpp
 *
 * Implementation of the HV input status menu system.
 * This menu displays the status of all HV inputs in
 * both boolean and voltage format.
 ****************************************************/

#include "hvInputMenu.h"
#include "mainMenu.h"
#include "SerialMenu.h"
#include "config.h"
#include "Application.h"
#include <Arduino.h>

/****************************************************
 * hvInputMenu object
 ****************************************************/
SerialMenu hvInputMenu = {
    "HV Input Status",
    &mainMenu,
    printHVInputMenu,
    handleHVInputMenuInput};

/****************************************************
 * printHVInputMenu()
 ****************************************************/
void printHVInputMenu(const SerialMenu &menu)
{
  Serial.println(String(F("\nBreadcrumb: ")) + buildBreadcrumbString(&menu));
  Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
  Serial.println(F("║                    HV INPUT STATUS                          ║"));
  Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
  Serial.println();

  Application *app = Application::getInstance();
  if (!app)
  {
    Serial.println(F("Application not initialized - cannot read input status"));
    Serial.println(F("b) Back to Main Menu"));
    Serial.println(F("Press Enter to re-print this menu"));
    return;
  }

  // Get current status of all HV inputs
  Serial.println(F("Current HV Input Status:"));
  Serial.println(F("====================================="));

  Serial.println(F("Input 1 (ACC):"));
  Serial.print(F("  Digital State: "));
  Serial.println(app->accOnInput.get() ? "HIGH" : "LOW");
  Serial.print(F("  Voltage: "));
  Serial.print(app->accOnInput.getVoltage(), 2);
  Serial.println(F("V"));
  Serial.print(F("  Enabled: "));
  Serial.println(app->accOnInput.isEnabled() ? "YES" : "NO");
  Serial.println(F("  Override: "));
  Serial.println(app->accOnInput.isOverride() ? "YES" : "NO");
  Serial.println();

  Serial.println(F("Input 2 (Left Indicator):"));
  Serial.print(F("  Digital State: "));
  Serial.println(app->leftIndicatorInput.get() ? "HIGH" : "LOW");
  Serial.print(F("  Voltage: "));
  Serial.print(app->leftIndicatorInput.getVoltage(), 2);
  Serial.println(F("V"));
  Serial.print(F("  Enabled: "));
  Serial.println(app->leftIndicatorInput.isEnabled() ? "YES" : "NO");
  Serial.println(F("  Override: "));
  Serial.println(app->leftIndicatorInput.isOverride() ? "YES" : "NO");
  Serial.println();

  Serial.println(F("Input 3 (Right Indicator):"));
  Serial.print(F("  Digital State: "));
  Serial.println(app->rightIndicatorInput.get() ? "HIGH" : "LOW");
  Serial.print(F("  Voltage: "));
  Serial.print(app->rightIndicatorInput.getVoltage(), 2);
  Serial.println(F("V"));
  Serial.print(F("  Enabled: "));
  Serial.println(app->rightIndicatorInput.isEnabled() ? "YES" : "NO");
  Serial.println(F("  Override: "));
  Serial.println(app->rightIndicatorInput.isOverride() ? "YES" : "NO");
  Serial.println();

  Serial.println(F("Input 4 (Headlight):"));
  Serial.print(F("  Digital State: "));
  Serial.println(app->headlightInput.get() ? "HIGH" : "LOW");
  Serial.print(F("  Voltage: "));
  Serial.print(app->headlightInput.getVoltage(), 2);
  Serial.println(F("V"));
  Serial.print(F("  Enabled: "));
  Serial.println(app->headlightInput.isEnabled() ? "YES" : "NO");
  Serial.println(F("  Override: "));
  Serial.println(app->headlightInput.isOverride() ? "YES" : "NO");
  Serial.println();

  Serial.println(F("Input 5 (Brake):"));
  Serial.print(F("  Digital State: "));
  Serial.println(app->brakeInput.get() ? "HIGH" : "LOW");
  Serial.print(F("  Voltage: "));
  Serial.print(app->brakeInput.getVoltage(), 2);
  Serial.println(F("V"));
  Serial.print(F("  Enabled: "));
  Serial.println(app->brakeInput.isEnabled() ? "YES" : "NO");
  Serial.println(F("  Override: "));
  Serial.println(app->brakeInput.isOverride() ? "YES" : "NO");
  Serial.println();

  Serial.println(F("Input 6 (Reverse):"));
  Serial.print(F("  Digital State: "));
  Serial.println(app->reverseInput.get() ? "HIGH" : "LOW");
  Serial.print(F("  Voltage: "));
  Serial.print(app->reverseInput.getVoltage(), 2);
  Serial.println(F("V"));
  Serial.print(F("  Enabled: "));
  Serial.println(app->reverseInput.isEnabled() ? "YES" : "NO");
  Serial.println(F("  Override: "));
  Serial.println(app->reverseInput.isOverride() ? "YES" : "NO");
  Serial.println();

  Serial.println(F("Commands:"));
  Serial.println(F("r) Refresh status"));
  Serial.println(F("m) Monitor mode (continuous refresh)"));
  Serial.println(F("s) Stop monitor mode"));
  Serial.println(F("b) Back to Main Menu"));
  Serial.println(F("Press Enter to re-print this menu"));
}

/****************************************************
 * handleHVInputMenuInput()
 ****************************************************/
bool handleHVInputMenuInput(SerialMenu &menu, const String &input)
{
  if (input == F("r"))
  {
    // Refresh - just reprint the menu
    printHVInputMenu(menu);
    return true;
  }
  else if (input == F("m"))
  {
    // Monitor mode - continuously print status
    Serial.println(F("Entering monitor mode... Press 's' and Enter to stop"));
    Serial.println(F("Refreshing every 1 second:"));
    Serial.println();

    // Simple monitor loop
    unsigned long lastUpdate = 0;
    while (true)
    {
      // Check for stop command
      if (Serial.available())
      {
        String stopInput = Serial.readString();
        stopInput.trim();
        if (stopInput == F("s"))
        {
          Serial.println(F("Monitor mode stopped."));
          printHVInputMenu(menu);
          break;
        }
      }

      // Update display every 1000ms
      if (millis() - lastUpdate >= 1000)
      {
        lastUpdate = millis();

        Application *monitorApp = Application::getInstance();
        if (!monitorApp)
        {
          Serial.println(F("Application not available"));
          break;
        }

        Serial.println(F("HV Input Status Update:"));
        Serial.println("Time: " + String(millis() / 1000) + "s");


        Serial.print(F("ACC: "));
        Serial.print(monitorApp->accOnInput.get() ? F("HIGH") : F("LOW"));
        Serial.print(F(" ("));
        Serial.print(monitorApp->accOnInput.getVoltage(), 2);
        Serial.print(F("V) "));

        Serial.print(F("L.IND: "));
        Serial.print(monitorApp->leftIndicatorInput.get() ? F("HIGH") : F("LOW"));
        Serial.print(F(" ("));
        Serial.print(monitorApp->leftIndicatorInput.getVoltage(), 2);
        Serial.print(F("V) "));

        Serial.print(F("R.IND: "));
        Serial.print(monitorApp->rightIndicatorInput.get() ? F("HIGH") : F("LOW"));
        Serial.print(F(" ("));
        Serial.print(monitorApp->rightIndicatorInput.getVoltage(), 2);
        Serial.println(F("V)"));

        Serial.print(F("HEAD: "));
        Serial.print(monitorApp->headlightInput.get() ? F("HIGH") : F("LOW"));
        Serial.print(F(" ("));
        Serial.print(monitorApp->headlightInput.getVoltage(), 2);
        Serial.print(F("V) "));

        Serial.print(F("BRAKE: "));
        Serial.print(monitorApp->brakeInput.get() ? F("HIGH") : F("LOW"));
        Serial.print(F(" ("));
        Serial.print(monitorApp->brakeInput.getVoltage(), 2);
        Serial.print(F("V) "));

        Serial.print(F("REV: "));
        Serial.print(monitorApp->reverseInput.get() ? F("HIGH") : F("LOW"));
        Serial.print(F(" ("));
        Serial.print(monitorApp->reverseInput.getVoltage(), 2);
        Serial.println(F("V)"));
        Serial.println(F("--------------------------------"));
      }

      delay(50); // Small delay to prevent watchdog issues
    }
    return true;
  }
  else if (input == F("s"))
  {
    // Stop command - handled in monitor mode
    Serial.println(F("Not in monitor mode"));
    return true;
  }

  // If not recognized, return false so parent menu can handle "b" command
  return false;
}