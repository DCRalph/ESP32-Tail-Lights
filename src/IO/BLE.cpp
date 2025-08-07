#include "BLE.h"
#include "../Application.h"
#include "LED/LEDStripManager.h"
#include "TimeProfiler.h"
#include <esp_system.h>
#include "Battery.h"
#include "../Sync/SyncManager.h"

// Initialize static instance
BLEManager *BLEManager::instance = nullptr;

BLEManager *BLEManager::getInstance()
{
  if (!instance)
  {
    instance = new BLEManager();
  }
  return instance;
}

BLEManager::BLEManager()
    : pServer(nullptr), pService(nullptr), app(nullptr), deviceConnected(false), connectionCount(0), lastPingUpdate(0), lastSyncUpdate(0)
{
  // Initialize characteristic pointers to nullptr
  pPingCharacteristic = nullptr;
  pModeCharacteristic = nullptr;
  pEffectsCharacteristic = nullptr;
  pStripActiveCharacteristic = nullptr;
  pSyncCharacteristic = nullptr;
}

BLEManager::~BLEManager()
{
  end();
}

void BLEManager::begin()
{
  Serial.println("BLEManager: Initializing BLE...");

  // Initialize BLE Device
  String deviceName = "ESP32-" + String(deviceInfo.serialNumber);
  BLEDevice::init(deviceName.c_str());

  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new CarThingBLEServerCallbacks(this));

  // Create BLE Service
  pService = pServer->createService(BLE_SERVICE_UUID);

  setupCharacteristics();
  setupCallbacks();

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // Functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLEManager: BLE service started and advertising");
}

void BLEManager::setupCharacteristics()
{
  // Ping Characteristic (Read only + Notify)
  pPingCharacteristic = pService->createCharacteristic(
      PING_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pPingCharacteristic->addDescriptor(new BLE2902());

  // Mode Characteristic (Read/Write + Notify)
  pModeCharacteristic = pService->createCharacteristic(
      MODE_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pModeCharacteristic->addDescriptor(new BLE2902());

  // Effects Characteristic (Read/Write + Notify)
  pEffectsCharacteristic = pService->createCharacteristic(
      EFFECTS_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pEffectsCharacteristic->addDescriptor(new BLE2902());

  // Strip Active Characteristic (Read/Write + Notify)
  pStripActiveCharacteristic = pService->createCharacteristic(
      STRIP_ACTIVE_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pStripActiveCharacteristic->addDescriptor(new BLE2902());

  // Sync Characteristic (Read/Write + Notify)
  pSyncCharacteristic = pService->createCharacteristic(
      SYNC_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pSyncCharacteristic->addDescriptor(new BLE2902());
}

void BLEManager::setupCallbacks()
{
  // Setup callbacks for all characteristics
  pPingCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Ping"));
  pModeCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Mode"));
  pEffectsCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Effects"));
  pStripActiveCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Strip Active"));
  pSyncCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Sync"));
}

void BLEManager::loop()
{
  uint32_t now = millis();

  if (!deviceConnected)
  {
    return;
  }

  // Update ping data every 1000ms
  if (now - lastPingUpdate > 1000)
  {
    updatePingData();
    lastPingUpdate = now;
  }

  // Update sync data every 2000ms (less frequent to avoid overwhelming BLE)
  if (now - lastSyncUpdate > 2000)
  {
    updateSyncData();
    lastSyncUpdate = now;
  }
}

void BLEManager::end()
{
  if (pServer)
  {
    pServer->getAdvertising()->stop();
    BLEDevice::deinit();
    pServer = nullptr;
    pService = nullptr;
  }
  deviceConnected = false;
  connectionCount = 0;
}

bool BLEManager::isConnected()
{
  return deviceConnected;
}

uint16_t BLEManager::getConnectionCount()
{
  return connectionCount;
}

void BLEManager::setApplication(Application *application)
{
  app = application;
}

void BLEManager::updatePingData()
{
  if (!deviceConnected || !pPingCharacteristic)
  {
    return;
  }

  BLEPingData pingData = preparePingData();

  // Check if data has changed
  bool changed = memcmp(&pingData, &lastPingData, sizeof(BLEPingData)) != 0;
  if (changed)
  {
    pPingCharacteristic->setValue((uint8_t *)&pingData, sizeof(pingData));
    pPingCharacteristic->notify();
    lastPingData = pingData;
  }
}

// Data preparation methods
BLEPingData BLEManager::preparePingData()
{
  BLEPingData data = {};

  if (!app)
    return data;

  LEDStripManager *ledManager = LEDStripManager::getInstance();

  data.mode = static_cast<uint8_t>(app->getMode());
  data.headlight = ledManager->isStripEnabled(LEDStripType::HEADLIGHT);
  data.taillight = ledManager->isStripEnabled(LEDStripType::TAILLIGHT);
  data.underglow = ledManager->isStripEnabled(LEDStripType::UNDERGLOW);
  data.interior = ledManager->isStripEnabled(LEDStripType::INTERIOR);
  data.deviceId = (uint32_t)ESP.getEfuseMac();
  data.batteryLevel = batteryGetPercentage();
  data.batteryVoltage = batteryGetVoltage();
  data.uptime = millis() / 1000;

  return data;
}

BLEModeData BLEManager::prepareModeData()
{
  BLEModeData data = {};

  if (app)
  {
    data.mode = static_cast<uint8_t>(app->getMode());
  }

  return data;
}

BLEEffectsData BLEManager::prepareEffectsData()
{
  BLEEffectsData data = {};

  if (!app)
    return data;

  // Get effect states from the application
  data.leftIndicator = app->leftIndicatorEffect ? app->leftIndicatorEffect->isActive() : false;
  data.rightIndicator = app->rightIndicatorEffect ? app->rightIndicatorEffect->isActive() : false;

  if (app->headlightEffect)
  {
    data.headlightMode = static_cast<uint8_t>(app->headlightEffect->getMode());
    data.headlightSplit = app->headlightEffect->getSplit();
    bool r, g, b;
    app->headlightEffect->getColor(r, g, b);
    data.headlightR = r;
    data.headlightG = g;
    data.headlightB = b;
  }

  if (app->taillightEffect)
  {
    data.taillightMode = static_cast<uint8_t>(app->taillightEffect->getMode());
    data.taillightSplit = app->taillightEffect->getSplit();
  }

  data.brake = app->brakeInput.get();
  data.reverse = app->reverseInput.get();
  data.rgb = app->rgbEffect ? app->rgbEffect->isActive() : false;
  data.nightrider = app->nightriderEffect ? app->nightriderEffect->isActive() : false;
  data.police = app->policeEffect ? app->policeEffect->isActive() : false;
  data.policeMode = app->policeEffect ? static_cast<uint8_t>(app->policeEffect->getMode()) : 0;
  data.pulseWave = app->pulseWaveEffect ? app->pulseWaveEffect->isActive() : false;
  data.aurora = app->auroraEffect ? app->auroraEffect->isActive() : false;
  data.solidColor = app->solidColorEffect ? app->solidColorEffect->isActive() : false;
  data.colorFade = app->colorFadeEffect ? app->colorFadeEffect->isActive() : false;
  data.commit = app->commitEffect ? app->commitEffect->isActive() : false;
  data.serviceLights = app->serviceLightsEffect ? app->serviceLightsEffect->isActive() : false;
  data.serviceLightsMode = app->serviceLightsEffect ? static_cast<uint8_t>(app->serviceLightsEffect->getMode()) : 0;

  if (app->solidColorEffect)
  {
    data.solidColorPreset = static_cast<uint8_t>(app->solidColorEffect->getColorPreset());
    uint8_t r, g, b;
    app->solidColorEffect->getCustomColor(r, g, b);
    data.solidColorR = r;
    data.solidColorG = g;
    data.solidColorB = b;
  }

  return data;
}

BLEStripActiveData BLEManager::prepareStripActiveData()
{
  BLEStripActiveData data = {};

  if (!app)
    return data;

  LEDStripManager *ledManager = LEDStripManager::getInstance();
  data.headlight = ledManager->isStripActive(LEDStripType::HEADLIGHT);
  data.taillight = ledManager->isStripActive(LEDStripType::TAILLIGHT);
  data.underglow = ledManager->isStripActive(LEDStripType::UNDERGLOW);
  data.interior = ledManager->isStripActive(LEDStripType::INTERIOR);

  return data;
}

BLESyncSendData BLEManager::prepareSyncData()
{
  BLESyncSendData data = {};

  if (!app)
    return data;

  SyncManager *syncMgr = SyncManager::getInstance();
  const auto &groupInfo = syncMgr->getGroupInfo();
  const auto &discoveredDevices = syncMgr->getDiscoveredDevices();
  const auto discoveredGroups = syncMgr->getDiscoveredGroups();
  uint32_t ourDeviceId = syncMgr->getDeviceId();
  uint32_t now = millis();

  // Basic sync information
  data.mode = static_cast<uint8_t>(syncMgr->getSyncMode());
  data.deviceId = ourDeviceId;
  data.groupId = groupInfo.groupId;
  data.masterDeviceId = groupInfo.masterDeviceId;
  data.isMaster = groupInfo.isMaster;
  data.timeSynced = syncMgr->isTimeSynced();
  data.timeOffset = syncMgr->getTimeOffset();
  data.syncedTime = syncMgr->getSyncedTime();
  data.memberCount = std::min((size_t)255, groupInfo.members.size());
  data.discoveredDeviceCount = std::min((size_t)255, discoveredDevices.size());
  data.discoveredGroupCount = std::min((size_t)255, discoveredGroups.size());
  data.currentTime = now;

  // Fill discovered devices (up to 4 for BLE packet size limits)
  int deviceIdx = 0;
  for (const auto &devicePair : discoveredDevices)
  {
    if (deviceIdx >= 4)
      break;

    const auto &device = devicePair.second;
    data.discoveredDevices[deviceIdx].deviceId = device.deviceId;
    memcpy(data.discoveredDevices[deviceIdx].mac, device.mac, 6);
    data.discoveredDevices[deviceIdx].timeSinceLastSeen = now - device.lastSeen;
    data.discoveredDevices[deviceIdx].isThisDevice = (device.deviceId == ourDeviceId);

    // Check if device is in our current group
    data.discoveredDevices[deviceIdx].inCurrentGroup = false;
    data.discoveredDevices[deviceIdx].isGroupMaster = false;

    if (groupInfo.groupId != 0)
    {
      auto memberIt = groupInfo.members.find(devicePair.first);
      if (memberIt != groupInfo.members.end())
      {
        data.discoveredDevices[deviceIdx].inCurrentGroup = true;
        data.discoveredDevices[deviceIdx].isGroupMaster = (device.deviceId == groupInfo.masterDeviceId);
      }
    }

    deviceIdx++;
  }

  // Fill discovered groups (up to 2 for BLE packet size limits)
  for (int i = 0; i < std::min((size_t)2, discoveredGroups.size()); i++)
  {
    const auto &group = discoveredGroups[i];
    data.discoveredGroups[i].groupId = group.groupId;
    data.discoveredGroups[i].masterDeviceId = group.masterDeviceId;
    memcpy(data.discoveredGroups[i].masterMac, group.masterMac, 6);
    data.discoveredGroups[i].timeSinceLastSeen = now - group.lastSeen;
    data.discoveredGroups[i].isCurrentGroup = (group.groupId == groupInfo.groupId);
    data.discoveredGroups[i].canJoin = (groupInfo.groupId == 0 || group.groupId != groupInfo.groupId);
  }

  // Fill current group members (up to 4 for BLE packet size limits)
  int memberIdx = 0;
  for (const auto &memberPair : groupInfo.members)
  {
    if (memberIdx >= 4)
      break;

    const auto &member = memberPair.second;
    data.groupMembers[memberIdx].deviceId = member.deviceId;
    memcpy(data.groupMembers[memberIdx].mac, member.mac, 6);
    data.groupMembers[memberIdx].isGroupMaster = (member.deviceId == groupInfo.masterDeviceId);
    data.groupMembers[memberIdx].isThisDevice = (member.deviceId == ourDeviceId);

    // Try to find heartbeat info from discovered devices
    auto discoveredIt = discoveredDevices.find(memberPair.first);
    if (discoveredIt != discoveredDevices.end())
    {
      data.groupMembers[memberIdx].lastHeartbeat = now - discoveredIt->second.lastSeen;
    }
    else
    {
      data.groupMembers[memberIdx].lastHeartbeat = 0; // Unknown
    }

    memberIdx++;
  }

  return data;
}

void BLEManager::updateSyncData()
{
  if (!deviceConnected || !pSyncCharacteristic)
    return;

  BLESyncSendData syncData = prepareSyncData();
  pSyncCharacteristic->setValue((uint8_t *)&syncData, sizeof(syncData));
  pSyncCharacteristic->notify();
}

// Characteristic callback handlers
void BLEManager::handleModeWrite(BLECharacteristic *pCharacteristic)
{
  if (!app)
    return;

  std::string value = pCharacteristic->getValue();
  if (value.length() != sizeof(BLEModeData))
  {
    Serial.println("BLE Mode: Invalid data size");
    return;
  }

  BLEModeData *data = (BLEModeData *)value.data();
  Serial.printf("BLE Mode: Setting mode to %d\n", data->mode);

  // Set the mode in the application

  switch (data->mode)
  {
  case 0:
    app->enableNormalMode();
    break;
  case 1:
    app->enableTestMode();
    break;
  case 2:
    app->enableRemoteMode();
    break;
  case 3:
    app->enableOffMode();
    break;
  default:
    break;
  }
}

void BLEManager::handleEffectsWrite(BLECharacteristic *pCharacteristic)
{
  if (!app)
    return;

  std::string value = pCharacteristic->getValue();
  if (value.length() != sizeof(BLEEffectsData))
  {
    Serial.println("BLE Effects: Invalid data size");
    return;
  }

  BLEEffectsData *data = (BLEEffectsData *)value.data();
  Serial.println("BLE Effects: Updating effects");

  // Update indicators
  if (app->leftIndicatorEffect)
    app->leftIndicatorEffect->setActive(data->leftIndicator);
  if (app->rightIndicatorEffect)
    app->rightIndicatorEffect->setActive(data->rightIndicator);

  // Update headlight
  if (app->headlightEffect)
  {
    app->headlightEffect->setMode(data->headlightMode);
    app->headlightEffect->setSplit(data->headlightSplit);
    app->headlightEffect->setColor(data->headlightR, data->headlightG, data->headlightB);
  }

  // Update taillight
  if (app->taillightEffect)
  {
    app->taillightEffect->setMode(data->taillightMode);
    app->taillightEffect->setSplit(data->taillightSplit);
  }

  // Update other effects
  if (app->rgbEffect)
    app->rgbEffect->setActive(data->rgb);
  if (app->nightriderEffect)
    app->nightriderEffect->setActive(data->nightrider);
  if (app->policeEffect)
  {
    app->policeEffect->setActive(data->police);
    app->policeEffect->setMode(static_cast<PoliceMode>(data->policeMode));
  }
  if (app->pulseWaveEffect)
    app->pulseWaveEffect->setActive(data->pulseWave);
  if (app->auroraEffect)
    app->auroraEffect->setActive(data->aurora);
  if (app->solidColorEffect)
  {
    app->solidColorEffect->setActive(data->solidColor);
    app->solidColorEffect->setColorPreset(static_cast<SolidColorPreset>(data->solidColorPreset));
    app->solidColorEffect->setCustomColor(data->solidColorR, data->solidColorG, data->solidColorB);
  }
  if (app->colorFadeEffect)
    app->colorFadeEffect->setActive(data->colorFade);
  if (app->commitEffect)
    app->commitEffect->setActive(data->commit);

  if (app->serviceLightsEffect)
  {
    app->serviceLightsEffect->setActive(data->serviceLights);
    app->serviceLightsEffect->setMode(static_cast<ServiceLightsMode>(data->serviceLightsMode));
  }
}

void BLEManager::handleStripActiveWrite(BLECharacteristic *pCharacteristic)
{
  if (!app)
    return;

  std::string value = pCharacteristic->getValue();
  if (value.length() != sizeof(BLEStripActiveData))
  {
    Serial.println("BLE Strip Active: Invalid data size");
    return;
  }

  BLEStripActiveData *data = (BLEStripActiveData *)value.data();
  Serial.println("BLE Strip Active: Updating strip active");

  LEDStripManager *ledManager = LEDStripManager::getInstance();
  ledManager->setStripActive(LEDStripType::HEADLIGHT, data->headlight);
  ledManager->setStripActive(LEDStripType::TAILLIGHT, data->taillight);
  ledManager->setStripActive(LEDStripType::UNDERGLOW, data->underglow);
  ledManager->setStripActive(LEDStripType::INTERIOR, data->interior);

  BLEStripActiveData dataTX = prepareStripActiveData();
  pStripActiveCharacteristic->setValue((uint8_t *)&dataTX, sizeof(dataTX));
  pStripActiveCharacteristic->notify();
}

void BLEManager::handleSyncWrite(BLECharacteristic *pCharacteristic)
{
  if (!app)
    return;

  std::string value = pCharacteristic->getValue();
  if (value.length() != sizeof(BLESyncReceiveData))
  {
    Serial.println("BLE Sync: Invalid data size");
    return;
  }

  BLESyncReceiveData *data = (BLESyncReceiveData *)value.data();
  Serial.println("BLE Sync: Received sync command");

  // Handle sync mode changes
  SyncManager *syncMgr = SyncManager::getInstance();
  syncMgr->setSyncMode(static_cast<SyncMode>(data->mode));

  BLESyncSendData dataTX = prepareSyncData();
  pSyncCharacteristic->setValue((uint8_t *)&dataTX, sizeof(dataTX));
  pSyncCharacteristic->notify();
}

// BLE Server Callbacks
CarThingBLEServerCallbacks::CarThingBLEServerCallbacks(BLEManager *manager)
    : bleManager(manager)
{
}

void CarThingBLEServerCallbacks::onConnect(BLEServer *pServer)
{
  bleManager->deviceConnected = true;
  bleManager->connectionCount++;
  Serial.printf("BLE Client connected (count: %d)\n", bleManager->connectionCount);
}

void CarThingBLEServerCallbacks::onDisconnect(BLEServer *pServer)
{
  bleManager->deviceConnected = false;
  if (bleManager->connectionCount > 0)
  {
    bleManager->connectionCount--;
  }
  Serial.printf("BLE Client disconnected (count: %d)\n", bleManager->connectionCount);

  // Restart advertising
  pServer->startAdvertising();
}

// BLE Characteristic Callbacks
CarThingBLECharacteristicCallbacks::CarThingBLECharacteristicCallbacks(BLEManager *manager, const String &charName)
    : bleManager(manager), characteristicName(charName)
{
}

void CarThingBLECharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic)
{
  Serial.println("BLE Write to " + characteristicName);

  if (characteristicName == "Mode")
  {
    bleManager->handleModeWrite(pCharacteristic);
  }
  else if (characteristicName == "Effects")
  {
    bleManager->handleEffectsWrite(pCharacteristic);
  }
  else if (characteristicName == "Strip Active")
  {
    bleManager->handleStripActiveWrite(pCharacteristic);
  }
  else if (characteristicName == "Sync")
  {
    bleManager->handleSyncWrite(pCharacteristic);
  }
}

void CarThingBLECharacteristicCallbacks::onRead(BLECharacteristic *pCharacteristic)
{
  Serial.println("BLE Read from " + characteristicName);

  // Update characteristic value based on current state
  if (characteristicName == "Ping")
  {
    BLEPingData data = bleManager->preparePingData();
    pCharacteristic->setValue((uint8_t *)&data, sizeof(data));
  }
  else if (characteristicName == "Mode")
  {
    BLEModeData data = bleManager->prepareModeData();
    pCharacteristic->setValue((uint8_t *)&data, sizeof(data));
  }
  else if (characteristicName == "Effects")
  {
    BLEEffectsData data = bleManager->prepareEffectsData();
    pCharacteristic->setValue((uint8_t *)&data, sizeof(data));
  }
  else if (characteristicName == "Strip Active")
  {
    BLEStripActiveData data = bleManager->prepareStripActiveData();
    pCharacteristic->setValue((uint8_t *)&data, sizeof(data));
  }
  else if (characteristicName == "Sync")
  {
    BLESyncSendData data = bleManager->prepareSyncData();
    pCharacteristic->setValue((uint8_t *)&data, sizeof(data));
  }
}