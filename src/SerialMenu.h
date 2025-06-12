/****************************************************
 * SerialMenu.h
 *
 * Declares the primary Menu struct (with name, parent,
 * function pointers) and the core functions for
 * managing the active menu.
 ****************************************************/

#pragma once
#include <Arduino.h>
#include <functional>
#include "config.h"

/****************************************************
 * Menu struct
 *
 * - name: The displayed name of the menu
 * - parent: The parent menu (used for breadcrumbs)
 * - printMenu: pointer to a function that prints the menu
 * - handleInput: pointer to a function that processes
 *                user input. Returns `true` if the
 *                command is recognized, else `false`.
 ****************************************************/
struct Menu
{
    String name;
    Menu *parent;
    void (*printMenu)(const Menu &menu);
    bool (*handleInput)(Menu &menu, const String &input);
};

/****************************************************
 * initSerialMenu()
 * - Initialises the menu system (e.g. sets current menu
 *   to mainMenu). Call it once in setup, or from your
 *   serialTask, etc.
 ****************************************************/
void initSerialMenu();

/****************************************************
 * setMenu()
 * - Changes the currently active menu and immediately
 *   calls printMenu() for the new menu.
 ****************************************************/
void setMenu(Menu *newMenu);

/****************************************************
 * getMenu()
 * - Returns the pointer to the currently active menu.
 ****************************************************/
Menu *getMenu();

/****************************************************
 * promptUserInput()
 * - Reusable function that prompts user for input with ">"
 * - Returns the complete input string when Enter is pressed
 * - Handles character echo and backspace
 * - Times out after specified milliseconds (0 = no timeout)
 ****************************************************/
String promptUserInput(const String &prompt = "", unsigned long timeoutMs = 0);

/****************************************************
 * processMenuInput()
 * - If `input` is empty, reprint the current menu.
 * - Checks "global" or "special" commands by calling
 *   handleSpecialCommand().
 * - If not handled, calls currentMenu->handleInput().
 ****************************************************/
void processMenuInput(const String &input);

/****************************************************
 * handleSpecialCommand()
 * - Processes commands that work from any menu,
 *   e.g., "gohome". Returns true if handled, false
 *   otherwise.
 ****************************************************/
bool handleSpecialCommand(const String &input);

/****************************************************
 * buildBreadcrumbString()
 * - Recursively builds a string for the breadcrumb
 *   like "Main > System" or "Main".
 ****************************************************/
String buildBreadcrumbString(const Menu *menu);
