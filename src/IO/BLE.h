#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <functional>
#include "config.h"
#include "IO/LED/Types.h"

// BLE Service and Characteristic UUIDs
#define BLE_SERVICE_UUID "51afeb92-c5fe-4efb-bf6f-5b9baf50d87f"

// Only 3 Characteristics
#define PING_CHARACTERISTIC_UUID "7285c8f6-30eb-4266-b5c3-7a34ba69a8b7"
#define MODE_CHARACTERISTIC_UUID "44969bf5-4a79-4fce-8acd-3b794f81c380"
#define EFFECTS_CHARACTERISTIC_UUID "7645b13d-3f3b-4046-8929-05c1c72e2900"

// Simplified BLE Data Structures
struct __attribute__((packed)) BLEPingData
{
  uint8_t mode;
  bool headlight;
  bool taillight;
  bool underglow;
  bool interior;
  uint32_t deviceId;
  uint8_t batteryLevel;
  uint32_t uptime;
};

struct __attribute__((packed)) BLEModeData
{
  uint8_t mode; // 0=NORMAL, 1=TEST, 2=REMOTE, 3=OFF
};

struct __attribute__((packed)) BLEEffectsData
{
  bool leftIndicator;
  bool rightIndicator;
  uint8_t headlightMode;
  bool headlightSplit;
  bool headlightR;
  bool headlightG;
  bool headlightB;
  uint8_t taillightMode;
  bool taillightSplit;
  bool brake;
  bool reverse;
  bool rgb;
  bool nightrider;
  bool police;
  uint8_t policeMode;
  bool pulseWave;
  bool aurora;
  bool solidColor;
  uint8_t solidColorPreset;
  uint8_t solidColorR;
  uint8_t solidColorG;
  uint8_t solidColorB;
  bool colorFade;
};

// Forward declarations
class Application;

class BLEManager
{
  friend class CarThingBLEServerCallbacks;
  friend class CarThingBLECharacteristicCallbacks;

public:
  static BLEManager *getInstance();

  void begin();
  void loop();
  void end();

  bool isConnected();
  uint16_t getConnectionCount();

  // Data update methods (called by Application)
  void updatePingData();

  // Callbacks
  void setApplication(Application *app);

private:
  BLEManager();
  ~BLEManager();

  static BLEManager *instance;

  BLEServer *pServer;
  BLEService *pService;

  // Only 3 characteristic pointers
  BLECharacteristic *pPingCharacteristic;
  BLECharacteristic *pModeCharacteristic;
  BLECharacteristic *pEffectsCharacteristic;

  Application *app;
  bool deviceConnected;
  uint16_t connectionCount;

  // Data caching for notifications
  BLEPingData lastPingData;

  // Timing
  uint32_t lastPingUpdate;

  void setupCharacteristics();
  void setupCallbacks();

  // Simplified characteristic callback handlers
  void handleModeWrite(BLECharacteristic *pCharacteristic);
  void handleEffectsWrite(BLECharacteristic *pCharacteristic);

  // Simplified data preparation methods
  BLEPingData preparePingData();
  BLEModeData prepareModeData();
  BLEEffectsData prepareEffectsData();
};

// BLE Server Callbacks
class CarThingBLEServerCallbacks : public BLEServerCallbacks
{
public:
  CarThingBLEServerCallbacks(BLEManager *manager);
  void onConnect(BLEServer *pServer) override;
  void onDisconnect(BLEServer *pServer) override;

private:
  BLEManager *bleManager;
};

// Generic BLE Characteristic Callbacks
class CarThingBLECharacteristicCallbacks : public BLECharacteristicCallbacks
{
public:
  CarThingBLECharacteristicCallbacks(BLEManager *manager, const String &charName);
  void onWrite(BLECharacteristic *pCharacteristic) override;
  void onRead(BLECharacteristic *pCharacteristic) override;

private:
  BLEManager *bleManager;
  String characteristicName;
};