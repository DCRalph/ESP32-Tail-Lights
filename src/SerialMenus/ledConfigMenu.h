/****************************************************
 * ledConfigMenu.h
 *
 * Declares the ledConfigMenu object and its
 * functions for managing LED strip configuration
 * through the serial menu system.
 ****************************************************/

#pragma once
#include "SerialMenu.h"

// Declare the ledConfigMenu object
extern SerialMenu ledConfigMenu;

// Function prototypes
void printLEDConfigMenu(const SerialMenu &menu);
bool handleLEDConfigMenuInput(SerialMenu &menu, const String &input);

// Helper functions for LED configuration
void showLEDConfig();
void configureStripLength(const String &stripName, uint16_t *lengthPtr);
// void configureStripPin(const String &stripName, uint8_t *pinPtr);
void toggleStripEnabled(const String &stripName, bool *enabledPtr);
void toggleStripFlipped(const String &stripName, bool *flippedPtr);