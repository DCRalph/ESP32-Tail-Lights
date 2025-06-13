/****************************************************
 * SerialMenu.cpp
 ****************************************************/

#include "SerialMenu.h"

#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "SerialMenus/mainMenu.h"
#include "SerialMenus/provisioningMenu.h"
#include "config.h"

//------------------------------------------
// Global pointer to the currently active menu
//------------------------------------------
static SerialMenu *currentMenu = nullptr;

/****************************************************
 * initSerialMenu()
 * - Check if device is provisioned and set appropriate menu
 ****************************************************/
void initSerialMenu()
{
    if (isDeviceProvisioned())
    {
        setMenu(&mainMenu);
    }
    else
    {
        Serial.println();
        Serial.println(F("╔══════════════════════════════════════════════════════════════╗"));
        Serial.println(F("║                    DEVICE NOT PROVISIONED                   ║"));
        Serial.println(F("╠══════════════════════════════════════════════════════════════╣"));
        Serial.println(F("║  This device must be provisioned before normal operation.   ║"));
        Serial.println(F("║  Entering provisioning mode...                              ║"));
        Serial.println(F("╚══════════════════════════════════════════════════════════════╝"));
        Serial.println();
        startProvisioning();
        setMenu(&provisioningMenu);
    }
}

/****************************************************
 * setMenu()
 ****************************************************/
void setMenu(SerialMenu *newMenu)
{
    if (newMenu != nullptr)
    {
        currentMenu = newMenu;
        if (currentMenu->printMenu)
        {
            currentMenu->printMenu(*currentMenu);
        }
    }
}

/****************************************************
 * getMenu()
 ****************************************************/
SerialMenu *getMenu()
{
    return currentMenu;
}

/****************************************************
 * promptUserInput()
 * - Reusable function that prompts user for input with ">"
 * - Returns the complete input string when Enter is pressed
 * - Handles character echo and backspace
 * - Times out after specified milliseconds (0 = no timeout)
 ****************************************************/
String promptUserInput(const String &prompt, unsigned long timeoutMs)
{
    if (prompt.length() > 0)
    {
        Serial.println(prompt);
    }
    Serial.print("> ");

    String inputBuffer = "";
    unsigned long startTime = millis();

    while (true)
    {
        // Check timeout if specified
        if (timeoutMs > 0 && (millis() - startTime) > timeoutMs)
        {
            Serial.println();
            Serial.println(F("Input timeout."));
            return "";
        }

        if (Serial.available())
        {
            char c = Serial.read();

            // Handle Enter key (CR or LF)
            if (c == '\r' || c == '\n')
            {
                Serial.println(); // Add newline after Enter
                inputBuffer.trim();
                return inputBuffer;
            }
            // Handle Backspace (ASCII 8 or 127)
            else if (c == 8 || c == 127)
            {
                if (inputBuffer.length() > 0)
                {
                    inputBuffer.remove(inputBuffer.length() - 1);
                    // Echo backspace to terminal (backspace, space, backspace)
                    Serial.print("\b \b");
                }
            }
            // Handle printable characters
            else if (c >= 32 && c <= 126)
            {
                inputBuffer += c;
                Serial.print(c); // Echo character back to user
            }
            // Ignore other control characters
        }

        delay(10); // Small delay to prevent excessive CPU usage
    }
}

/****************************************************
 * processMenuInput()
 * - builds up command string character by character
 * - processes complete command when Enter is received
 ****************************************************/
void processMenuInput(const String &input)
{
    static String commandBuffer = "";

    // Process each character in the input
    for (int i = 0; i < input.length(); i++)
    {
        char c = input.charAt(i);

        // Handle Enter key (CR or LF)
        if (c == '\r' || c == '\n')
        {
            // Process the complete command
            if (commandBuffer.length() == 0)
            {
                // Empty command - reprint menu
                if (currentMenu && currentMenu->printMenu)
                {
                    Serial.println(); // Add newline after Enter
                    currentMenu->printMenu(*currentMenu);
                }
            }
            else
            {
                Serial.println(); // Add newline after Enter

                // Check special commands first
                if (handleSpecialCommand(commandBuffer))
                {
                    // Special command handled
                }
                else if (currentMenu && currentMenu->handleInput)
                {
                    // Let the active menu handle it
                    bool recognized = currentMenu->handleInput(*currentMenu, commandBuffer);
                    if (!recognized)
                    {
                        Serial.println("Unknown command.");
                        currentMenu->printMenu(*currentMenu);
                    }
                }
            }

            // Clear the command buffer for next command
            commandBuffer = "";
            Serial.print("> "); // Show prompt for next command
        }
        // Handle Backspace (ASCII 8 or 127)
        else if (c == 8 || c == 127)
        {
            if (commandBuffer.length() > 0)
            {
                commandBuffer.remove(commandBuffer.length() - 1);
                // Echo backspace to terminal (backspace, space, backspace)
                Serial.print("\b \b");
            }
        }
        // Handle printable characters
        else if (c >= 32 && c <= 126)
        {
            commandBuffer += c;
            Serial.print(c); // Echo character back to user
        }
        // Ignore other control characters
    }
}

/****************************************************
 * handleSpecialCommand()
 * - Implement your global commands here, e.g.,
 *   "caltouch", "initml", etc. Return true if handled,
 *   else false.
 ****************************************************/
bool handleSpecialCommand(const String &input)
{
    // Emergency provisioning bypass (for debugging only)
    if (input == F("provision:bypass"))
    {
        Serial.println(F("[WARNING] Bypassing provisioning check - DEBUG MODE ONLY"));
        deviceInfo.provisioned = true;
        saveDeviceInfo();
        Serial.println(F("Device marked as provisioned. Restarting..."));
        delay(1000);
        restart();
        return true;
    }

    // Force provisioning mode (for re-configuration)
    if (input == F("provision:force"))
    {
        Serial.println(F("[INFO] Forcing provisioning mode"));
        setMenu(&provisioningMenu);
        return true;
    }

    // Show provisioning status from any menu
    if (input == F("provision:status"))
    {
        deviceInfo.print();
        return true;
    }

    // If not recognized
    return false;
}

/****************************************************
 * buildBreadcrumbString()
 * - Builds "Parent > Current" recursively
 ****************************************************/
String buildBreadcrumbString(const SerialMenu *menu)
{
    if (!menu)
        return "";
    if (menu->parent == nullptr)
    {
        // This is the root
        return menu->name;
    }
    else
    {
        return buildBreadcrumbString(menu->parent) + " > " + menu->name;
    }
}
