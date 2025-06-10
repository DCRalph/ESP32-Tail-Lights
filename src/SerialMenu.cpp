/****************************************************
 * SerialMenu.cpp
 *
 * Defines:
 *   - The global pointer to the current menu.
 *   - initSerialMenu(), setMenu(), getMenu(),
 *     processMenuInput(), handleSpecialCommand(),
 *     buildBreadcrumbString().
 *
 * It does NOT define the actual menu instances.
 * Those are defined in separate files (mainMenu.cpp,
 * systemMenu.cpp, etc.).
 ****************************************************/

#include "SerialMenu.h"

#include <Arduino.h> // for String, etc.

// If you have references to other includes for
// globally-known functions like "vTaskDelay" or
// "Serial", do so here.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "SerialMenus/mainMenu.h"

//------------------------------------------
// Global pointer to the currently active menu
//------------------------------------------
static Menu *currentMenu = nullptr;

/****************************************************
 * initSerialMenu()
 * - Optionally set the currentMenu to mainMenu
 *   (but we can't do it here unless we can
 *   reference the mainMenu object, which might be
 *   in another file. If so, just call setMenu()
//   externally after everything is linked).
 ****************************************************/
void initSerialMenu()
{
    setMenu(&mainMenu);
}

/****************************************************
 * setMenu()
 ****************************************************/
void setMenu(Menu *newMenu)
{
    if (newMenu != nullptr)
    {
        currentMenu = newMenu;
        if (currentMenu->printMenu)
        {
            currentMenu->printMenu(*currentMenu);
        }
    }
}

/****************************************************
 * getMenu()
 ****************************************************/
Menu *getMenu()
{
    return currentMenu;
}

/****************************************************
 * processMenuInput()
 * - re-print menu if input is empty
 * - check handleSpecialCommand() first
 * - if not handled, call currentMenu->handleInput()
 ****************************************************/
void processMenuInput(const String &input)
{

    // 1) If empty input, reprint the current menu
    if (input.length() == 0)
    {
        if (currentMenu && currentMenu->printMenu)
        {
            currentMenu->printMenu(*currentMenu);
        }
        return;
    }

    // 2) Check special commands
    if (handleSpecialCommand(input))
    {
        return;
    }

    // 3) Otherwise, let the active menu handle it
    if (currentMenu && currentMenu->handleInput)
    {
        bool recognized = currentMenu->handleInput(*currentMenu, input);
        if (!recognized)
        {
            // Not recognized => let user know
            Serial.println("Unknown command.");
            currentMenu->printMenu(*currentMenu);
        }
    }
}

/****************************************************
 * handleSpecialCommand()
 * - Implement your global commands here, e.g.,
 *   "caltouch", "initml", etc. Return true if handled,
 *   else false.
 ****************************************************/
bool handleSpecialCommand(const String &input)
{
    // check if ipput starts with provision:

    // If not recognized
    return false;
}

/****************************************************
 * buildBreadcrumbString()
 * - Builds "Parent > Current" recursively
 ****************************************************/
String buildBreadcrumbString(const Menu *menu)
{
    if (!menu)
        return "";
    if (menu->parent == nullptr)
    {
        // This is the root
        return menu->name;
    }
    else
    {
        return buildBreadcrumbString(menu->parent) + " > " + menu->name;
    }
}
