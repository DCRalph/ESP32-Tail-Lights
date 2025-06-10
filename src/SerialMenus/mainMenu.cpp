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
#include "syncMenu.h"

// If you need references to other menus:

// If you want to do a direct setMenu() call
// or other global calls, include "SerialMenu.h"
#include "SerialMenu.h"

#include <Arduino.h> // for Serial, String, etc.

/****************************************************
 * mainMenu object
 * - name: "Main"
 * - parent: nullptr (this is the root menu)
 * - printMenu: printMainMenu
 * - handleInput: handleMainMenuInput
 ****************************************************/
Menu mainMenu = {
    "Main",
    nullptr,
    printMainMenu,
    handleMainMenuInput};

/****************************************************
 * printMainMenu()
 ****************************************************/
void printMainMenu(const Menu &menu)
{
    String breadcrumb = buildBreadcrumbString(&menu);
    Serial.println(String(F("\nBreadcrumb: ")) + breadcrumb);
    Serial.println(F("=== MAIN MENU ==="));
    Serial.println(F("1) System Menu"));
    Serial.println(F("2) Sync Manager"));
    Serial.println(F("r) Reboot"));
    Serial.println(F("Press Enter to re-print this menu"));
}

/****************************************************
 * handleMainMenuInput()
 * - Returns true if a command is recognized
 * - Otherwise returns false
 ****************************************************/
bool handleMainMenuInput(Menu &menu, const String &input)
{
    if (input == F("1"))
    {
        // Switch to system menu
        extern Menu systemMenu; // or #include "systemMenu.h"
        setMenu(&systemMenu);
        return true;
    }
    else if (input == F("2"))
    {
        // Switch to sync manager menu
        setMenu(&syncMenu);
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
