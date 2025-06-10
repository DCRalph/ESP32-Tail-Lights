/****************************************************
 * systemMenu.cpp
 *
 * Defines the systemMenu object and implements its
 * printMenu/handleInput.
 ****************************************************/
#include "systemMenu.h"

// If you need references to mainMenu for going back:
#include "mainMenu.h"

// If you want to do setMenu, processMenuInput, etc.
#include "SerialMenu.h"

#include <WiFi.h> // If you're using WiFi.localIP, etc.

Menu systemMenu = {
    F("System"),
    &mainMenu, // parent is mainMenu
    printSystemMenu,
    handleSystemMenuInput};

void printSystemMenu(const Menu &menu)
{
    Serial.println(String(F("\nBreadcrumb: ")) + buildBreadcrumbString(&menu));
    Serial.println(F("=== SYSTEM MENU ==="));
    Serial.println(F("1) Get ID"));
    Serial.println(F("2) Get Version"));
    Serial.println(F("3) Get IP"));
    Serial.println(F("4) Get MAC"));
    Serial.println(F("5) Sysinfo"));
    Serial.println(F("b) Back"));
    Serial.println(F("Press Enter to re-print this menu"));
}

bool handleSystemMenuInput(Menu &menu, const String &input)
{
    if (input == F("1"))
    {
        // example: getDeviceId()
        // (where getDeviceId() is from your code)
        Serial.println(1);
        return true;
    }
    else if (input == F("2"))
    {
        // e.g. updater.currentVersion
        Serial.println(2);
        return true;
    }
    else if (input == F("3"))
    {
        Serial.println(WiFi.localIP());
        return true;
    }
    else if (input == F("4"))
    {
        Serial.println(WiFi.macAddress());
        return true;
    }
    else if (input == F("5"))
    {
        // Sysinfo example
        Serial.println(F("System Info:"));
        Serial.println(String(F("Device ID: ")) + "1");
        Serial.println(String(F("IP Address: ")) + WiFi.localIP().toString());
        Serial.println(String(F("MAC Address: ")) + WiFi.macAddress());

        // show memory usage
        uint32_t usedHeap = ESP.getHeapSize() - ESP.getFreeHeap();
        uint32_t usedPsram = ESP.getPsramSize() - ESP.getFreePsram();

        Serial.println(String(F("Heap: ")) +
                       formatBytes(usedHeap) + String(F(" / ")) + formatBytes(ESP.getHeapSize()));
        Serial.println(String(F("PSRAM: ")) +
                       formatBytes(usedPsram) + String(F(" / ")) + formatBytes(ESP.getPsramSize()));
        return true;
    }
    else if (input == F("b"))
    {
        // go back to main menu
        extern Menu mainMenu;
        setMenu(&mainMenu);
        return true;
    }

    // Not recognized
    return false;
}
