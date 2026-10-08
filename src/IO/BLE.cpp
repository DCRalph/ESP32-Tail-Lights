#include "BLE.h"
#include "../Application.h"
#include "LED/LEDStripManager.h"
#include "TimeProfiler.h"
#include <esp_system.h>
#include "Battery.h"
#include "../MeshSupport.h"
#include <Mesh.h>

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

#if !BLE_TRANSPORT_AVAILABLE

BLEManager::BLEManager()
    : pServer(nullptr), pService(nullptr), pPingCharacteristic(nullptr), pModeCharacteristic(nullptr), pEffectsInfoCharacteristic(nullptr), pEffectsRequestCharacteristic(nullptr), pEffectsDataCharacteristic(nullptr), pEffectsCommandCharacteristic(nullptr), pStripActiveCharacteristic(nullptr), pSyncCharacteristic(nullptr), app(nullptr), deviceConnected(false), connectionCount(0), lastPingUpdate(0), lastSyncUpdate(0), lastEffectsStatePoll(0), selectedEffectsResource(EffectsResource::Catalog), selectedEffectsChunk(0), effectsStateRevision(0)
{
}

BLEManager::~BLEManager() {}

void BLEManager::begin()
{
  Serial.println("BLEManager: BLE transport unavailable on this target");
}

void BLEManager::loop() {}

void BLEManager::end() {}

bool BLEManager::isConnected()
{
  return false;
}

uint16_t BLEManager::getConnectionCount()
{
  return 0;
}

void BLEManager::setApplication(Application *application)
{
  app = application;
}

void BLEManager::updatePingData() {}

void BLEManager::updateSyncData() {}

void BLEManager::setupCharacteristics() {}

void BLEManager::setupCallbacks() {}

void BLEManager::queueWrite(BLECharacteristic *pCharacteristic) {}

void BLEManager::processPendingWrites() {}

void BLEManager::handleModeWrite(const std::string &value) {}

void BLEManager::handleEffectsRequestWrite(const std::string &value) {}

void BLEManager::handleEffectsCommandWrite(const std::string &value) {}

void BLEManager::handleStripActiveWrite(const std::string &value) {}

void BLEManager::handleSyncWrite(const std::string &value) {}

void BLEManager::updateReadValues() {}

BLEPingData BLEManager::preparePingData()
{
  return {};
}

BLEModeData BLEManager::prepareModeData()
{
  return {};
}

BLEStripActiveData BLEManager::prepareStripActiveData()
{
  return {};
}

BLESyncSendData BLEManager::prepareSyncData()
{
  return {};
}

String BLEManager::prepareEffectsInfo()
{
  return "{\"protocolVersion\":2,\"available\":false}";
}

String BLEManager::prepareEffectsDataChunk()
{
  return "{}";
}

void BLEManager::refreshEffectsStateCache(bool notifyInfo) {}

void BLEManager::updateEffectsInfoCharacteristic(bool notify) {}

void BLEManager::updateEffectsDataCharacteristic(bool notify) {}

void BLEManager::updateEffectsCommandResponse(const String &response, bool notify) {}

#else

BLEManager::BLEManager()
    : pServer(nullptr), pService(nullptr), app(nullptr), deviceConnected(false), connectionCount(0), lastPingUpdate(0), lastSyncUpdate(0), lastEffectsStatePoll(0), selectedEffectsResource(EffectsResource::Catalog), selectedEffectsChunk(0), effectsStateRevision(0)
{
  // Initialize characteristic pointers to nullptr
  pPingCharacteristic = nullptr;
  pModeCharacteristic = nullptr;
  pEffectsInfoCharacteristic = nullptr;
  pEffectsRequestCharacteristic = nullptr;
  pEffectsDataCharacteristic = nullptr;
  pEffectsCommandCharacteristic = nullptr;
  pStripActiveCharacteristic = nullptr;
  pSyncCharacteristic = nullptr;
  lastEffectsCommandResponse = "{\"ok\":true,\"message\":\"ready\"}";
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
  refreshEffectsStateCache(false);
  updateEffectsInfoCharacteristic(false);
  updateEffectsDataCharacteristic(false);
  updateEffectsCommandResponse(lastEffectsCommandResponse, false);
  updateReadValues();

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

  // Effects metadata and data transport
  pEffectsInfoCharacteristic = pService->createCharacteristic(
      EFFECTS_INFO_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pEffectsInfoCharacteristic->addDescriptor(new BLE2902());

  pEffectsRequestCharacteristic = pService->createCharacteristic(
      EFFECTS_REQUEST_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_WRITE);

  pEffectsDataCharacteristic = pService->createCharacteristic(
      EFFECTS_DATA_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pEffectsDataCharacteristic->addDescriptor(new BLE2902());

  pEffectsCommandCharacteristic = pService->createCharacteristic(
      EFFECTS_COMMAND_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pEffectsCommandCharacteristic->addDescriptor(new BLE2902());

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
  pEffectsInfoCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Effects Info"));
  pEffectsRequestCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Effects Request"));
  pEffectsDataCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Effects Data"));
  pEffectsCommandCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Effects Command"));
  pStripActiveCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Strip Active"));
  pSyncCharacteristic->setCallbacks(new CarThingBLECharacteristicCallbacks(this, "Sync"));
}

void BLEManager::loop()
{
  uint32_t now = millis();

  processPendingWrites();

  if (!deviceConnected)
  {
    return;
  }

  // Update ping data every 1000ms
  if (now - lastPingUpdate > 1000)
  {
    updatePingData();
    updateReadValues();
    lastPingUpdate = now;
  }

  // Update sync data every 2000ms (less frequent to avoid overwhelming BLE)
  if (now - lastSyncUpdate > 2000)
  {
    updateSyncData();
    lastSyncUpdate = now;
  }

  if (now - lastEffectsStatePoll > 500)
  {
    refreshEffectsStateCache(true);
    lastEffectsStatePoll = now;
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

String BLEManager::prepareEffectsInfo()
{
  DynamicJsonDocument doc(1024);
  JsonObject root = doc.to<JsonObject>();
  root["protocolVersion"] = BLE_EFFECTS_PROTOCOL_VERSION;
  root["schemaHash"] = BLEEffectsCatalog::getCatalogSchemaHash();
  root["chunkSize"] = BLE_EFFECTS_CHUNK_SIZE;
  root["catalogChunks"] = BLEEffectsCatalog::getCatalogChunkCount();
  root["stateChunks"] = BLEEffectsCatalog::getChunkCountForPayload(cachedEffectsStateJson);
  root["stateRevision"] = effectsStateRevision;

  size_t effectCount = 0;
  BLEEffectsCatalog::getEffects(effectCount);
  root["effectCount"] = effectCount;

  String json;
  serializeJson(doc, json);
  return json;
}

String BLEManager::prepareEffectsDataChunk()
{
  if (selectedEffectsResource == EffectsResource::Catalog)
  {
    return BLEEffectsCatalog::getCatalogChunk(selectedEffectsChunk);
  }

  return BLEEffectsCatalog::getPayloadChunk(cachedEffectsStateJson, selectedEffectsChunk);
}

void BLEManager::refreshEffectsStateCache(bool notifyInfo)
{
  if (!app)
  {
    return;
  }

  String nextStateJson = BLEEffectsCatalog::buildStateJson(*app);
  if (nextStateJson == cachedEffectsStateJson)
  {
    return;
  }

  cachedEffectsStateJson = nextStateJson;
  ++effectsStateRevision;
  updateEffectsInfoCharacteristic(notifyInfo);

  if (selectedEffectsResource == EffectsResource::State)
  {
    updateEffectsDataCharacteristic(notifyInfo);
  }
}

void BLEManager::updateEffectsInfoCharacteristic(bool notify)
{
  if (!pEffectsInfoCharacteristic)
  {
    return;
  }

  lastEffectsInfoJson = prepareEffectsInfo();
  pEffectsInfoCharacteristic->setValue(lastEffectsInfoJson.c_str());
  if (notify && deviceConnected)
  {
    pEffectsInfoCharacteristic->notify();
  }
}

void BLEManager::updateEffectsDataCharacteristic(bool notify)
{
  if (!pEffectsDataCharacteristic)
  {
    return;
  }

  String payload = prepareEffectsDataChunk();
  pEffectsDataCharacteristic->setValue(payload.c_str());
  if (notify && deviceConnected)
  {
    pEffectsDataCharacteristic->notify();
  }
}

void BLEManager::updateEffectsCommandResponse(const String &response, bool notify)
{
  lastEffectsCommandResponse = response;

  if (!pEffectsCommandCharacteristic)
  {
    return;
  }

  pEffectsCommandCharacteristic->setValue(lastEffectsCommandResponse.c_str());
  if (notify && deviceConnected)
  {
    pEffectsCommandCharacteristic->notify();
  }
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
  data.mode = toLegacySyncModeValue(syncMgr->getSyncMode());
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
    copyTransportAddressToMac(device.address, data.discoveredDevices[deviceIdx].mac);
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
    copyTransportAddressToMac(group.masterAddress, data.discoveredGroups[i].masterMac);
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
    copyTransportAddressToMac(member.address, data.groupMembers[memberIdx].mac);
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

void BLEManager::updateReadValues()
{
  if (pModeCharacteristic)
  {
    BLEModeData modeData = prepareModeData();
    pModeCharacteristic->setValue((uint8_t *)&modeData, sizeof(modeData));
  }

  if (pStripActiveCharacteristic)
  {
    BLEStripActiveData stripData = prepareStripActiveData();
    pStripActiveCharacteristic->setValue((uint8_t *)&stripData, sizeof(stripData));
  }
}

// Called on the BLE task: copy the value and return, everything else happens in loop()
void BLEManager::queueWrite(BLECharacteristic *pCharacteristic)
{
  std::lock_guard<std::mutex> lock(pendingWritesMutex);
  pendingWrites.push_back({pCharacteristic, pCharacteristic->getValue()});
}

void BLEManager::processPendingWrites()
{
  std::vector<PendingWrite> writes;
  {
    std::lock_guard<std::mutex> lock(pendingWritesMutex);
    writes.swap(pendingWrites);
  }

  for (const auto &write : writes)
  {
    if (write.characteristic == pModeCharacteristic)
      handleModeWrite(write.value);
    else if (write.characteristic == pEffectsRequestCharacteristic)
      handleEffectsRequestWrite(write.value);
    else if (write.characteristic == pEffectsCommandCharacteristic)
      handleEffectsCommandWrite(write.value);
    else if (write.characteristic == pStripActiveCharacteristic)
      handleStripActiveWrite(write.value);
    else if (write.characteristic == pSyncCharacteristic)
      handleSyncWrite(write.value);
  }
}

// Characteristic write handlers (main task only)
void BLEManager::handleModeWrite(const std::string &value)
{
  if (!app)
    return;

  if (value.length() != sizeof(BLEModeData))
  {
    Serial.println("BLE Mode: Invalid data size");
    return;
  }

  const BLEModeData *data = (const BLEModeData *)value.data();
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

  updateReadValues();
  pModeCharacteristic->notify();
}

void BLEManager::handleEffectsRequestWrite(const std::string &value)
{
  if (value.empty())
  {
    Serial.println("BLE Effects Request: Empty payload");
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, value);
  if (error)
  {
    Serial.println("BLE Effects Request: Invalid JSON");
    return;
  }

  const char *resource = doc["resource"] | "catalog";
  selectedEffectsResource = (String(resource) == "state") ? EffectsResource::State : EffectsResource::Catalog;
  selectedEffectsChunk = doc["chunk"] | 0;

  Serial.printf("BLE Effects Request: %s chunk %u\n", resource, selectedEffectsChunk);
  updateEffectsDataCharacteristic(true);
}

void BLEManager::handleEffectsCommandWrite(const std::string &value)
{
  if (!app)
  {
    updateEffectsCommandResponse("{\"ok\":false,\"error\":\"application unavailable\"}", true);
    return;
  }

  if (value.empty())
  {
    updateEffectsCommandResponse("{\"ok\":false,\"error\":\"empty payload\"}", true);
    return;
  }

  String response;
  bool ok = BLEEffectsCatalog::applyCommand(*app, String(value.c_str()), response);
  refreshEffectsStateCache(true);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, response);
  if (!error)
  {
    doc["stateRevision"] = effectsStateRevision;
    response = "";
    serializeJson(doc, response);
  }

  updateEffectsCommandResponse(response, true);
  if (ok && selectedEffectsResource == EffectsResource::State)
  {
    updateEffectsDataCharacteristic(true);
  }
}

void BLEManager::handleStripActiveWrite(const std::string &value)
{
  if (!app)
    return;

  if (value.length() != sizeof(BLEStripActiveData))
  {
    Serial.println("BLE Strip Active: Invalid data size");
    return;
  }

  const BLEStripActiveData *data = (const BLEStripActiveData *)value.data();
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

void BLEManager::handleSyncWrite(const std::string &value)
{
  if (!app)
    return;

  if (value.length() != sizeof(BLESyncReceiveData))
  {
    Serial.println("BLE Sync: Invalid data size");
    return;
  }

  const BLESyncReceiveData *data = (const BLESyncReceiveData *)value.data();
  Serial.println("BLE Sync: Received sync command");

  // Handle sync mode changes
  SyncManager *syncMgr = SyncManager::getInstance();

  if (data->command == 0)
  {
    syncMgr->setSyncMode(fromLegacySyncModeValue(data->mode));
  }
  else if (data->command == 1) // these are wip
  {
    // syncMgr->joinGroup(data->groupId);
  }
  else if (data->command == 2)
  {
    // syncMgr->createGroup(data->groupId);
  }
  else if (data->command == 3)
  {
    // syncMgr->leaveGroup(data->groupId);
  }

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
  bleManager->queueWrite(pCharacteristic);
}

#endif