# ESP32 Tail Lights Sync System

This system allows multiple ESP32 devices to coordinate their light effects when they're in proximity with each other. For example, when multiple cars equipped with ESP32 Tail Lights are driving together, their light effects can automatically synchronize.

## Features

- **Automatic Master Election**: The system uses a priority-based master election mechanism to decide which device controls the synchronized effects
- **Effect Synchronization**: When multiple devices are in range, their effects are synchronized based on the master device's state
- **Broadcast Communication**: Uses ESP-NOW broadcast messages to communicate with all devices in range
- **Resilience**: Handles device disconnections gracefully; if the master disappears, a new one is elected

## How It Works

The system uses ESP-NOW to establish a peer-to-peer network between ESP32 devices:

1. **Device Discovery**: Each device broadcasts periodic heartbeat messages, which include a random priority number
2. **Master Election**: The device with the highest priority becomes the "master"
3. **Effect Synchronization**: The master device broadcasts its effect states to all other devices
4. **Failover**: If the master device disappears, a new one is automatically elected

## Integration with Existing Code

The sync system is designed to work with the existing wireless framework:

- It uses the existing `Wireless` class for communication without modifying it
- It integrates with the main `Application` class to apply synchronized effects
- When syncing is active, only the master device responds to physical inputs; other devices follow the master's state

## Enabling Debug Output

To enable debug messages for troubleshooting, add the following to your config.h file:

```cpp
#define DEBUG_SYNC
```

## Packet Types

The sync system uses the following packet types:

- **SYNC_HEARTBEAT (0x01)**: Periodic messages to announce presence and share device priority
- **SYNC_MASTER_ANNOUNCE (0x02)**: Sent when a device declares itself as master
- **SYNC_EFFECT_STATE (0x03)**: Sent by the master to synchronize effect states
- **SYNC_MASTER_REQUEST (0x04)**: Used to request which device is the current master
- **SYNC_MASTER_RESIGN (0x05)**: Sent when a master device wants to give up control

## Technical Notes

- The system uses the ESP-NOW broadcast address (FF:FF:FF:FF:FF:FF) to communicate with all devices in range
- The master election is based on a random priority number generated at startup
- The system handles up to 20 devices simultaneously (ESP-NOW peer limit)
- Devices are considered disconnected if they haven't sent a heartbeat for 5 seconds
