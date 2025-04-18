#include "SyncManager.h"
#include <WiFi.h>
#include <algorithm>
#include <sstream>

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
      Serial.println("SyncManager: Broadcasted effect state");
      Serial.println("SyncManager: Number of devices: " + String(knownDevices.size()));
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
}

void SyncManager::setEffectChangeCallback(std::function<void(const EffectSyncState &)> callback)
{
  effectChangeCallback = callback;
}

void SyncManager::updateEffectStates(const EffectSyncState &newState)
{
  // Update our local state
  currentEffects = newState;

  // If we're the master, send the update to all other devices
  if (isMasterDevice)
  {
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
  Serial.println("SyncManager: Received packet type " + String(subtype) + " from " + String(macStr.c_str()));
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
    if (fp->p.len >= 5)
    { // Ensure packet has priority data
      // Extract priority
      uint32_t devicePriority = 0;
      memcpy(&devicePriority, &fp->p.data[1], 4);
      knownDevices[macStr].priority = devicePriority;
    }
    break;
  }

  case SYNC_MASTER_ANNOUNCE:
  {
    // Someone is announcing they're the master
    knownDevices[macStr].isMaster = true;

    // If we also think we're master, but they have higher priority, yield
    if (isMasterDevice && knownDevices[macStr].priority > ourPriority)
    {
      isMasterDevice = false;
    }
    break;
  }

  case SYNC_EFFECT_STATE:
  {
    // Master is sending effect state
    if (fp->p.len >= sizeof(EffectSyncState) + 1)
    { // +1 for subtype
      EffectSyncState receivedState;
      memcpy(&receivedState, &fp->p.data[1], sizeof(EffectSyncState));

      // Only update our state if we're not the master
      if (!isMasterDevice)
      {
        currentEffects = receivedState;

        // Notify about state change
        if (effectChangeCallback)
        {
          effectChangeCallback(currentEffects);
        }
      }
    }
    break;
  }

  case SYNC_MASTER_REQUEST:
  {
    // Someone is asking who's master
    if (isMasterDevice)
    {
      // Send master announcement
      data_packet pkt;
      pkt.type = SYNC_MSG_TYPE;
      pkt.data[0] = SYNC_MASTER_ANNOUNCE;
      memcpy(&pkt.data[1], &ourPriority, sizeof(ourPriority));
      pkt.len = 1 + sizeof(ourPriority);
      wireless.send(&pkt, fp->mac);
    }
    break;
  }

  case SYNC_MASTER_RESIGN:
  {
    // Current master is resigning
    if (knownDevices[macStr].isMaster)
    {
      knownDevices[macStr].isMaster = false;
      // Trigger a new master election
      electMaster();
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