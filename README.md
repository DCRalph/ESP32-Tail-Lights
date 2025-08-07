# ESP32 Custom LED Tail Lights

# OUTDATED
Will update someday

This project implements a comprehensive LED lighting system for vehicles using ESP32 microcontrollers and WS2812B LED strips. The system supports multiple lighting configurations (headlights, taillights, underglow, interior), wireless communication, synchronized multi-device operation, and extensive visual effects.

## Features

### Core Lighting Effects

- **Taillight Effects** - Multiple modes with configurable split operation
- **Headlight Effects** - Various modes with RGB color control and split configuration
- **Brake Light Animation** - Dynamic brake light effects with tap sequences
- **Reverse Light Animation** - Special animated effects for reverse operation
- **Left/Right Indicators** - Smooth indicator animations with flicker sequences
- **RGB Cycling** - Full spectrum color cycling effects
- **Night Rider** - Classic KITT-style sweeping light effect
- **Police Effect** - Emergency lighting patterns with multiple modes
- **Aurora Effect** - Dynamic aurora-like color patterns
- **Pulse Wave Effect** - Rhythmic pulsing light patterns

### Advanced Features

- **Multi-Device Synchronization** - ESP-NOW based system allowing multiple vehicles to synchronize their lighting effects
- **Wireless Remote Control** - Full wireless control via ESP-NOW protocol with comprehensive command set
- **Multiple Operation Modes**:
  - Normal Mode - Standard operation based on vehicle inputs
  - Test Mode - Manual control for testing and configuration
  - Remote Mode - Wireless control from external devices
  - Off Mode - System shutdown with minimal power consumption
- **Effect Sequences** - Programmable sequences for unlock/lock animations and special effects
- **Real-time Statistics** - Performance monitoring and loop timing statistics

### Hardware Integration

- **High-Voltage Input Support** - Direct integration with vehicle electrical systems (12V ACC, indicators, brake, reverse)
- **Multiple Board Configurations** - Support for ESP32-S2, ESP32-S3 variants with custom board definitions
- **Flexible GPIO Configuration** - Configurable pin assignments for different hardware setups

## Hardware Requirements

### Primary Components

- **ESP32** microcontroller (S2 or S3 variants supported)
- **WS2812B** LED strips (configurable count per strip type)
- **12V to 3.3V level shifters** for vehicle integration
- Wires and connectors for integration with vehicle electrical system

### Supported Configurations

- **Headlights**
- **Taillights**
- **Underglow**
- **Interior**

## Software Requirements

- **[PlatformIO](https://platformio.org/)** development environment
- **ESP32 Arduino Framework** (version 6.11.0)
- **FastLED** library for WS2812B control
- **ESP-NOW** for wireless communication (built into ESP32 framework)

## Project Structure

```
ESP32-Tail-Lights/
├── src/
│   ├── Application.cpp/.h          # Main application logic
│   ├── config.h                    # Hardware and feature configuration
│   ├── main.cpp                    # Entry point and setup
│   ├── AppStuff/                   # Application modes and effects management
│   ├── IO/                         # Input/Output handling
│   │   ├── GPIO.cpp/.h            # GPIO management
│   │   ├── Inputs.cpp/.h          # High-voltage input handling
│   │   ├── Wireless.cpp/.h        # ESP-NOW communication
│   │   └── LED/                   # LED strip management
│   │       ├── LEDStrip.cpp/.h    # Core LED strip functionality
│   │       ├── LEDStripManager.cpp/.h # Multi-strip coordination
│   │       └── Effects/           # Individual lighting effects
│   ├── Sequences/                  # Programmable effect sequences
│   └── Sync/                      # Multi-device synchronization system
├── boards/                        # Custom board definitions
├── extra/
│   └── sim.py                     # Python LED effect simulator
├── platformio.ini                 # Build configuration
└── partitions.csv                 # ESP32 partition table
```

## Setup and Installation

### 1. Clone the Repository

```bash
git clone https://github.com/DCRalph/ESP32-Tail-Lights.git
cd ESP32-Tail-Lights
```

### 2. Configure Your Build Environment

The project uses PlatformIO with pre-configured environments:

- `esp32s3` - For ESP32-S3 based builds (default configuration)
- `esp32s2` - For ESP32-S2 based builds

### 3. Hardware Configuration

Configure your hardware setup in `src/config.h`:

```cpp
// Choose your board configuration
#define S2_CAR          // For ESP32-S2 car installation
// #define S3_V1        // For ESP32-S3 version 1
// #define S3_DEV       // For ESP32-S3 development board

// Enable desired LED strips
#define ENABLE_TAILLIGHTS
// #define ENABLE_HEADLIGHTS
// #define ENABLE_UNDERGLOW
// #define ENABLE_INTERIOR

// Configure LED counts and pins
#ifdef ENABLE_TAILLIGHTS
#define TAILLIGHT_LED_COUNT 120
#define TAILLIGHT_LED_PIN 16
#endif
```

### 4. Vehicle Integration

Connect the high-voltage inputs to your vehicle's electrical system:

**Default ESP32-S2 Car Configuration:**

```cpp
#define INPUT_1_PIN 1    // 12V ACC (Accessory power)
#define INPUT_2_PIN 2    // Left indicator
#define INPUT_3_PIN 3    // Right indicator
#define INPUT_4_PIN 4    // Headlight signal
#define INPUT_5_PIN 5    // Brake signal
#define INPUT_6_PIN 6    // Reverse signal
```

### 5. Build and Upload

```bash
# For ESP32-S3
pio run -e esp32s3 --target upload

# For ESP32-S2
pio run -e esp32s2 --target upload
```

## Multi-Device Synchronization

The project includes an advanced synchronization system that allows multiple ESP32 devices to coordinate their lighting effects:

### Features

- **Automatic Master Election** - Devices automatically elect a master for coordination
- **Effect Synchronization** - All devices mirror the master's lighting state
- **Resilient Communication** - Handles device disconnections gracefully
- **Broadcast Protocol** - Uses ESP-NOW for peer-to-peer communication

### Configuration

Enable synchronization in `config.h`:

```cpp
#define ENABLE_SYNC
#define DEBUG_SYNC  // Optional: Enable debug output
```

## Wireless Control

The system supports comprehensive wireless control via ESP-NOW protocol:

### Available Commands

- **Ping** (`0xe0`) - Device status and capability discovery
- **Set Mode** (`0xe1`) - Switch between Normal/Test/Remote/Off modes
- **Set Effects** (`0xe2`) - Control individual lighting effects
- **Get Effects** (`0xe3`) - Query current effect states
- **Set Inputs** (`0xe4`) - Override input states (Test mode)
- **Get Inputs** (`0xe5`) - Read current input states
- **Trigger Sequence** (`0xe6`) - Activate predefined sequences
- **Get Stats** (`0xe7`) - Retrieve performance statistics

## Development Tools

### LED Effect Simulator

The project includes a Python-based LED effect simulator (`extra/sim.py`) for development and testing:

```bash
cd extra
python sim.py
```

This allows you to preview lighting effects without physical hardware.

### Debug Features

Enable various debug outputs in `config.h`:

```cpp
#define DEBUG_ESP_NOW    // ESP-NOW communication debug
#define DEBUG_SYNC       // Synchronization debug
#define CORE_DEBUG_LEVEL=3  // ESP32 core debug level
```

## Customization

### Adding New Effects

1. Create effect class inheriting from `LEDEffect` in `src/IO/LED/Effects/`
2. Implement `update()` and `render()` methods
3. Register effect in `Application::setupEffects()`

### Custom Sequences

1. Create sequence class inheriting from `SequenceBase` in `src/Sequences/`
2. Implement sequence logic and timing
3. Register sequence in `Application::setupSequences()`

### Hardware Variants

1. Add board definition in `boards/` directory
2. Update `config.h` with new pin definitions
3. Configure `platformio.ini` build environment

## Performance

The system is optimized for real-time operation:

- **High-frequency main loop** with microsecond timing precision
- **Multi-core task distribution** utilizing both ESP32 cores
- **Efficient LED buffer management** with priority-based effect rendering
- **Performance monitoring** with loop timing statistics

## License

This project is licensed under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for full details.

## Contributing

This is an open-source project welcoming contributions. Please ensure any modifications maintain compatibility with the existing wireless protocol and synchronization system.
