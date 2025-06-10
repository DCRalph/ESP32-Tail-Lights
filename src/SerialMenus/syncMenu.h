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
extern Menu syncMenu;

// Function prototypes
void printSyncMenu(const Menu &menu);
bool handleSyncMenuInput(Menu &menu, const String &input);

// Helper functions for sync management
void showSyncStatus();
void showAvailableGroups();
void createNewGroup();
void joinGroupByNumber(int groupNumber);
void listKnownDevices();