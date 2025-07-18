/****************************************************LOW
 * mainMenu.cpp
 *
 * Defines the mainMenu object, providing the
 * function pointers to printMainMenu and
 * handleMainMenuInput.
 *
 * Contains the actual code for those two functions.
 ****************************************************/

#include "mainMenu.h"

#include "SerialMenu.h"
#include <Arduino.h> // for Serial, String, etc.

#include "systemMenu.h"
#include "syncMenu.h"
#include "provisioningMenu.h"
#include "ledConfigMenu.h"
#include "hvInputMenu.h"
#include "colorBufferMenu.h"
#include "IO/TimeProfiler.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/****************************************************
 * mainMenu object
 * - name: "Main"
 * - parent: nullptr (this is the root menu)
 * - printMenu: printMainMenu
 * - handleInput: handleMainMenuInput
 ****************************************************/
SerialMenu mainMenu = {
    "Main",
    nullptr,
    printMainMenu,
    handleMainMenuInput};

void printMainMenu(const SerialMenu &menu)
{
    String breadcrumb = buildBreadcrumbString(&menu);
    Serial.println(String(F("\nBreadcrumb: ")) + breadcrumb);
    Serial.println(F("=== MAIN MENU ==="));
    Serial.println(F("1) System Menu"));
    Serial.println(F("2) Sync Manager"));
    Serial.println(F("3) Device Provisioning"));
    Serial.println(F("4) LED Configuration"));
    Serial.println(F("5) HV Input Status"));
    Serial.println(F("6) Time Profiler"));
    Serial.println(F("7) Color Buffer Debug"));
    Serial.println(F("r) Reboot"));
    Serial.println(F("Press Enter to re-print this menu"));
}

bool handleMainMenuInput(SerialMenu &menu, const String &input)
{
    if (input == F("1"))
    {
        setMenu(&systemMenu);
        return true;
    }
    else if (input == F("2"))
    {
        setMenu(&syncMenu);
        return true;
    }
    else if (input == F("3"))
    {
        setMenu(&provisioningMenu);
        return true;
    }
    else if (input == F("4"))
    {
        setMenu(&ledConfigMenu);
        return true;
    }
    else if (input == F("5"))
    {
        setMenu(&hvInputMenu);
        return true;
    }
    else if (input == F("6"))
    {
        timeProfiler.printAll();
        return true;
    }
    else if (input == F("7"))
    {
        setMenu(&colorBufferMenu);
        return true;
    }

    else if (input == F("r"))
    {
        Serial.println(F("Rebooting..."));
        restart();
        return true;
    }

    // If not recognized
    return false;
}
