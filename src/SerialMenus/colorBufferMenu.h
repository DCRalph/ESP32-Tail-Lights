/****************************************************
 * colorBufferMenu.h
 *
 * Declares the "colorBufferMenu" object and the
 * functions that implement its printMenu and
 * handleInput logic for displaying LED color buffer values.
 ****************************************************/

#pragma once
#include "SerialMenu.h" // needed for struct SerialMenu

// Declare the colorBufferMenu instance
extern SerialMenu colorBufferMenu;

// Declare function prototypes for the color buffer menu
void printColorBufferMenu(const SerialMenu &menu);
bool handleColorBufferMenuInput(SerialMenu &menu, const String &input);

// Helper functions for displaying color buffer data
void printStatusLEDBuffers();
void printLEDStripBuffers();
void printSpecificLEDStripBuffer(const String &stripName);
void printColorValue(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);