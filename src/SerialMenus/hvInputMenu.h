/****************************************************
 * hvInputMenu.h
 *
 * Declares the hvInputMenu object and its function
 * pointers for printing/handling input.
 ****************************************************/
#pragma once

#include "SerialMenu.h"

extern SerialMenu hvInputMenu;

void printHVInputMenu(const SerialMenu &menu);
bool handleHVInputMenuInput(SerialMenu &menu, const String &input);