/****************************************************
 * syncMenu.h
 *
 * Declares the syncMenu object and its associated
 * functions for managing the SyncManager through
 * the serial menu system.
 ****************************************************/

#pragma once
#include "SerialMenu.h"

// Declare the syncMenu object
extern SerialMenu syncMenu;

// Function prototypes
void printSyncMenu(const SerialMenu &menu);
bool handleSyncMenuInput(SerialMenu &menu, const String &input);

// Helper functions for sync management
void showSyncStatus();
void showAvailableGroups();
void createNewGroup();
void joinGroupByNumber(int groupNumber);
void listKnownDevices();
void setSyncMode();