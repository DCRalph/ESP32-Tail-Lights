# Device Provisioning System

This ESP32 device now includes a comprehensive provisioning system that ensures proper configuration before normal operation.

## Overview

The provisioning system prevents the device from operating normally until it has been properly configured with essential parameters. When `deviceInfo.provisioned` is `false`, the device will:

- Display a provisioning-only serial interface
- Show orange status LEDs to indicate provisioning mode
- Block all normal application functionality
- Only allow configuration through the provisioning menu

## Provisioning Menu

When the device is not provisioned, you'll see a dedicated provisioning menu with the following options:

### Menu Options

1. **Show Current Status** - Display current device configuration
2. **Set Serial Number** - Set a unique serial number (required)
3. **Set Hardware Version** - Set the hardware version (required)
4. **Toggle Debug Mode** - Enable/disable debug logging (optional)
5. **Complete Provisioning** - Finalize provisioning and enable normal operation
6. **Factory Reset** - Reset all device configuration
   h. **Help** - Show detailed help information

### Required Configuration

Before completing provisioning, you must set:

- **Serial Number**: A unique identifier for this device (1-4294967295)
- **Hardware Version**: The hardware revision number (1-65535)

### Optional Configuration

- **Debug Mode**: Enable detailed logging output
- **MAC Address**: Automatically set from WiFi chip (cannot be changed)

## Special Commands

The following special commands work from any menu:

- `provision:status` - Show current provisioning status
- `provision:force` - Force entry into provisioning mode
- `provision:bypass` - **DEBUG ONLY** - Bypass provisioning check

⚠️ **Warning**: The bypass command should only be used for debugging purposes.

## Normal Operation Access

Once provisioned, the device will:

- Start normally with full application functionality
- Show the main menu instead of provisioning menu
- Allow access to provisioning menu for reconfiguration via "Device Provisioning" option

## Factory Reset

The factory reset option will:

- Clear all stored device configuration
- Reset device to unprovisioned state
- Require complete re-provisioning
- Reset all preferences and settings

## Status Indicators

- **Orange Status LEDs**: Device in provisioning mode
- **Normal LED Patterns**: Device provisioned and operating normally

## Configuration Storage

Device configuration is stored in ESP32 preferences (flash memory) and persists across reboots. The configuration includes:

- Provisioning status
- Serial number
- Hardware version
- Debug mode setting
- MAC address

## Re-provisioning

To re-provision an already provisioned device:

1. From main menu, select "3) Device Provisioning"
2. Use option "5) Mark as Unprovisioned" to reset provisioning status
3. Device will restart in provisioning mode
4. Complete provisioning process again

Or use the special command `provision:force` from any menu to enter provisioning mode temporarily.
