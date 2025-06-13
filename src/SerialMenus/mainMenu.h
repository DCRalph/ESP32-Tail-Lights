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
extern SerialMenu mainMenu;

// Declare function prototypes for the main menu
void printMainMenu(const SerialMenu &menu);
bool handleMainMenuInput(SerialMenu &menu, const String &input);

