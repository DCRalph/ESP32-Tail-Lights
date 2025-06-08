#include "SyncManager.h"
#include <WiFi.h>
#include <algorithm>
#include <sstream>
#include <iomanip> // For std::setw

#ifdef ENABLE_SYNC

// Static instance
SyncManager *SyncManager::getInstance()
{
  static SyncManager instance;
  return &instance;
}

SyncManager::SyncManager()
{
  isMasterDevice = false;
  syncActive = false;
  lastHeartbeat = 0;
  lastEffectSync = 0;
  lastMasterCheck = 0;
  lastCleanup = 0;

  // Generate a random priority number for this device
  // Higher number = higher priority for master election
  randomSeed(ESP.getEfuseMac());
  ourPriority = random(0, UINT32_MAX);

  // Initialize effect states to off
  memset(&currentEffects, 0, sizeof(EffectSyncState));

  Serial.println("SyncManager: Initialized with priority: " + String(ourPriority));
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

  // Send initial heartbeat
  sendHeartbeat();
}

void SyncManager::loop()
{
  uint32_t currentTime = millis();

  // Only perform sync operations if we have other devices
  if (knownDevices.size() > 0)
  {
    syncActive = true;

    // Send periodic heartbeat
    if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL)
    {
      sendHeartbeat();
      lastHeartbeat = currentTime;
    }

    // Check master status
    if (currentTime - lastMasterCheck >= MASTER_CHECK_INTERVAL)
    {
      checkMasterStatus();
      lastMasterCheck = currentTime;
    }

    // Send effect state if we're the master
    if (isMasterDevice && (currentTime - lastEffectSync >= EFFECT_SYNC_INTERVAL))
    {
      broadcastEffectState();
      lastEffectSync = currentTime;

      // print some debug info if debug is enabled
#ifdef DEBUG_SYNC
      // Serial.println("SyncManager: Broadcasted effect state");
      // Serial.println("SyncManager: Number of devices: " + String(knownDevices.size()));
#endif
    }

    // Clean up devices that haven't been seen for a while
    if (currentTime - lastCleanup >= CLEANUP_INTERVAL)
    {
      cleanupOldDevices();
      lastCleanup = currentTime;
    }
  }
  else
  {
    syncActive = false;
  }

  if (currentTime - lastPrintTime >= 2000)
  {
    lastPrintTime = currentTime;
    printDeviceInfo();
  }
}

void SyncManager::setEffectChangeCallback(std::function<void(const EffectSyncState &)> callback)
{
  effectChangeCallback = callback;
}

void SyncManager::updateEffectStates(const EffectSyncState &newState)
{
  if (isMasterDevice)
  {
    currentEffects = newState;
    broadcastEffectState();
  }
}

bool SyncManager::isSyncing() const
{
  return syncActive;
}

bool SyncManager::isMaster() const
{
  return isMasterDevice;
}

size_t SyncManager::getDeviceCount() const
{
  return knownDevices.size();
}

void SyncManager::handleSyncPacket(fullPacket *fp)
{
  if (fp->p.len < 1)
    return; // Invalid packet

  uint8_t subtype = fp->p.data[0];
  std::string macStr = "";

  // Convert MAC to string for map key
  for (int i = 0; i < 6; i++)
  {
    char buf[3];
    sprintf(buf, "%02X", fp->mac[i]);
    macStr += buf;
  }

#ifdef DEBUG_SYNC
  // Serial.println("SyncManager: Received packet type " + String(subtype) + " from " + String(macStr.c_str()));
#endif

  // Update last seen timestamp for this device
  if (knownDevices.find(macStr) == knownDevices.end())
  {
    // New device - add to known devices
    DeviceInfo info;
    memcpy(info.mac, fp->mac, 6);
    info.lastSeen = millis();
    info.isMaster = false;
    info.priority = 0;
    knownDevices[macStr] = info;
  }
  else
  {
    // Update existing device
    knownDevices[macStr].lastSeen = millis();
  }

  // Process by subtype
  switch (subtype)
  {
  case SYNC_HEARTBEAT:
  {
    // Extract priority
    uint32_t peerPriority = 0;
    if (fp->p.len >= 1 + sizeof(peerPriority))
    {
      memcpy(&peerPriority, &fp->p.data[1], sizeof(peerPriority));
      knownDevices[macStr].priority = peerPriority;
    }

#ifdef DEBUG_SYNC
    // Serial.println("SyncManager: Received heartbeat with priority " + String(peerPriority));
#endif
    break;
  }

  case SYNC_MASTER_ANNOUNCE:
  {
    // Someone is claiming to be master
    uint32_t masterPriority = 0;
    if (fp->p.len >= 1 + sizeof(masterPriority))
    {
      memcpy(&masterPriority, &fp->p.data[1], sizeof(masterPriority));
    }

    // If they have higher priority than us, accept them as master
    if (masterPriority > ourPriority)
    {
      // Mark all other devices as non-master first
      for (auto &device : knownDevices)
      {
        device.second.isMaster = false;
      }

      // Then set this device as master
      knownDevices[macStr].isMaster = true;
      isMasterDevice = false; // We are no longer master

#ifdef DEBUG_SYNC
      Serial.println("SyncManager: Accepted new master with priority " + String(masterPriority));
#endif
    }
    else if (masterPriority < ourPriority && isMasterDevice)
    {
      // We have higher priority, reaffirm our mastery
      becomeMaster();
    }
    break;
  }

  case SYNC_EFFECT_STATE:
  {
    // Only process if it's from the master device
    if (!knownDevices[macStr].isMaster)
      break;

    // Extract effect state
    EffectSyncState recvState;
    if (fp->p.len >= 1 + sizeof(EffectSyncState))
    {
      memcpy(&recvState, &fp->p.data[1], sizeof(EffectSyncState));

      // Only update state if there's a difference
      if (memcmp(&recvState, &currentEffects, sizeof(EffectSyncState)) != 0)
      {
        // Update our local state
        currentEffects = recvState;

        // Call callback if registered
        if (effectChangeCallback)
        {
          effectChangeCallback(currentEffects);
        }

#ifdef DEBUG_SYNC
        Serial.println("SyncManager: Updated effect state from master");
#endif
      }
    }
    break;
  }

  case SYNC_MASTER_REQUEST:
  {
    // If we're master, respond
    if (isMasterDevice)
    {
      data_packet pkt;
      pkt.type = SYNC_MSG_TYPE;
      pkt.data[0] = SYNC_MASTER_ANNOUNCE;
      memcpy(&pkt.data[1], &ourPriority, sizeof(ourPriority));
      pkt.len = 1 + sizeof(ourPriority);
      wireless.send(&pkt, (uint8_t *)BROADCAST_MAC);
    }
    break;
  }
  }
}

void SyncManager::sendHeartbeat()
{
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_HEARTBEAT;

  // Include our priority for master elections
  memcpy(&pkt.data[1], &ourPriority, sizeof(ourPriority));
  pkt.len = 1 + sizeof(ourPriority);

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

  // If no master exists, start an election
  if (!masterExists && !isMasterDevice)
  {
    electMaster();
  }
}

void SyncManager::cleanupOldDevices()
{
  uint32_t currentTime = millis();
  std::vector<std::string> devicesToRemove;

  // Find devices that haven't been seen for a while
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
    knownDevices.erase(macStr);

    // If the master disappeared, elect a new one
    if (wasMaster)
    {
      electMaster();
    }
  }

  // If we've lost all devices, turn off syncing
  if (knownDevices.empty())
  {
    syncActive = false;
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

  // If we have the highest priority, become master
  if (weAreHighest)
  {
    becomeMaster();
  }
}

void SyncManager::becomeMaster()
{
  if (!isMasterDevice)
  {
    isMasterDevice = true;

#ifdef DEBUG_SYNC
    Serial.println("SyncManager: This device is now the master!");
#endif

    // Announce mastery to all devices
    data_packet pkt;
    pkt.type = SYNC_MSG_TYPE;
    pkt.data[0] = SYNC_MASTER_ANNOUNCE;
    memcpy(&pkt.data[1], &ourPriority, sizeof(ourPriority));
    pkt.len = 1 + sizeof(ourPriority);

    wireless.send(&pkt, (uint8_t *)BROADCAST_MAC);

    // Broadcast current effect state
    broadcastEffectState();
  }
}

void SyncManager::broadcastEffectState()
{
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_EFFECT_STATE;

  // Copy current effect state into packet
  memcpy(&pkt.data[1], &currentEffects, sizeof(EffectSyncState));
  pkt.len = 1 + sizeof(EffectSyncState);

  wireless.send(&pkt, (uint8_t *)BROADCAST_MAC);
}

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

// this should print a nice looking table of the our state and the state of all known devices
// it should do this in one print call to ensure info prints as one block of text
void SyncManager::printDeviceInfo()
{
  std::stringstream ss;
  ss << "\n=== Sync Network Status ===\n";

  // Our device info
  ss << "This Device: ";
  const String mac = WiFi.macAddress();
  ss << mac;
  // for (int i = 0; i < 6; i++)
  // {
  //   ss << String(ourMac[i], HEX);
  //   if (i < 5)
  //     ss << ":";
  // }
  ss << "\n";
  ss << "Priority: " << ourPriority << "\n";
  ss << "Role: " << (isMasterDevice ? "MASTER" : "SLAVE") << "\n";
  ss << "Sync Active: " << (syncActive ? "YES" : "NO") << "\n";
  ss << "\n";

  // Current effect state
  ss << "Current Effects:\n";
  ss << "  Left Indicator: " << (currentEffects.leftIndicator ? "ON" : "OFF") << "\n";
  ss << "  Right Indicator: " << (currentEffects.rightIndicator ? "ON" : "OFF") << "\n";
  ss << "  RGB: " << (currentEffects.rgb ? "ON" : "OFF") << "\n";
  ss << "  Nightrider: " << (currentEffects.nightrider ? "ON" : "OFF") << "\n";
  ss << "  Startup: " << (currentEffects.startup ? "ON" : "OFF") << "\n";
  ss << "  Police: " << (currentEffects.police ? "ON" : "OFF") << "\n";
  ss << "  Pulse Wave: " << (currentEffects.pulseWave ? "ON" : "OFF") << "\n";
  ss << "  Aurora: " << (currentEffects.aurora ? "ON" : "OFF") << "\n";
  ss << "\n";

  // Known devices table
  ss << "Known Devices (" << knownDevices.size() << "):\n";
  if (knownDevices.size() > 0)
  {
    ss << "+-------------------+----------+--------+------------------+\n";
    ss << "| MAC Address       | Priority | Master | Last Seen (ms)   |\n";
    ss << "+-------------------+----------+--------+------------------+\n";

    uint32_t currentTime = millis();
    for (const auto &device : knownDevices)
    {
      // MAC address
      ss << "| ";
      for (int i = 0; i < 6; i++)
      {
        char buf[3];
        sprintf(buf, "%02X", device.second.mac[i]);
        ss << buf;
        if (i < 5)
          ss << ":";
      }

      // Other device info
      ss << " | " << std::setw(8) << device.second.priority;
      ss << " | " << std::setw(6) << (device.second.isMaster ? "YES" : "NO");
      ss << " | " << std::setw(16) << (currentTime - device.second.lastSeen) << " |\n";
    }
    ss << "+-------------------+----------+--------+------------------+\n";
  }

  Serial.println(ss.str().c_str());
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
void SyncManager::setEffectChangeCallback(std::function<void(const EffectSyncState &)> callback) {}
void SyncManager::updateEffectStates(const EffectSyncState &newState) {}
bool SyncManager::isSyncing() const { return false; }
bool SyncManager::isMaster() const { return false; }
size_t SyncManager::getDeviceCount() const { return 0; }
void SyncManager::printDeviceInfo() {}

#endif // ENABLE_SYNC
