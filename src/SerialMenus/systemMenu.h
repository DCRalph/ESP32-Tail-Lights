/****************************************************
 * systemMenu.h
 *
 * Declares the systemMenu object and its function
 * pointers for printing/handling input.
 ****************************************************/
#pragma once

#include "SerialMenu.h"

extern Menu systemMenu;

void printSystemMenu(const Menu &menu);
bool handleSystemMenuInput(Menu &menu, const String &input);

