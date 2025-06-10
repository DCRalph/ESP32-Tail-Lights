/****************************************************
 * mainMenu.h
 *
 * Declares the "mainMenu" object and the
 * functions that implement its printMenu and
 * handleInput logic.
 ****************************************************/

#pragma once
#include "SerialMenu.h" // needed for struct Menu

// Declare the mainMenu instance
extern Menu mainMenu;

// Declare function prototypes for the main menu
void printMainMenu(const Menu &menu);
bool handleMainMenuInput(Menu &menu, const String &input);

