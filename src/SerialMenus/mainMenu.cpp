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
    else if (input == F("r"))
    {
        Serial.println(F("Rebooting..."));
        restart();
        return true;
    }

    // If not recognized
    return false;
}
