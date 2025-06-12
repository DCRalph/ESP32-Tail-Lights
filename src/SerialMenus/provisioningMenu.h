/****************************************************
 * provisioningMenu.h
 *
 * Defines the provisioning menu system that allows
 * device configuration before the main application
 * functionality is enabled.
 ****************************************************/

#pragma once
#include "SerialMenu.h"

extern Menu provisioningMenu;

void printProvisioningMenu(const Menu &menu);
bool handleProvisioningMenuInput(Menu &menu, const String &input);

// Provisioning functions
void startProvisioning();
void setSerialNumber(uint32_t serialNumber);
void setHardwareVersion(uint16_t hardwareVersion);
void enableDebugMode(bool enabled);
void completeProvisioning();
void showProvisioningStatus();
void factoryReset();