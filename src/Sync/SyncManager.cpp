#include "SyncManager.h"
#include <WiFi.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include "IO/GPIO.h"

#ifdef ENABLE_SYNC

// Static instance
SyncManager *SyncManager::getInstance()
{
  static SyncManager instance;
  return &instance;
}

SyncManager::SyncManager()
{
  // Initialize state
  isMasterDevice = false;
  syncActive = false;
  timeIsSynced = false;
  ourTimeOffset = 0;
  masterSyncTime = 0;
  lastTimeSyncRequest = 0;
  currentGroupId = 0;

  // Initialize timing
  lastHeartbeat = 0;
  lastDeviceInfo = 0;
  lastMasterCheck = 0;
  lastCleanup = 0;
  lastTimeSync = 0;
  lastPrintTime = 0;

  // Generate unique device ID and priority
  randomSeed(ESP.getEfuseMac());
  ourDeviceId = generateDeviceId();
  ourPriority = random(0, UINT32_MAX);

  // Initialize auto-join configuration
  autoJoinEnabled = false;
  autoJoinTimeout = 10000; // 10 seconds default
  autoJoinStartTime = 0;

  Serial.println("SyncManager: Initialized");
  Serial.println("  Device ID: 0x" + String(ourDeviceId, HEX));
  Serial.println("  Priority: " + String(ourPriority));
}

SyncManager::~SyncManager()
{
  // Clean up if needed
}

void SyncManager::begin()
{
  // Register for sync packets
  wireless.addOnReceiveFor(SYNC_MSG_TYPE, [this](fullPacket *fp)
                           { this->handleSyncPacket(fp); });

  Serial.println("SyncManager: Started - waiting for network...");
}

void SyncManager::loop()
{
  uint32_t currentTime = millis();

  // Always send heartbeat to discover other devices
  if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL)
  {
    sendHeartbeat();
    lastHeartbeat = currentTime;
  }

  // Send detailed device info periodically
  if (currentTime - lastDeviceInfo >= DEVICE_INFO_INTERVAL)
  {
    sendDeviceInfo();
    lastDeviceInfo = currentTime;
  }

  // Only perform full sync operations if we have other devices
  if (knownDevices.size() > 0)
  {
    syncActive = true;

    // Check master status
    if (currentTime - lastMasterCheck >= MASTER_CHECK_INTERVAL)
    {
      checkMasterStatus();
      lastMasterCheck = currentTime;
    }

    // Synchronize time if we're not master
    if (!isMasterDevice && (currentTime - lastTimeSync >= TIME_SYNC_INTERVAL))
    {
      requestTimeSync();
      lastTimeSync = currentTime;
    }

    // Clean up devices that haven't been seen
    if (currentTime - lastCleanup >= CLEANUP_INTERVAL)
    {
      cleanupOldDevices();
      lastCleanup = currentTime;
    }
  }
  else
  {
    syncActive = false;
    timeIsSynced = false;
  }

  // Check for auto-join opportunities
  if (autoJoinEnabled)
  {
    checkAutoJoin();
  }

  // Debug output
  // if (currentTime - lastPrintTime >= 5000)
  // {
  //   lastPrintTime = currentTime;
  // printDeviceInfo();
  // printNetworkStatus();
  // printGroupInfo();
  // }
}

// Group management
void SyncManager::createGroup(uint32_t groupId)
{
  if (groupId == 0)
  {
    groupId = generateGroupId();
  }

  currentGroupId = groupId;
  becomeMaster(); // Creator becomes master

  Serial.println("SyncManager: Created group 0x" + String(groupId, HEX));
}

void SyncManager::joinGroup(uint32_t groupId)
{
  if (currentGroupId != groupId)
  {
    currentGroupId = groupId;
    isMasterDevice = false; // Can't be master when joining

    // Send join request
    data_packet pkt;
    pkt.type = SYNC_MSG_TYPE;
    pkt.data[0] = SYNC_GROUP_JOIN;
    memcpy(&pkt.data[1], &ourDeviceId, sizeof(ourDeviceId));
    memcpy(&pkt.data[5], &groupId, sizeof(groupId));
    pkt.len = 1 + sizeof(ourDeviceId) + sizeof(groupId);

    wireless.send(&pkt, (uint8_t *)BROADCAST_MAC);

    Serial.println("SyncManager: Joined group 0x" + String(groupId, HEX));
  }
}

void SyncManager::leaveGroup()
{
  if (currentGroupId != 0)
  {
    Serial.println("SyncManager: Left group 0x" + String(currentGroupId, HEX));
    currentGroupId = 0;

    if (isMasterDevice)
    {
      resignMaster();
    }

    // Clear known devices from old group
    knownDevices.clear();
    syncActive = false;
    timeIsSynced = false;
  }
}

uint32_t SyncManager::getGroupId() const
{
  return currentGroupId;
}

// Auto-join functionality
void SyncManager::enableAutoJoin(bool enabled)
{
  autoJoinEnabled = enabled;
  if (enabled && autoJoinStartTime == 0)
  {
    autoJoinStartTime = millis();
    Serial.println("SyncManager: Auto-join enabled - searching for groups...");
  }
  else if (!enabled)
  {
    autoJoinStartTime = 0;
    Serial.println("SyncManager: Auto-join disabled");
  }
}

void SyncManager::setAutoJoinTimeout(uint32_t timeoutMs)
{
  autoJoinTimeout = timeoutMs;
}

bool SyncManager::isAutoJoinEnabled() const
{
  return autoJoinEnabled;
}

// Time synchronization
void SyncManager::requestTimeSync()
{
  if (knownDevices.empty())
    return;

  lastTimeSyncRequest = millis();

  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_TIME_REQUEST;
  memcpy(&pkt.data[1], &ourDeviceId, sizeof(ourDeviceId));
  memcpy(&pkt.data[5], &lastTimeSyncRequest, sizeof(lastTimeSyncRequest));
  pkt.len = 1 + sizeof(ourDeviceId) + sizeof(lastTimeSyncRequest);

  wireless.send(&pkt, (uint8_t *)BROADCAST_MAC);
}

uint32_t SyncManager::getSyncedTime() const
{
  if (!timeIsSynced)
    return millis();

  return millis() + ourTimeOffset;
}

bool SyncManager::isTimeSynced() const
{
  return timeIsSynced;
}

int32_t SyncManager::getTimeOffset() const
{
  return ourTimeOffset;
}

// Device management
bool SyncManager::isSyncing() const
{
  return syncActive;
}

bool SyncManager::isMaster() const
{
  return isMasterDevice;
}

uint32_t SyncManager::getDeviceId() const
{
  return ourDeviceId;
}

size_t SyncManager::getDeviceCount() const
{
  return knownDevices.size();
}

std::vector<DeviceInfo> SyncManager::getKnownDevices() const
{
  std::vector<DeviceInfo> devices;
  for (const auto &pair : knownDevices)
  {
    devices.push_back(pair.second);
  }
  return devices;
}

SyncNetworkInfo SyncManager::getNetworkInfo() const
{
  SyncNetworkInfo info;
  info.groupId = currentGroupId;
  info.syncedTime = getSyncedTime();
  info.deviceCount = knownDevices.size();
  info.isTimeSynced = timeIsSynced;
  info.avgTimeOffset = 0;

  // Find master device
  info.masterDeviceId = ourDeviceId; // Default to us
  for (const auto &pair : knownDevices)
  {
    if (pair.second.isMaster)
    {
      info.masterDeviceId = pair.second.deviceId;
      break;
    }
  }

  // Calculate average time offset
  if (knownDevices.size() > 0)
  {
    int32_t totalOffset = 0;
    size_t count = 0;
    for (const auto &pair : knownDevices)
    {
      totalOffset += pair.second.timeOffset;
      count++;
    }
    info.avgTimeOffset = count > 0 ? (totalOffset / count) : 0;
  }

  return info;
}

// Callbacks
void SyncManager::setDeviceJoinCallback(std::function<void(const DeviceInfo &)> callback)
{
  deviceJoinCallback = callback;
}

void SyncManager::setDeviceLeaveCallback(std::function<void(uint32_t deviceId)> callback)
{
  deviceLeaveCallback = callback;
}

void SyncManager::setMasterChangeCallback(std::function<void(uint32_t newMasterDeviceId)> callback)
{
  masterChangeCallback = callback;
}

void SyncManager::setTimeSyncCallback(std::function<void(uint32_t syncedTime)> callback)
{
  timeSyncCallback = callback;
}

// Core sync handlers
void SyncManager::handleSyncPacket(fullPacket *fp)
{
  if (fp->p.len < 1)
    return; // Invalid packet

  uint8_t subtype = fp->p.data[0];
  std::string macStr = macToString(fp->mac);

  // Update device info from packet
  updateDeviceFromPacket(macStr, fp);

  // Process by subtype
  switch (subtype)
  {
  case SYNC_HEARTBEAT:
    processHeartbeat(fp);
    break;
  case SYNC_MASTER_ANNOUNCE:
    processMasterAnnounce(fp);
    break;
  case SYNC_TIME_REQUEST:
    processTimeRequest(fp);
    break;
  case SYNC_TIME_RESPONSE:
    processTimeResponse(fp);
    break;
  case SYNC_GROUP_JOIN:
    processGroupJoin(fp);
    break;
  case SYNC_GROUP_ANNOUNCE:
    processGroupAnnounce(fp);
    break;
  case SYNC_DEVICE_INFO:
    processDeviceInfo(fp);
    break;
  case SYNC_MASTER_REQUEST:
    // If we're master, announce it
    if (isMasterDevice)
    {
      data_packet pkt;
      pkt.type = SYNC_MSG_TYPE;
      pkt.data[0] = SYNC_MASTER_ANNOUNCE;
      memcpy(&pkt.data[1], &ourDeviceId, sizeof(ourDeviceId));
      memcpy(&pkt.data[5], &ourPriority, sizeof(ourPriority));
      pkt.len = 1 + sizeof(ourDeviceId) + sizeof(ourPriority);
      wireless.send(&pkt, (uint8_t *)BROADCAST_MAC);
    }
    break;
  }
}

void SyncManager::processHeartbeat(fullPacket *fp)
{
  if (fp->p.len < 1 + sizeof(uint32_t) + sizeof(uint32_t))
    return;

  uint32_t deviceId, priority;
  memcpy(&deviceId, &fp->p.data[1], sizeof(deviceId));
  memcpy(&priority, &fp->p.data[5], sizeof(priority));

  std::string macStr = macToString(fp->mac);
  if (knownDevices.find(macStr) != knownDevices.end())
  {
    knownDevices[macStr].deviceId = deviceId;
    knownDevices[macStr].priority = priority;
  }
}

void SyncManager::processMasterAnnounce(fullPacket *fp)
{
  if (fp->p.len < 1 + sizeof(uint32_t) + sizeof(uint32_t))
    return;

  uint32_t masterDeviceId, masterPriority;
  memcpy(&masterDeviceId, &fp->p.data[1], sizeof(masterDeviceId));
  memcpy(&masterPriority, &fp->p.data[5], sizeof(masterPriority));

  std::string macStr = macToString(fp->mac);

  // Accept if they have higher priority
  if (masterPriority >= ourPriority)
  {
    // Clear all master flags
    for (auto &device : knownDevices)
    {
      device.second.isMaster = false;
    }

    // Set new master
    if (knownDevices.find(macStr) != knownDevices.end())
    {
      knownDevices[macStr].isMaster = true;
      knownDevices[macStr].deviceId = masterDeviceId;
    }

    // We're no longer master
    if (isMasterDevice)
    {
      isMasterDevice = false;
      Serial.println("SyncManager: No longer master - accepted device 0x" + String(masterDeviceId, HEX));

      if (masterChangeCallback)
      {
        masterChangeCallback(masterDeviceId);
      }
    }
  }
  else if (isMasterDevice)
  {
    // We have higher priority, reaffirm mastery
    becomeMaster();
  }
}

void SyncManager::processTimeRequest(fullPacket *fp)
{
  if (!isMasterDevice)
    return; // Only master responds to time requests

  if (fp->p.len < 1 + sizeof(uint32_t))
    return;

  uint32_t requestorId;
  memcpy(&requestorId, &fp->p.data[1], sizeof(requestorId));

  respondToTimeRequest(fp->mac);
}

void SyncManager::processTimeResponse(fullPacket *fp)
{
  if (isMasterDevice)
    return; // Master doesn't need time sync

  if (fp->p.len < 1 + sizeof(uint32_t) + sizeof(uint32_t))
    return;

  uint32_t masterTime, roundTripTime;
  memcpy(&masterTime, &fp->p.data[1], sizeof(masterTime));
  memcpy(&roundTripTime, &fp->p.data[5], sizeof(roundTripTime));

  // Calculate time offset (simple approach)
  uint32_t currentTime = millis();
  uint32_t estimatedNetworkDelay = (currentTime - lastTimeSyncRequest) / 2;
  int32_t newOffset = (int32_t)(masterTime + estimatedNetworkDelay) - (int32_t)currentTime;

  // Apply simple filtering to avoid large jumps
  if (!timeIsSynced)
  {
    ourTimeOffset = newOffset;
    timeIsSynced = true;
  }
  else
  {
    // Weighted average with previous offset
    ourTimeOffset = (ourTimeOffset * 3 + newOffset) / 4;
  }

  masterSyncTime = masterTime;

  Serial.println("SyncManager: Time synced, offset: " + String(ourTimeOffset) + "ms");

  if (timeSyncCallback)
  {
    timeSyncCallback(getSyncedTime());
  }
}

void SyncManager::processGroupJoin(fullPacket *fp)
{
  if (!isMasterDevice)
    return; // Only master handles join requests

  if (fp->p.len < 1 + sizeof(uint32_t) + sizeof(uint32_t))
    return;

  uint32_t deviceId, groupId;
  memcpy(&deviceId, &fp->p.data[1], sizeof(deviceId));
  memcpy(&groupId, &fp->p.data[5], sizeof(groupId));

  // Accept if it's our group
  if (groupId == currentGroupId)
  {
    std::string macStr = macToString(fp->mac);
    if (knownDevices.find(macStr) != knownDevices.end())
    {
      knownDevices[macStr].groupId = groupId;
      knownDevices[macStr].deviceId = deviceId;

      Serial.println("SyncManager: Device 0x" + String(deviceId, HEX) + " joined group");

      if (deviceJoinCallback)
      {
        deviceJoinCallback(knownDevices[macStr]);
      }
    }

    // Send group announcement
    broadcastGroupInfo();
  }
}

void SyncManager::processGroupAnnounce(fullPacket *fp)
{
  if (fp->p.len < 1 + sizeof(uint32_t))
    return;

  uint32_t groupId;
  memcpy(&groupId, &fp->p.data[1], sizeof(groupId));

  // Update device's group info
  std::string macStr = macToString(fp->mac);
  if (knownDevices.find(macStr) != knownDevices.end())
  {
    knownDevices[macStr].groupId = groupId;
  }
}

void SyncManager::processDeviceInfo(fullPacket *fp)
{
  if (fp->p.len < 1 + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t))
    return;

  uint32_t deviceId, priority, groupId;
  memcpy(&deviceId, &fp->p.data[1], sizeof(deviceId));
  memcpy(&priority, &fp->p.data[5], sizeof(priority));
  memcpy(&groupId, &fp->p.data[9], sizeof(groupId));

  std::string macStr = macToString(fp->mac);
  if (knownDevices.find(macStr) != knownDevices.end())
  {
    knownDevices[macStr].deviceId = deviceId;
    knownDevices[macStr].priority = priority;
    knownDevices[macStr].groupId = groupId;
  }
}

// Periodic tasks
void SyncManager::sendHeartbeat()
{
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_HEARTBEAT;
  memcpy(&pkt.data[1], &ourDeviceId, sizeof(ourDeviceId));
  memcpy(&pkt.data[5], &ourPriority, sizeof(ourPriority));
  pkt.len = 1 + sizeof(ourDeviceId) + sizeof(ourPriority);

  wireless.send(&pkt, (uint8_t *)BROADCAST_MAC);
}

void SyncManager::sendDeviceInfo()
{
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_DEVICE_INFO;
  memcpy(&pkt.data[1], &ourDeviceId, sizeof(ourDeviceId));
  memcpy(&pkt.data[5], &ourPriority, sizeof(ourPriority));
  memcpy(&pkt.data[9], &currentGroupId, sizeof(currentGroupId));
  pkt.len = 1 + sizeof(ourDeviceId) + sizeof(ourPriority) + sizeof(currentGroupId);

  wireless.send(&pkt, (uint8_t *)BROADCAST_MAC);
}

void SyncManager::checkMasterStatus()
{
  bool masterExists = false;

  // Check if any device claims to be master
  for (auto &device : knownDevices)
  {
    if (device.second.isMaster)
    {
      masterExists = true;
      break;
    }
  }

  // If no master exists, start election
  if (!masterExists && !isMasterDevice)
  {
    electMaster();
  }
}

void SyncManager::cleanupOldDevices()
{
  uint32_t currentTime = millis();
  std::vector<std::string> devicesToRemove;

  // Find devices that haven't been seen
  for (auto &device : knownDevices)
  {
    if (currentTime - device.second.lastSeen > DEVICE_TIMEOUT)
    {
      devicesToRemove.push_back(device.first);
    }
  }

  // Remove dead devices
  for (const auto &macStr : devicesToRemove)
  {
    bool wasMaster = knownDevices[macStr].isMaster;
    uint32_t deviceId = knownDevices[macStr].deviceId;

    Serial.println("SyncManager: Device 0x" + String(deviceId, HEX) + " timed out");

    knownDevices.erase(macStr);

    if (deviceLeaveCallback)
    {
      deviceLeaveCallback(deviceId);
    }

    // If master disappeared, elect new one
    if (wasMaster)
    {
      electMaster();
    }
  }

  // Update sync status
  if (knownDevices.empty())
  {
    syncActive = false;
    timeIsSynced = false;
  }
}

void SyncManager::electMaster()
{
  // Find device with highest priority
  uint32_t highestPriority = ourPriority;
  bool weAreHighest = true;

  for (const auto &device : knownDevices)
  {
    if (device.second.priority > highestPriority)
    {
      highestPriority = device.second.priority;
      weAreHighest = false;
    }
  }

  // If we have highest priority, become master
  if (weAreHighest)
  {
    becomeMaster();
  }
}

// Master operations
void SyncManager::becomeMaster()
{
  if (!isMasterDevice)
  {
    isMasterDevice = true;
    timeIsSynced = true; // Master is always time-synced
    ourTimeOffset = 0;   // Master has no offset

    Serial.println("SyncManager: This device is now the master!");

    // Announce mastery
    data_packet pkt;
    pkt.type = SYNC_MSG_TYPE;
    pkt.data[0] = SYNC_MASTER_ANNOUNCE;
    memcpy(&pkt.data[1], &ourDeviceId, sizeof(ourDeviceId));
    memcpy(&pkt.data[5], &ourPriority, sizeof(ourPriority));
    pkt.len = 1 + sizeof(ourDeviceId) + sizeof(ourPriority);

    wireless.send(&pkt, (uint8_t *)BROADCAST_MAC);

    if (masterChangeCallback)
    {
      masterChangeCallback(ourDeviceId);
    }
  }
}

void SyncManager::resignMaster()
{
  if (isMasterDevice)
  {
    isMasterDevice = false;
    Serial.println("SyncManager: Resigned as master");

    // Trigger election
    electMaster();
  }
}

void SyncManager::broadcastGroupInfo()
{
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_GROUP_ANNOUNCE;
  memcpy(&pkt.data[1], &currentGroupId, sizeof(currentGroupId));
  pkt.len = 1 + sizeof(currentGroupId);

  wireless.send(&pkt, (uint8_t *)BROADCAST_MAC);
}

void SyncManager::respondToTimeRequest(const uint8_t *requestorMac)
{
  uint32_t currentTime = getSyncedTime();
  uint32_t roundTripEstimate = 0; // Could implement RTT measurement

  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_TIME_RESPONSE;
  memcpy(&pkt.data[1], &currentTime, sizeof(currentTime));
  memcpy(&pkt.data[5], &roundTripEstimate, sizeof(roundTripEstimate));
  pkt.len = 1 + sizeof(currentTime) + sizeof(roundTripEstimate);

  wireless.send(&pkt, (uint8_t *)requestorMac);
}

// Utility functions
bool SyncManager::macEqual(const uint8_t *mac1, const uint8_t *mac2)
{
  return memcmp(mac1, mac2, 6) == 0;
}

const uint8_t *SyncManager::getOurMac()
{
  static uint8_t ourMac[6] = {0};
  static bool macSet = false;

  if (!macSet)
  {
    WiFi.macAddress(ourMac);
    macSet = true;
  }

  return ourMac;
}

std::string SyncManager::macToString(const uint8_t *mac)
{
  std::string macStr = "";
  for (int i = 0; i < 6; i++)
  {
    char buf[3];
    sprintf(buf, "%02X", mac[i]);
    macStr += buf;
  }
  return macStr;
}

uint32_t SyncManager::generateDeviceId()
{
  // Generate based on MAC and some randomness
  uint64_t macValue = ESP.getEfuseMac();
  return (uint32_t)(macValue ^ random(0, UINT32_MAX));
}

uint32_t SyncManager::generateGroupId()
{
  return random(1, UINT32_MAX); // Avoid 0 which means no group
}

void SyncManager::updateDeviceFromPacket(const std::string &macStr, fullPacket *fp)
{
  if (knownDevices.find(macStr) == knownDevices.end())
  {
    // New device
    DeviceInfo info;
    memcpy(info.mac, fp->mac, 6);
    info.deviceId = 0; // Will be updated by specific packet handlers
    info.lastSeen = millis();
    info.priority = 0;
    info.isMaster = false;
    info.syncedTime = 0;
    info.groupId = 0;
    info.timeOffset = 0;

    knownDevices[macStr] = info;

    Serial.println("SyncManager: Discovered new device: " + String(macStr.c_str()));
  }
  else
  {
    // Update existing device
    knownDevices[macStr].lastSeen = millis();
  }
}

// Debug and monitoring
void SyncManager::printDeviceInfo()
{
  std::stringstream ss;
  ss << "\n=== Device List ===\n";

  if (knownDevices.size() > 0)
  {
    ss << "Known Devices (" << knownDevices.size() << "):\n";
    ss << "+----------+-------------------+----------+--------+----------+--------+\n";
    ss << "| DeviceID | MAC Address       | Priority | Master | Group    | RSSI   |\n";
    ss << "+----------+-------------------+----------+--------+----------+--------+\n";

    for (const auto &device : knownDevices)
    {
      ss << "| " << std::setw(8) << std::hex << device.second.deviceId;
      ss << " | ";
      for (int i = 0; i < 6; i++)
      {
        char buf[3];
        sprintf(buf, "%02X", device.second.mac[i]);
        ss << buf;
        if (i < 5)
          ss << ":";
      }
      ss << " | " << std::setw(8) << std::dec << device.second.priority;
      ss << " | " << std::setw(6) << (device.second.isMaster ? "YES" : "NO");
      ss << " | " << std::setw(8) << std::hex << device.second.groupId;
      ss << " | " << std::setw(6) << std::dec << -1 << " |\n";
    }
    ss << "+----------+-------------------+----------+--------+----------+--------+\n";
  }
  else
  {
    ss << "No devices discovered yet.\n";
  }

  Serial.println(ss.str().c_str());
}

void SyncManager::printNetworkStatus()
{
  std::stringstream ss;
  ss << "\n=== Sync Network Status ===\n";

  // Our device info
  ss << "This Device:\n";
  ss << "  Device ID: 0x" << std::hex << ourDeviceId << std::dec << "\n";
  ss << "  MAC: " << WiFi.macAddress().c_str() << "\n";
  ss << "  Priority: " << ourPriority << "\n";
  ss << "  Role: " << (isMasterDevice ? "MASTER" : "SLAVE") << "\n";
  ss << "  Group: 0x" << std::hex << currentGroupId << std::dec << "\n";
  ss << "  Sync Active: " << (syncActive ? "YES" : "NO") << "\n";
  ss << "  Time Synced: " << (timeIsSynced ? "YES" : "NO") << "\n";

  if (timeIsSynced)
  {
    ss << "  Time Offset: " << ourTimeOffset << "ms\n";
    ss << "  Synced Time: " << getSyncedTime() << "\n";
  }

  ss << "\nNetwork:\n";
  ss << "  Known Devices: " << knownDevices.size() << "\n";

  // Find master
  for (const auto &device : knownDevices)
  {
    if (device.second.isMaster)
    {
      ss << "  Master: 0x" << std::hex << device.second.deviceId << std::dec << "\n";
      break;
    }
  }

  if (knownDevices.empty())
  {
    ss << "  Status: Searching for devices...\n";
  }

  Serial.println(ss.str().c_str());
}

void SyncManager::printGroupInfo()
{
  std::stringstream ss;
  ss << "\n=== Group Info [" << millis() / 1000 << "s] ===\n";

  // Our device summary
  ss << "Device: 0x" << std::hex << ourDeviceId << std::dec;
  ss << " | Group: 0x" << std::hex << currentGroupId << std::dec;
  ss << " | " << (isMasterDevice ? "MASTER" : "SLAVE");
  ss << " | Priority: " << ourPriority;

  if (timeIsSynced)
  {
    ss << " | TimeSync: +" << ourTimeOffset << "ms";
  }
  else
  {
    ss << " | TimeSync: NO";
  }
  ss << "\n";

  // Network status
  if (knownDevices.size() > 0)
  {
    ss << "Network: " << knownDevices.size() << " devices | ";

    // Find and show master
    bool masterFound = false;
    for (const auto &device : knownDevices)
    {
      if (device.second.isMaster)
      {
        ss << "Master: 0x" << std::hex << device.second.deviceId << std::dec;
        masterFound = true;
        break;
      }
    }
    if (!masterFound && isMasterDevice)
    {
      ss << "Master: 0x" << std::hex << ourDeviceId << std::dec << " (me)";
    }
    if (!masterFound && !isMasterDevice)
    {
      ss << "Master: NONE";
    }
    ss << "\n";

    // Device list
    ss << "Devices:\n";
    for (const auto &device : knownDevices)
    {
      ss << "  0x" << std::hex << device.second.deviceId << std::dec;
      ss << " | Group: 0x" << std::hex << device.second.groupId << std::dec;
      ss << " | " << (device.second.isMaster ? "MASTER" : "slave");
      ss << " | Age: " << (millis() - device.second.lastSeen) << "ms";

      // MAC address (shortened)
      ss << " | MAC: ";
      for (int i = 0; i < 6; i++)
      { // Show last 3 bytes of MAC
        char buf[3];
        sprintf(buf, "%02X", device.second.mac[i]);
        ss << buf;
        if (i < 5)
          ss << ":";
      }
      ss << "\n";
    }
  }
  else
  {
    ss << "Network: Searching for devices...\n";
  }

  // Status indicators
  ss << "Status: ";
  if (syncActive)
  {
    ss << "SYNCING";
  }
  else
  {
    ss << "STANDALONE";
  }

  if (currentGroupId == 0)
  {
    ss << " | NO GROUP";
  }

  ss << "\n"
     << String(40, '=').c_str() << "\n";

  Serial.println(ss.str().c_str());
}

// Auto-join functionality
void SyncManager::checkAutoJoin()
{
  // Skip if already in a group
  if (currentGroupId != 0)
    return;

  uint32_t currentTime = millis();

  // If we haven't started the auto-join timer, start it now
  if (autoJoinStartTime == 0)
  {
    autoJoinStartTime = currentTime;
    Serial.println("SyncManager: Starting auto-join search...");
    return;
  }

  // If we have discovered devices, try to join their group
  if (knownDevices.size() > 0)
  {
    attemptAutoJoin();
  }
  // If timeout has passed and no groups found, create our own
  else if (currentTime - autoJoinStartTime >= autoJoinTimeout)
  {
    Serial.println("SyncManager: Auto-join timeout - creating own group");
    createGroup();         // Creates random group ID
    autoJoinStartTime = 0; // Reset timer
  }
}

void SyncManager::attemptAutoJoin()
{
  uint32_t bestGroupId = findBestGroupToJoin();

  if (bestGroupId != 0)
  {
    Serial.println("SyncManager: Auto-joining group 0x" + String(bestGroupId, HEX));
    joinGroup(bestGroupId);
    autoJoinStartTime = 0; // Reset timer since we joined
  }
}

uint32_t SyncManager::findBestGroupToJoin()
{
  std::map<uint32_t, int> groupCounts;

  // Count devices per group
  for (const auto &device : knownDevices)
  {
    if (device.second.groupId != 0)
    {
      groupCounts[device.second.groupId]++;
    }
  }

  // Find the group with the most devices
  uint32_t bestGroup = 0;
  int maxCount = 0;

  for (const auto &group : groupCounts)
  {
    if (group.second > maxCount)
    {
      maxCount = group.second;
      bestGroup = group.first;
    }
  }

  return bestGroup;
}

void SyncManager::updateSyncedLED()
{
  // Only blink LED if we're in a group and time is synced
  if (currentGroupId != 0 && timeIsSynced && syncActive)
  {
    uint32_t syncTime = getSyncedTime();

    // Create 1-second blink cycle: 500ms on, 500ms off
    if ((syncTime % 1000) < 500)
    {
      led.On();
    }
    else
    {
      led.Off();
    }
  }
  else
  {
    // Turn off LED if not syncing
    led.Off();
  }
}

#else

// Stub implementation when ENABLE_SYNC is not defined
SyncManager *SyncManager::getInstance()
{
  static SyncManager instance;
  return &instance;
}

SyncManager::SyncManager() {}
SyncManager::~SyncManager() {}
void SyncManager::begin() {}
void SyncManager::loop() {}

// Group management stubs
void SyncManager::createGroup(uint32_t groupId) {}
void SyncManager::joinGroup(uint32_t groupId) {}
void SyncManager::leaveGroup() {}
uint32_t SyncManager::getGroupId() const { return 0; }

// Auto-join stubs
void SyncManager::enableAutoJoin(bool enabled) {}
void SyncManager::setAutoJoinTimeout(uint32_t timeoutMs) {}
bool SyncManager::isAutoJoinEnabled() const { return false; }

// Time synchronization stubs
void SyncManager::requestTimeSync() {}
uint32_t SyncManager::getSyncedTime() const { return millis(); }
bool SyncManager::isTimeSynced() const { return false; }
int32_t SyncManager::getTimeOffset() const { return 0; }

// Device management stubs
bool SyncManager::isSyncing() const { return false; }
bool SyncManager::isMaster() const { return false; }
uint32_t SyncManager::getDeviceId() const { return 0; }
size_t SyncManager::getDeviceCount() const { return 0; }
std::vector<DeviceInfo> SyncManager::getKnownDevices() const { return {}; }
SyncNetworkInfo SyncManager::getNetworkInfo() const { return {}; }

// Callback stubs
void SyncManager::setDeviceJoinCallback(std::function<void(const DeviceInfo &)> callback) {}
void SyncManager::setDeviceLeaveCallback(std::function<void(uint32_t deviceId)> callback) {}
void SyncManager::setMasterChangeCallback(std::function<void(uint32_t newMasterDeviceId)> callback) {}
void SyncManager::setTimeSyncCallback(std::function<void(uint32_t syncedTime)> callback) {}

// Debug stubs
void SyncManager::printDeviceInfo() {}
void SyncManager::printNetworkStatus() {}
void SyncManager::printGroupInfo() {}

// LED control stub
void SyncManager::updateSyncedLED() {}

#endif // ENABLE_SYNC
