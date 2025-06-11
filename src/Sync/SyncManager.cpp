// SyncManager.cpp
#include "SyncManager.h"
#include <WiFi.h>
#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include "IO/GPIO.h"

SyncManager *SyncManager::getInstance()
{
  static SyncManager inst;
  return &inst;
}

SyncManager::SyncManager()
{
  // Initialize random seed with multiple entropy sources
  uint64_t macVal = ESP.getEfuseMac();
  uint32_t millisVal = millis();
  uint32_t microsVal = micros();

  // Combine entropy sources for initial seed
  uint32_t initialSeed = (uint32_t)(macVal ^ (macVal >> 32)) ^ millisVal ^ microsVal;
  randomSeed(initialSeed);

  Serial.println(String("[SyncManager] Initialized with seed: 0x") + String(initialSeed, HEX));

  ourDeviceId = generateDeviceId();
  // clear initial group
  currentGroup = {};
}

SyncManager::~SyncManager() {}

void SyncManager::begin()
{
  wireless.addOnReceiveFor(
      SYNC_MSG_TYPE,
      [this](fullPacket *fp)
      { handleSyncPacket(fp); });
}

void SyncManager::loop()
{
  uint32_t now = millis();
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL)
  {
    sendHeartbeat();
    lastHeartbeat = now;
  }
  checkDiscoveryCleanup(now);
  checkGroupCleanup(now);

  if (currentGroup.groupId != 0)
  {
    if (currentGroup.isMaster)
    {
      if (now - lastGrpAnnounce >= GROUP_ANNOUNCE_INTERVAL)
      {
        sendGroupAnnounce();
        lastGrpAnnounce = now;
      }
      if (now - lastGrpInfo >= GROUP_INFO_INTERVAL)
      {
        sendGroupInfo();
        lastGrpInfo = now;
      }
    }
    else
    {
      if (now - lastTimeSync >= TIME_SYNC_INTERVAL)
      {
        requestTimeSync();
        lastTimeSync = now;
      }
    }
  }
  else if (autoJoinEnabled)
  {
    checkAutoJoin(now);
  }
}

const std::map<std::string, DiscoveredDevice> &
SyncManager::getDiscoveredDevices() const
{
  return discoveredDevices;
}

std::vector<GroupAdvert> SyncManager::getDiscoveredGroups() const
{
  std::vector<GroupAdvert> out;
  for (auto &kv : discoveredGroups)
  {
    out.push_back(kv.second);
  }
  return out;
}

const GroupInfo &SyncManager::getGroupInfo() const
{
  return currentGroup;
}

bool SyncManager::isInGroup() const
{
  return currentGroup.groupId != 0;
}

bool SyncManager::isGroupMaster() const
{
  return currentGroup.isMaster;
}

uint32_t SyncManager::getGroupId() const
{
  return currentGroup.groupId;
}

uint32_t SyncManager::getDeviceId() const
{
  return ourDeviceId;
}

void SyncManager::createGroup(uint32_t groupId)
{
  if (groupId == 0)
    groupId = generateGroupId();
  currentGroup.groupId = groupId;
  currentGroup.masterDeviceId = ourDeviceId;
  currentGroup.isMaster = true;
  currentGroup.members.clear();

  // Masters are immediately time synced (they are the reference)
  timeSynced = true;
  timeOffset = 0;
  currentGroup.timeSynced = true;
  currentGroup.timeOffset = 0;

  // add self
  std::string ms = macToString(getOurMac());
  GroupMember gm;
  gm.deviceId = ourDeviceId;
  memcpy(gm.mac, getOurMac(), 6);
  currentGroup.members[ms] = gm;
  sendGroupAnnounce();
  sendGroupInfo();
  if (onGroupCreated)
    onGroupCreated(currentGroup);
}

void SyncManager::joinGroup(uint32_t groupId)
{
  if (currentGroup.groupId == groupId)
    return;
  // leave old
  leaveGroup();
  auto it = discoveredGroups.find(groupId);
  if (it == discoveredGroups.end())
    return;
  auto &adv = it->second;
  currentGroup.groupId = groupId;
  currentGroup.masterDeviceId = adv.masterDeviceId;
  currentGroup.isMaster = false;
  currentGroup.members.clear();

  // Reset sync state when joining a group (slaves need to sync)
  timeSynced = false;
  timeOffset = 0;
  currentGroup.timeSynced = false;
  currentGroup.timeOffset = 0;

  // add self
  std::string ms = macToString(getOurMac());
  GroupMember gm;
  gm.deviceId = ourDeviceId;
  memcpy(gm.mac, getOurMac(), 6);
  currentGroup.members[ms] = gm;
  // send join
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_GROUP_JOIN;
  memcpy(&pkt.data[1], &groupId, sizeof(groupId));
  memcpy(&pkt.data[5], &ourDeviceId, sizeof(ourDeviceId));
  pkt.len = 1 + sizeof(groupId) + sizeof(ourDeviceId);
  wireless.send(&pkt, adv.masterMac);

  // Request immediate time sync after joining
  requestTimeSync();
  lastTimeSync = millis();

  if (onGroupJoined)
    onGroupJoined(currentGroup);
}

void SyncManager::leaveGroup()
{
  if (currentGroup.groupId == 0)
    return;

  // Clear sync state
  timeSynced = false;
  timeOffset = 0;

  // Clear group info including sync state
  currentGroup = {};

  if (onGroupLeft)
    onGroupLeft();
}

void SyncManager::enableAutoJoin(bool en)
{
  autoJoinEnabled = en;
  if (en)
  {
    autoJoinStartTime = millis();
  }
  else
  {
    autoJoinStartTime = 0;
  }
}

void SyncManager::setAutoJoinTimeout(uint32_t ms)
{
  autoJoinTimeout = ms;
}

bool SyncManager::isAutoJoinEnabled() const
{
  return autoJoinEnabled;
}

void SyncManager::enableAutoCreate(bool en)
{
  autoCreateEnabled = en;
  Serial.println(String("[AutoJoin] Auto-create ") + (en ? "ENABLED" : "DISABLED"));
}

bool SyncManager::isAutoCreateEnabled() const
{
  return autoCreateEnabled;
}

void SyncManager::requestTimeSync()
{
  if (currentGroup.groupId == 0 || currentGroup.isMaster)
    return;
  uint32_t reqTs = millis();
  lastTimeReq = reqTs;

  Serial.println("[TimeSync] Requesting time sync from master");

  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_TIME_REQUEST;
  memcpy(&pkt.data[1], &reqTs, sizeof(reqTs));
  pkt.len = 1 + sizeof(reqTs);
  auto &adv = discoveredGroups[currentGroup.groupId];
  wireless.send(&pkt, adv.masterMac);
}

bool SyncManager::isTimeSynced() const
{
  // Masters are always considered synced (they are the time reference)
  if (currentGroup.isMaster && currentGroup.groupId != 0)
    return true;

  return timeSynced;
}

uint32_t SyncManager::getSyncedTime() const
{
  return timeSynced ? millis() + timeOffset : millis();
}

int32_t SyncManager::getTimeOffset() const
{
  return timeOffset;
}

void SyncManager::setDeviceDiscoveredCallback(
    std::function<void(const DiscoveredDevice &)> cb) { onDeviceDiscovered = cb; }

void SyncManager::setGroupFoundCallback(
    std::function<void(const GroupAdvert &)> cb) { onGroupFound = cb; }

void SyncManager::setGroupCreatedCallback(
    std::function<void(const GroupInfo &)> cb) { onGroupCreated = cb; }

void SyncManager::setGroupJoinedCallback(
    std::function<void(const GroupInfo &)> cb) { onGroupJoined = cb; }

void SyncManager::setGroupLeftCallback(std::function<void()> cb)
{
  onGroupLeft = cb;
}

void SyncManager::setTimeSyncCallback(
    std::function<void(uint32_t)> cb) { onTimeSynced = cb; }

void SyncManager::printDeviceInfo() const
{
  Serial.println(F("\n=== DISCOVERED DEVICES ==="));

  if (discoveredDevices.empty())
  {
    Serial.println(F("No devices discovered."));
  }
  else
  {
    Serial.println(String(F("Found ")) + String(discoveredDevices.size()) + F(" device(s):"));
    Serial.println();

    int index = 1;
    for (const auto &devicePair : discoveredDevices)
    {
      const DiscoveredDevice &device = devicePair.second;

      Serial.println(String(F("Device ")) + String(index) + F(":"));
      Serial.println(String(F("  Device ID: 0x")) + String(device.deviceId, HEX));

      // Format MAC address
      String macStr = "";
      for (int j = 0; j < 6; j++)
      {
        if (device.mac[j] < 16)
          macStr += "0";
        macStr += String(device.mac[j], HEX);
        if (j < 5)
          macStr += ":";
      }
      macStr.toUpperCase();
      Serial.println(String(F("  MAC Address: ")) + macStr);

      uint32_t timeSinceLastSeen = millis() - device.lastSeen;
      Serial.println(String(F("  Last Seen: ")) + String(timeSinceLastSeen) + F("ms ago"));

      // Check if this device is in our current group
      if (currentGroup.groupId != 0)
      {
        auto memberIt = currentGroup.members.find(devicePair.first);
        if (memberIt != currentGroup.members.end())
        {
          Serial.println(F("  Status: In current group"));
        }
        else
        {
          Serial.println(F("  Status: Not in current group"));
        }
      }
      else
      {
        Serial.println(F("  Status: No group context"));
      }

      Serial.println();
      index++;
    }
  }

  Serial.println(F("==========================="));
}

void SyncManager::printGroupInfo() const
{
  Serial.println(F("\n=== GROUP INFORMATION ==="));

  if (currentGroup.groupId == 0)
  {
    Serial.println(F("Not currently in a group."));
  }
  else
  {
    Serial.println(String(F("Group ID: 0x")) + String(currentGroup.groupId, HEX));
    Serial.println(String(F("Master Device: 0x")) + String(currentGroup.masterDeviceId, HEX));
    Serial.println(String(F("Role: ")) + (currentGroup.isMaster ? "MASTER" : "SLAVE"));
    Serial.println(String(F("Time Synced: ")) + (timeSynced ? "YES" : "NO"));

    if (timeSynced)
    {
      Serial.println(String(F("Time Offset: ")) + String(timeOffset) + F("ms"));
      Serial.println(String(F("Synced Time: ")) + String(getSyncedTime()));
    }

    Serial.println(String(F("Group Members: ")) + String(currentGroup.members.size()));

    if (!currentGroup.members.empty())
    {
      Serial.println(F("\nMember Details:"));
      int index = 1;

      for (const auto &memberPair : currentGroup.members)
      {
        const GroupMember &member = memberPair.second;

        Serial.println(String(F("  Member ")) + String(index) + F(":"));
        Serial.println(String(F("    Device ID: 0x")) + String(member.deviceId, HEX));

        // Format MAC address
        String macStr = "";
        for (int j = 0; j < 6; j++)
        {
          if (member.mac[j] < 16)
            macStr += "0";
          macStr += String(member.mac[j], HEX);
          if (j < 5)
            macStr += ":";
        }
        macStr.toUpperCase();
        Serial.println(String(F("    MAC Address: ")) + macStr);

        // Check if this is the master
        if (member.deviceId == currentGroup.masterDeviceId)
        {
          Serial.println(F("    Role: MASTER"));
        }
        else
        {
          Serial.println(F("    Role: SLAVE"));
        }

        // Check if this is us
        if (member.deviceId == ourDeviceId)
        {
          Serial.println(F("    Status: This device"));
        }
        else
        {
          Serial.println(F("    Status: Remote device"));

          // Try to find additional info from discovered devices
          auto discoveredIt = discoveredDevices.find(memberPair.first);
          if (discoveredIt != discoveredDevices.end())
          {
            uint32_t timeSinceLastSeen = millis() - discoveredIt->second.lastSeen;
            Serial.println(String(F("    Last Heartbeat: ")) + String(timeSinceLastSeen) + F("ms ago"));
          }
          else
          {
            Serial.println(F("    Last Heartbeat: Unknown"));
          }
        }

        Serial.println();
        index++;
      }
    }
  }

  // Also show discovered groups
  Serial.println(F("\nDiscovered Groups:"));
  if (discoveredGroups.empty())
  {
    Serial.println(F("No other groups discovered."));
  }
  else
  {
    int index = 1;
    for (const auto &groupPair : discoveredGroups)
    {
      const GroupAdvert &group = groupPair.second;

      Serial.println(String(F("  Group ")) + String(index) + F(":"));
      Serial.println(String(F("    Group ID: 0x")) + String(group.groupId, HEX));
      Serial.println(String(F("    Master Device: 0x")) + String(group.masterDeviceId, HEX));

      // Format master MAC address
      String macStr = "";
      for (int j = 0; j < 6; j++)
      {
        if (group.masterMac[j] < 16)
          macStr += "0";
        macStr += String(group.masterMac[j], HEX);
        if (j < 5)
          macStr += ":";
      }
      macStr.toUpperCase();
      Serial.println(String(F("    Master MAC: ")) + macStr);

      uint32_t timeSinceLastSeen = millis() - group.lastSeen;
      Serial.println(String(F("    Last Announce: ")) + String(timeSinceLastSeen) + F("ms ago"));

      if (group.groupId == currentGroup.groupId)
      {
        Serial.println(F("    Status: Current group"));
      }
      else
      {
        Serial.println(F("    Status: Available to join"));
      }

      Serial.println();
      index++;
    }
  }

  Serial.println(F("========================="));
}

void SyncManager::updateSyncedLED()
{
  // Only blink the LED if we're in a group and time is synced
  if (currentGroup.groupId != 0 && timeSynced)
  {
    // Use synchronized time for blinking
    uint32_t syncTime = getSyncedTime();

    // Blink pattern: syncTime % 1000 < 500 ? on : off
    // This creates a 1Hz blink with 50% duty cycle
    bool ledState = (syncTime % 1000) < 500;

    if (ledState)
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
    // If not synced or not in group, turn LED off
    led.Off();
  }
}

void SyncManager::handleSyncPacket(fullPacket *fp)
{
  if (fp->p.len < 1)
    return;
  uint8_t sub = fp->p.data[0];
  switch (sub)
  {
  case SYNC_HEARTBEAT:
    processHeartbeat(fp);
    break;
  case SYNC_GROUP_ANNOUNCE:
    processGroupAnnounce(fp);
    break;
  case SYNC_GROUP_JOIN:
    processGroupJoin(fp);
    break;
  case SYNC_GROUP_INFO:
    processGroupInfo(fp);
    break;
  case SYNC_TIME_REQUEST:
    processTimeRequest(fp);
    break;
  case SYNC_TIME_RESPONSE:
    processTimeResponse(fp);
    break;
  default:
    break;
  }
}

void SyncManager::processHeartbeat(fullPacket *fp)
{
  if (fp->p.len < 1 + sizeof(uint32_t))
    return;
  if (memcmp(fp->mac, getOurMac(), 6) == 0)
    return;
  uint32_t devId;
  memcpy(&devId, &fp->p.data[1], sizeof(devId));
  std::string ms = macToString(fp->mac);
  bool isNew = (discoveredDevices.find(ms) == discoveredDevices.end());
  DiscoveredDevice d{devId, {}, millis()};
  memcpy(d.mac, fp->mac, 6);
  discoveredDevices[ms] = d;
  if (isNew && onDeviceDiscovered)
    onDeviceDiscovered(d);
}

void SyncManager::processGroupAnnounce(fullPacket *fp)
{
  if (fp->p.len < 1 + 4 + 4)
    return;
  if (memcmp(fp->mac, getOurMac(), 6) == 0)
    return;
  uint32_t gid, mid;
  memcpy(&gid, &fp->p.data[1], 4);
  memcpy(&mid, &fp->p.data[5], 4);
  std::string ms = macToString(fp->mac);
  bool isNew = (discoveredGroups.find(gid) == discoveredGroups.end());
  GroupAdvert adv;
  adv.groupId = gid;
  adv.masterDeviceId = mid;
  memcpy(adv.masterMac, fp->mac, 6);
  adv.lastSeen = millis();
  discoveredGroups[gid] = adv;
  if (isNew && onGroupFound)
    onGroupFound(adv);
}

void SyncManager::processGroupJoin(fullPacket *fp)
{
  if (!currentGroup.isMaster)
    return;
  if (fp->p.len < 1 + 4 + 4)
    return;
  uint32_t gid, did;
  memcpy(&gid, &fp->p.data[1], 4);
  memcpy(&did, &fp->p.data[5], 4);
  if (gid != currentGroup.groupId)
    return;
  std::string ms = macToString(fp->mac);
  GroupMember gm{
      did,
      {0},
  };
  memcpy(gm.mac, fp->mac, 6);
  currentGroup.members[ms] = gm;
  sendGroupInfo();
}

void SyncManager::processGroupInfo(fullPacket *fp)
{
  if (currentGroup.isMaster)
    return;
  if (fp->p.len < 1 + 4 + 4 + 1)
    return;
  uint32_t gid, mid;
  memcpy(&gid, &fp->p.data[1], 4);
  memcpy(&mid, &fp->p.data[5], 4);
  if (gid != currentGroup.groupId)
    return;
  uint8_t cnt = fp->p.data[9];
  size_t off = 10;
  currentGroup.masterDeviceId = mid;
  currentGroup.members.clear();
  for (uint8_t i = 0; i < cnt; i++)
  {
    if (off + 10 > fp->p.len)
      break;
    uint32_t did;
    memcpy(&did, &fp->p.data[off], 4);
    std::string ms;
    char buf[3];
    off += 4;
    for (int b = 0; b < 6; b++)
    {
      sprintf(buf, "%02X", fp->p.data[off++]);
      ms += buf;
    }
    GroupMember gm{
        did,
        {0},
    };
    memcpy(gm.mac, &fp->p.data[off - 6], 6);
    currentGroup.members[ms] = gm;
  }
}

void SyncManager::processTimeRequest(fullPacket *fp)
{
  if (!currentGroup.isMaster)
    return;
  if (fp->p.len < 1 + 4)
    return;
  uint32_t reqTs;
  memcpy(&reqTs, &fp->p.data[1], 4);
  uint32_t now = millis();

  Serial.println("[TimeSync] Master processing time request, responding with time: " + String(now));

  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_TIME_RESPONSE;
  memcpy(&pkt.data[1], &reqTs, 4);
  memcpy(&pkt.data[5], &now, 4);
  pkt.len = 1 + 4 + 4;
  wireless.send(&pkt, fp->mac);
}

void SyncManager::processTimeResponse(fullPacket *fp)
{
  if (currentGroup.isMaster || currentGroup.groupId == 0)
    return;
  if (fp->p.len < 1 + 4 + 4)
    return;
  uint32_t reqTs, masterTs;
  memcpy(&reqTs, &fp->p.data[1], 4);
  memcpy(&masterTs, &fp->p.data[5], 4);
  uint32_t now = millis();
  uint32_t rtt = now - reqTs;
  int32_t newOff = (int32_t)masterTs + (int32_t)(rtt / 2) - (int32_t)now;

  Serial.println("[TimeSync] Received time response - RTT: " + String(rtt) + "ms, New offset: " + String(newOff) + "ms");

  if (!timeSynced)
  {
    timeOffset = newOff;
    timeSynced = true;
    Serial.println("[TimeSync] Initial sync achieved!");
  }
  else
  {
    int32_t oldOffset = timeOffset;
    timeOffset = (timeOffset * 3 + newOff) / 4;
    Serial.println("[TimeSync] Sync updated - Old offset: " + String(oldOffset) + "ms, New offset: " + String(timeOffset) + "ms");
  }

  // Update group sync state to match
  currentGroup.timeSynced = timeSynced;
  currentGroup.timeOffset = timeOffset;

  if (onTimeSynced)
    onTimeSynced(getSyncedTime());
}

void SyncManager::sendHeartbeat()
{
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_HEARTBEAT;
  memcpy(&pkt.data[1], &ourDeviceId, sizeof(ourDeviceId));
  pkt.len = 1 + sizeof(ourDeviceId);
  wireless.send(&pkt, BROADCAST_MAC);
}

void SyncManager::sendGroupAnnounce()
{
  if (!currentGroup.isMaster)
    return;
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_GROUP_ANNOUNCE;
  memcpy(&pkt.data[1], &currentGroup.groupId, 4);
  memcpy(&pkt.data[5], &currentGroup.masterDeviceId, 4);
  pkt.len = 1 + 4 + 4;
  wireless.send(&pkt, BROADCAST_MAC);
}

void SyncManager::sendGroupInfo()
{
  if (!currentGroup.isMaster)
    return;
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_GROUP_INFO;
  uint8_t cnt = currentGroup.members.size();
  memcpy(&pkt.data[1], &currentGroup.groupId, 4);
  memcpy(&pkt.data[5], &currentGroup.masterDeviceId, 4);
  pkt.data[9] = cnt;
  size_t off = 10;
  for (auto &kv : currentGroup.members)
  {
    if (off + 10 > sizeof(pkt.data))
      break;
    memcpy(&pkt.data[off], &kv.second.deviceId, 4);
    off += 4;
    memcpy(&pkt.data[off], kv.second.mac, 6);
    off += 6;
  }
  pkt.len = off;
  wireless.send(&pkt, BROADCAST_MAC);
}

void SyncManager::checkDiscoveryCleanup(uint32_t now)
{
  std::vector<std::string> rm;
  for (auto &kv : discoveredDevices)
  {
    if (now - kv.second.lastSeen > DISCOVERY_TIMEOUT)
    {
      rm.push_back(kv.first);
    }
  }
  for (auto &key : rm)
    discoveredDevices.erase(key);
}

void SyncManager::checkGroupCleanup(uint32_t now)
{
  std::vector<uint32_t> rm;
  for (auto &kv : discoveredGroups)
  {
    if (now - kv.second.lastSeen > GROUP_DISCOVERY_TIMEOUT)
    {
      rm.push_back(kv.first);
    }
  }
  for (auto gid : rm)
  {
    discoveredGroups.erase(gid);
    if (!currentGroup.isMaster &&
        currentGroup.groupId == gid)
    {
      leaveGroup();
    }
  }
}

void SyncManager::checkAutoJoin(uint32_t now)
{
  if (currentGroup.groupId != 0)
    return;
  if (autoJoinStartTime == 0)
  {
    autoJoinStartTime = now;
    return;
  }
  if (!discoveredGroups.empty())
  {
    joinGroup(discoveredGroups.begin()->first);
    autoJoinStartTime = 0;
  }
  else if (now - autoJoinStartTime >= autoJoinTimeout)
  {
    if (autoCreateEnabled)
    {
      Serial.println("[AutoJoin] No groups found, creating new group (auto-create enabled)");
      createGroup();
      autoJoinStartTime = 0;
    }
    else
    {
      // Reset the timer to keep looking for groups without creating one
      Serial.println("[AutoJoin] No groups found, continuing search (auto-create disabled)");
      autoJoinStartTime = now;
    }
  }
}

uint32_t SyncManager::generateDeviceId()
{
  // Use multiple entropy sources for better randomization
  uint64_t macVal = ESP.getEfuseMac();
  uint32_t mac32Val = (uint32_t)(macVal & 0xFFFFFFFF);
  uint32_t currentTime = millis();
  uint32_t microTime = micros();

  // Re-seed random with current time and chip info for additional entropy
  randomSeed(mac32Val ^ currentTime ^ microTime);

  // Generate multiple random values and combine them
  uint32_t rand1 = random(1, UINT32_MAX);
  uint32_t rand2 = random(1, UINT32_MAX);
  uint32_t rand3 = random(1, UINT32_MAX);
  uint32_t rand4 = random(1, UINT32_MAX);

  // Combine all entropy sources using different operations
  uint32_t deviceId = 0;
  deviceId ^= (uint32_t)(macVal & 0xFFFFFFFF);         // Lower 32 bits of MAC
  deviceId ^= (uint32_t)((macVal >> 32) & 0xFFFFFFFF); // Upper 32 bits of MAC
  deviceId ^= currentTime;
  deviceId ^= microTime;
  deviceId ^= rand1;
  deviceId = (deviceId << 7) ^ rand2;               // Bit shift and XOR
  deviceId = (deviceId * 31) ^ rand3;               // Multiply by prime and XOR
  deviceId = ((deviceId >> 13) ^ deviceId) * rand4; // More bit mixing

  // Final mixing to ensure good distribution
  deviceId ^= (deviceId >> 16);
  deviceId *= 0x85ebca6b;
  deviceId ^= (deviceId >> 13);
  deviceId *= 0xc2b2ae35;
  deviceId ^= (deviceId >> 16);

  // Ensure we never return 0
  if (deviceId == 0)
  {
    deviceId = rand1 ^ rand2 ^ rand3 ^ rand4 ^ 0xDEADBEEF;
  }

  return deviceId;
}

uint32_t SyncManager::generateGroupId()
{
  return random(1, UINT32_MAX);
}

std::string SyncManager::macToString(const uint8_t *mac) const
{
  char buf[3];
  std::string s;
  for (int i = 0; i < 6; i++)
  {
    sprintf(buf, "%02X", mac[i]);
    s += buf;
  }
  return s;
}

const uint8_t *SyncManager::getOurMac()
{
  static uint8_t our[6];
  static bool inited = false;
  if (!inited)
  {
    WiFi.macAddress(our);
    inited = true;
  }
  return our;
}