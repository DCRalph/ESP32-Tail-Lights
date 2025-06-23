# ESP32 Tail Lights Mobile App Development Guide

## Overview - SIMPLIFIED BLE INTERFACE

This guide helps you create mobile applications to control an ESP32-based tail lights system via **DRAMATICALLY SIMPLIFIED** Bluetooth Low Energy (BLE). The previous complex interface with 21+ characteristics has been reduced to just **3 characteristics** for maximum simplicity.

## Key Advantages of Simplified Interface

✅ **Only 3 characteristics** instead of 21+  
✅ **Easier development** and maintenance  
✅ **Reduced complexity** in mobile apps  
✅ **Better performance** with less BLE overhead  
✅ **Consolidated control** - all effects in one place  
✅ **Simpler state management**  
✅ **Faster prototyping** and testing  

## BLE Service Structure

**Service UUID:** `12345678-1234-5678-9012-123456789abc`

### The Only 3 Characteristics You Need

#### 1. 📡 Ping/Status (Read + Notify)
- **UUID**: `12345678-1234-5678-9012-123456789001`
- **Purpose**: Real-time device status and information
- **Updates**: Automatic notifications every 1000ms
- **Size**: 14 bytes

```c
struct BLEPingData {
  uint8_t mode;           // Current mode (0-3)
  bool headlight;         // Headlight strip enabled
  bool taillight;         // Taillight strip enabled
  bool underglow;         // Underglow strip enabled
  bool interior;          // Interior strip enabled
  uint32_t deviceId;      // Unique device ID
  uint8_t batteryLevel;   // Battery level (0-100%)
  uint32_t uptime;        // Uptime in seconds
}
```

#### 2. 🎛️ Mode Control (Read + Write + Notify)
- **UUID**: `12345678-1234-5678-9012-123456789002`
- **Purpose**: Application mode control
- **Size**: 1 byte

```c
struct BLEModeData {
  uint8_t mode; // 0=NORMAL, 1=TEST, 2=REMOTE, 3=OFF
}
```

**Mode Values:**
- `0` = **NORMAL** - Normal car operation
- `1` = **TEST** - Test mode (input overrides)
- `2` = **REMOTE** - Remote control enabled
- `3` = **OFF** - All effects disabled

#### 3. 🎨 Effects Control (Read + Write + Notify)
- **UUID**: `12345678-1234-5678-9012-123456789003`
- **Purpose**: Complete lighting effects control
- **Size**: 23 bytes

```c
struct BLEEffectsData {
  // Turn Indicators
  bool leftIndicator;        // Left turn signal
  bool rightIndicator;       // Right turn signal
  
  // Headlight Control
  uint8_t headlightMode;     // 0=Off, 1=Startup, 2=CarOn
  bool headlightSplit;       // Split headlight mode
  bool headlightR;           // Red component
  bool headlightG;           // Green component
  bool headlightB;           // Blue component
  
  // Taillight Control
  uint8_t taillightMode;     // Taillight mode
  bool taillightSplit;       // Split taillight mode
  
  // Input States
  bool brake;                // Brake light active
  bool reverse;              // Reverse light active
  
  // Special Effects
  bool rgb;                  // RGB rainbow effect
  bool nightrider;           // Knight Rider effect
  bool police;               // Police light effect
  uint8_t policeMode;        // 0=SLOW, 1=FAST
  bool pulseWave;            // Pulse wave effect
  bool aurora;               // Aurora effect
  bool solidColor;           // Solid color effect
  uint8_t solidColorPreset;  // Color preset
  uint8_t solidColorR;       // Custom red (0-255)
  uint8_t solidColorG;       // Custom green (0-255)
  uint8_t solidColorB;       // Custom blue (0-255)
  bool colorFade;            // Color fade effect
}
```

## Mobile App Architecture

### Recommended Structure
```
TailLightsApp/
├── BLE/
│   ├── TailLightsController
│   └── BLEDataHelper
├── UI/
│   ├── ConnectionScreen
│   ├── DashboardScreen
│   ├── ModeScreen
│   └── EffectsScreen
└── Utils/
    └── Constants
```

## React Native Implementation

```javascript
import { BleManager } from 'react-native-ble-plx';

const SERVICE_UUID = '12345678-1234-5678-9012-123456789abc';
const PING_UUID = '12345678-1234-5678-9012-123456789001';
const MODE_UUID = '12345678-1234-5678-9012-123456789002';
const EFFECTS_UUID = '12345678-1234-5678-9012-123456789003';

class TailLightsController {
  constructor() {
    this.manager = new BleManager();
    this.device = null;
    this.isConnected = false;
  }

  async scanAndConnect() {
    return new Promise((resolve, reject) => {
      this.manager.startDeviceScan(null, null, (error, device) => {
        if (error) {
          reject(error);
          return;
        }

        if (device.name?.startsWith('ESP32-TailLights-')) {
          this.manager.stopDeviceScan();
          this.connectToDevice(device.id)
            .then(() => resolve(device))
            .catch(reject);
        }
      });
    });
  }

  async connectToDevice(deviceId) {
    this.device = await this.manager.connectToDevice(deviceId);
    await this.device.discoverAllServicesAndCharacteristics();
    
    // Subscribe to ping notifications
    await this.device.monitorCharacteristicForService(
      SERVICE_UUID, PING_UUID, (error, characteristic) => {
        if (!error) {
          const pingData = this.parsePingData(characteristic.value);
          this.onPingReceived?.(pingData);
        }
      }
    );

    this.isConnected = true;
  }

  async setMode(mode) {
    const data = new Uint8Array([mode]);
    await this.device.writeCharacteristicWithResponseForService(
      SERVICE_UUID, MODE_UUID, data.buffer
    );
  }

  async enableRGBEffect() {
    const effectsData = new Uint8Array(23);
    effectsData.fill(0);
    effectsData[11] = 1; // Enable RGB
    await this.device.writeCharacteristicWithResponseForService(
      SERVICE_UUID, EFFECTS_UUID, effectsData.buffer
    );
  }

  async setSolidColor(preset, r = 0, g = 0, b = 0) {
    const effectsData = new Uint8Array(23);
    effectsData.fill(0);
    effectsData[17] = 1; // Enable solid color
    effectsData[18] = preset;
    effectsData[19] = r;
    effectsData[20] = g;
    effectsData[21] = b;
    await this.device.writeCharacteristicWithResponseForService(
      SERVICE_UUID, EFFECTS_UUID, effectsData.buffer
    );
  }

  async enablePoliceEffect(fastMode = true) {
    const effectsData = new Uint8Array(23);
    effectsData.fill(0);
    effectsData[13] = 1; // Enable police
    effectsData[14] = fastMode ? 1 : 0; // Set mode
    await this.device.writeCharacteristicWithResponseForService(
      SERVICE_UUID, EFFECTS_UUID, effectsData.buffer
    );
  }

  async setIndicators(left, right) {
    const effectsData = new Uint8Array(23);
    effectsData.fill(0);
    effectsData[0] = left ? 1 : 0;
    effectsData[1] = right ? 1 : 0;
    await this.device.writeCharacteristicWithResponseForService(
      SERVICE_UUID, EFFECTS_UUID, effectsData.buffer
    );
  }

  parsePingData(base64Data) {
    const buffer = Buffer.from(base64Data, 'base64');
    const view = new DataView(buffer.buffer);
    
    return {
      mode: view.getUint8(0),
      headlight: view.getUint8(1) !== 0,
      taillight: view.getUint8(2) !== 0,
      underglow: view.getUint8(3) !== 0,
      interior: view.getUint8(4) !== 0,
      deviceId: view.getUint32(5, true),
      batteryLevel: view.getUint8(9),
      uptime: view.getUint32(10, true)
    };
  }

  disconnect() {
    this.device?.cancelConnection();
    this.device = null;
    this.isConnected = false;
  }
}
```

## Flutter Implementation

```dart
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'dart:typed_data';

class TailLightsController {
  static const String serviceUUID = '12345678-1234-5678-9012-123456789abc';
  static const String pingUUID = '12345678-1234-5678-9012-123456789001';
  static const String modeUUID = '12345678-1234-5678-9012-123456789002';
  static const String effectsUUID = '12345678-1234-5678-9012-123456789003';
  
  BluetoothDevice? device;
  BluetoothCharacteristic? modeChar;
  BluetoothCharacteristic? effectsChar;
  bool isConnected = false;

  Future<void> scanAndConnect() async {
    FlutterBluePlus.startScan(timeout: Duration(seconds: 10));
    
    await for (ScanResult result in FlutterBluePlus.scanResults) {
      if (result.device.name.startsWith('ESP32-TailLights-')) {
        await FlutterBluePlus.stopScan();
        await connectToDevice(result.device);
        break;
      }
    }
  }

  Future<void> connectToDevice(BluetoothDevice targetDevice) async {
    device = targetDevice;
    await device!.connect();
    
    List<BluetoothService> services = await device!.discoverServices();
    for (BluetoothService service in services) {
      if (service.uuid.toString().toLowerCase() == serviceUUID.toLowerCase()) {
        for (BluetoothCharacteristic char in service.characteristics) {
          String charUuid = char.uuid.toString().toLowerCase();
          
          if (charUuid == modeUUID.toLowerCase()) {
            modeChar = char;
          } else if (charUuid == effectsUUID.toLowerCase()) {
            effectsChar = char;
          }
        }
        break;
      }
    }
    
    isConnected = true;
  }

  Future<void> setMode(int mode) async {
    await modeChar?.write([mode]);
  }

  Future<void> enableRGBEffect() async {
    List<int> data = List.filled(23, 0);
    data[11] = 1; // Enable RGB
    await effectsChar?.write(data);
  }

  Future<void> setSolidColor(int preset, {int r = 0, int g = 0, int b = 0}) async {
    List<int> data = List.filled(23, 0);
    data[17] = 1; // Enable solid color
    data[18] = preset;
    data[19] = r;
    data[20] = g;
    data[21] = b;
    await effectsChar?.write(data);
  }

  Future<void> enablePoliceEffect({bool fastMode = true}) async {
    List<int> data = List.filled(23, 0);
    data[13] = 1; // Enable police
    data[14] = fastMode ? 1 : 0;
    await effectsChar?.write(data);
  }

  Future<void> setIndicators({bool left = false, bool right = false}) async {
    List<int> data = List.filled(23, 0);
    data[0] = left ? 1 : 0;
    data[1] = right ? 1 : 0;
    await effectsChar?.write(data);
  }

  void disconnect() {
    device?.disconnect();
    isConnected = false;
  }
}
```

## UI/UX Recommendations

### 📱 Dashboard Screen
- **Connection Status**: Clear indicator
- **Device Info**: Battery, uptime from ping data
- **Quick Actions**: RGB, Police, Indicators buttons
- **Strip Status**: Visual indicators for enabled strips

### 🎨 Effects Screen
- **Effect Categories**: Indicators, Special Effects, Colors
- **Toggle Switches**: Simple on/off for each effect
- **Color Picker**: For custom RGB values
- **Preset Buttons**: Common colors (Red, Blue, Green, etc.)
- **Conflict Prevention**: Auto-disable conflicting effects

### ⚙️ Mode Screen
- **Mode Buttons**: Clear buttons for each mode
- **Descriptions**: Explain what each mode does
- **Safety Warnings**: Alert for TEST/OFF modes

## Data Structure Byte Map

### BLEEffectsData Layout (23 bytes)
```
Byte 0:  leftIndicator
Byte 1:  rightIndicator  
Byte 2:  headlightMode
Byte 3:  headlightSplit
Byte 4:  headlightR
Byte 5:  headlightG
Byte 6:  headlightB
Byte 7:  taillightMode
Byte 8:  taillightSplit
Byte 9:  brake
Byte 10: reverse
Byte 11: rgb
Byte 12: nightrider
Byte 13: police
Byte 14: policeMode
Byte 15: pulseWave
Byte 16: aurora
Byte 17: solidColor
Byte 18: solidColorPreset
Byte 19: solidColorR
Byte 20: solidColorG
Byte 21: solidColorB
Byte 22: colorFade
```

## Common Usage Patterns

### 🌈 Enable RGB Effect
```javascript
const data = new Uint8Array(23);
data.fill(0);
data[11] = 1; // RGB on
```

### 🔴 Set Red Color
```javascript
const data = new Uint8Array(23);
data.fill(0);
data[17] = 1; // Solid color on
data[18] = 1; // Red preset
```

### 🚨 Police Lights (Fast)
```javascript
const data = new Uint8Array(23);
data.fill(0);
data[13] = 1; // Police on
data[14] = 1; // Fast mode
```

### ⬅️➡️ Turn Signals
```javascript
const data = new Uint8Array(23);
data.fill(0);
data[0] = 1; // Left indicator
data[1] = 0; // Right indicator off
```

## Testing Strategy

### 🔧 Development Testing
1. Use nRF Connect app to verify 3 characteristics
2. Test connection and ping notifications
3. Verify mode switching works
4. Test effect activation/deactivation
5. Check error handling

### 👥 User Testing
1. Easy device discovery and connection
2. Intuitive effect controls
3. Visual feedback for all actions
4. Error recovery scenarios

## Key Benefits

✅ **Simplified Development**: Only 3 characteristics to manage  
✅ **Faster Implementation**: Less code, fewer bugs  
✅ **Better UX**: Consolidated controls, cleaner interface  
✅ **Easier Debugging**: Simple architecture to troubleshoot  
✅ **Better Performance**: Less BLE overhead  
✅ **Rapid Prototyping**: Quick to test new features  

This simplified BLE interface makes mobile app development dramatically easier while maintaining full control over the ESP32 Tail Lights system! 