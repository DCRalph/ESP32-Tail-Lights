/****************************************************
 * systemMenu.h
 *
 * Declares the systemMenu object and its function
 * pointers for printing/handling input.
 ****************************************************/
#pragma once

#include "SerialMenu.h"

extern SerialMenu systemMenu;

void printSystemMenu(const SerialMenu &menu);
bool handleSystemMenuInput(SerialMenu &menu, const String &input);

