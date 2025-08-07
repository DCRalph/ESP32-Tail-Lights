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
#define STRIP_ACTIVE_CHARACTERISTIC_UUID "63caefc6-8a24-4da2-ac3d-367b3f7c6be3"
#define SYNC_CHARACTERISTIC_UUID "192c22a9-547e-4c4f-b0f1-0e197042ee9b"

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
  float batteryVoltage;
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
  bool commit;
  bool serviceLights;
  uint8_t serviceLightsMode;
};

struct __attribute__((packed)) BLEStripActiveData
{
  bool headlight;
  bool taillight;
  bool underglow;
  bool interior;
};

// Discovered device info for BLE
struct __attribute__((packed)) BLEDiscoveredDevice
{
  uint32_t deviceId;
  uint8_t mac[6];
  uint32_t timeSinceLastSeen;
  bool inCurrentGroup;
  bool isGroupMaster;
  bool isThisDevice;
};

// Discovered group info for BLE
struct __attribute__((packed)) BLEDiscoveredGroup
{
  uint32_t groupId;
  uint32_t masterDeviceId;
  uint8_t masterMac[6];
  uint32_t timeSinceLastSeen;
  bool isCurrentGroup;
  bool canJoin;
};

// Group member info for BLE
struct __attribute__((packed)) BLEGroupMember
{
  uint32_t deviceId;
  uint8_t mac[6];
  bool isGroupMaster;
  bool isThisDevice;
  uint32_t lastHeartbeat;
};

struct __attribute__((packed)) BLESyncSendData
{
  uint8_t mode; // 0=SOLO, 1=JOIN, 2=HOST

  // Device and group information
  uint32_t deviceId;
  uint32_t groupId;
  uint32_t masterDeviceId;
  bool isMaster;
  bool timeSynced;
  int32_t timeOffset;
  uint32_t syncedTime;
  uint8_t memberCount;
  uint8_t discoveredDeviceCount;
  uint8_t discoveredGroupCount;
  uint32_t currentTime;

  // Discovered devices (up to 4 for BLE packet size limits)
  BLEDiscoveredDevice discoveredDevices[4];

  // Discovered groups (up to 2 for BLE packet size limits)
  BLEDiscoveredGroup discoveredGroups[2];

  // Current group members (up to 4 for BLE packet size limits)
  BLEGroupMember groupMembers[4];
};

struct __attribute__((packed)) BLESyncReceiveData
{
  uint8_t mode; // 0=SOLO, 1=JOIN, 2=HOST
  uint32_t groupId;
  uint8_t command; // 0=set_mode, 1=join_group, 2=create_group, 3=leave_group
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
  void updateSyncData();

  // Callbacks
  void setApplication(Application *app);

private:
  BLEManager();
  ~BLEManager();

  static BLEManager *instance;

  BLEServer *pServer;
  BLEService *pService;

  // Characteristic pointers
  BLECharacteristic *pPingCharacteristic;
  BLECharacteristic *pModeCharacteristic;
  BLECharacteristic *pEffectsCharacteristic;
  BLECharacteristic *pStripActiveCharacteristic;
  BLECharacteristic *pSyncCharacteristic;

  Application *app;
  bool deviceConnected;
  uint16_t connectionCount;

  // Data caching for notifications
  BLEPingData lastPingData;

  // Timing
  uint32_t lastPingUpdate;
  uint32_t lastSyncUpdate;

  void setupCharacteristics();
  void setupCallbacks();

  // Simplified characteristic callback handlers
  void handleModeWrite(BLECharacteristic *pCharacteristic);
  void handleEffectsWrite(BLECharacteristic *pCharacteristic);
  void handleStripActiveWrite(BLECharacteristic *pCharacteristic);
  void handleSyncWrite(BLECharacteristic *pCharacteristic);

  // Simplified data preparation methods
  BLEPingData preparePingData();
  BLEModeData prepareModeData();
  BLEEffectsData prepareEffectsData();
  BLEStripActiveData prepareStripActiveData();
  BLESyncSendData prepareSyncData();
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