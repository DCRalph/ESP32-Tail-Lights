// SyncManager.cpp
#include "SyncManager.h"
#include <WiFi.h>
#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include "IO/GPIO.h"
#include "IO/StatusLed.h"
#include "IO/TimeProfiler.h"

SyncManager *SyncManager::getInstance()
{
  static SyncManager inst;
  return &inst;
}

SyncManager::SyncManager()
{
  ourDeviceId = generateDeviceId();
  // clear initial group
  currentGroup = {};

  // Load saved preferences
}

SyncManager::~SyncManager() {}

void SyncManager::begin()
{
  wireless.addOnReceiveFor(
      SYNC_MSG_TYPE,
      [this](fullPacket *fp)
      { handleSyncPacket(fp); });

  loadPreferences();
}

void SyncManager::loop()
{
  timeProfiler.start("syncManagerLoop", TimeUnit::MICROSECONDS);

  uint32_t now = millis();
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL)
  {
    sendHeartbeat();
    lastHeartbeat = now;
  }

  checkDiscoveryCleanup(now);
  checkGroupCleanup(now);
  checkMemberTimeout(now);

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
      if (effectSyncEnabled && now - lastEffectSync >= EFFECT_SYNC_INTERVAL)
      {
        // currentEffectState.print();
        sendEffectState();
        lastEffectSync = now;
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
  else
  {
    // Check auto join and auto create when not in a group
    if (autoJoinEnabled)
    {
      checkAutoJoin(now);
    }
    else if (autoCreateEnabled)
    {
      checkAutoCreate(now);
    }
  }

  timeProfiler.stop("syncManagerLoop");
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

  GroupJoinCmd joinCmd;
  joinCmd.groupId = groupId;
  joinCmd.deviceId = ourDeviceId;

  memcpy(&pkt.data[1], &joinCmd, sizeof(joinCmd));
  pkt.len = 1 + sizeof(joinCmd);
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

  // If we're not the master, notify the master that we're leaving
  if (!currentGroup.isMaster && discoveredGroups.find(currentGroup.groupId) != discoveredGroups.end())
  {
    const auto &groupAdv = discoveredGroups[currentGroup.groupId];

    data_packet pkt;
    pkt.type = SYNC_MSG_TYPE;
    pkt.data[0] = SYNC_GROUP_LEAVE;

    GroupLeaveCmd leaveCmd;
    leaveCmd.groupId = currentGroup.groupId;
    leaveCmd.deviceId = ourDeviceId;

    memcpy(&pkt.data[1], &leaveCmd, sizeof(leaveCmd));
    pkt.len = 1 + sizeof(leaveCmd);
    wireless.send(&pkt, (uint8_t *)groupAdv.masterMac);

    Serial.println("[GroupLeave] Notified master of group departure");
  }

  // Clear sync state
  timeSynced = false;
  timeOffset = 0;

  // Clear group info including sync state
  currentGroup = {};

  if (onGroupLeft)
    onGroupLeft();
}

// Auto Join Settings
void SyncManager::enableAutoJoin(bool enabled)
{
  autoJoinEnabled = enabled;
  if (enabled)
  {
    autoJoinStartTime = millis();
    Serial.println("[AutoJoin] Auto-join ENABLED");
  }
  else
  {
    autoJoinStartTime = 0;
    Serial.println("[AutoJoin] Auto-join DISABLED");
  }
  saveAutoJoinPreferences();
}

void SyncManager::setAutoJoinTimeout(uint32_t ms)
{
  autoJoinTimeout = ms;
  Serial.println(String("[AutoJoin] Timeout set to ") + String(ms) + "ms");
  saveAutoJoinPreferences();
}

bool SyncManager::isAutoJoinEnabled() const
{
  return autoJoinEnabled;
}

uint32_t SyncManager::getAutoJoinTimeout() const
{
  return autoJoinTimeout;
}

// Auto Create Settings (separate from auto-join)
void SyncManager::enableAutoCreate(bool enabled)
{
  autoCreateEnabled = enabled;
  if (enabled)
  {
    autoCreateStartTime = millis();
    Serial.println("[AutoCreate] Auto-create ENABLED");
  }
  else
  {
    autoCreateStartTime = 0;
    Serial.println("[AutoCreate] Auto-create DISABLED");
  }
  saveAutoCreatePreferences();
}

void SyncManager::setAutoCreateTimeout(uint32_t ms)
{
  autoCreateTimeout = ms;
  Serial.println(String("[AutoCreate] Timeout set to ") + String(ms) + "ms");
  saveAutoCreatePreferences();
}

bool SyncManager::isAutoCreateEnabled() const
{
  return autoCreateEnabled;
}

uint32_t SyncManager::getAutoCreateTimeout() const
{
  return autoCreateTimeout;
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

  TimeRequestCmd requestCmd;
  requestCmd.requestTimestamp = reqTs;

  memcpy(&pkt.data[1], &requestCmd, sizeof(requestCmd));
  pkt.len = 1 + sizeof(requestCmd);
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

uint32_t SyncManager::syncMillis()
{
  SyncManager *syncMgr = SyncManager::getInstance();
  if (syncMgr->isTimeSynced())
  {
    return syncMgr->getSyncedTime();
  }
  return millis();
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

void SyncManager::setEffectSyncCallback(
    std::function<void(const EffectSyncState &)> cb) { onEffectSync = cb; }

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

    if (currentGroup.isMaster && currentGroup.members.size() < 2)
    {
      statusLed2.setColor(255, 0, 255);
    }
    else if (ledState)
    {
      if (currentGroup.isMaster)
        statusLed2.setColor(255, 0, 255);
      else
        statusLed2.setColor(0, 0, 255);
    }
    else
    {
      statusLed2.setColor(0, 0, 0);
    }
    // statusLeds.show();
  }
  else
  {
    // If not synced or not in group, turn LED off
    statusLed2.setColor(0, 0, 0);
    // statusLeds.show();
  }
}

void SyncManager::handleSyncPacket(fullPacket *fp)
{


  if (fp->p.len < 1)
  {

    return;
  }

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
  case SYNC_GROUP_LEAVE:
    processGroupLeave(fp);
    break;
  case SYNC_TIME_REQUEST:
    processTimeRequest(fp);
    break;
  case SYNC_TIME_RESPONSE:
    processTimeResponse(fp);
    break;
  case SYNC_EFFECT_STATE:
    processEffectState(fp);
    break;
  default:
    break;
  }


}

void SyncManager::processHeartbeat(fullPacket *fp)
{
  if (fp->p.len < 1 + sizeof(HeartbeatCmd))
    return;
  if (memcmp(fp->mac, getOurMac(), 6) == 0)
    return;

  HeartbeatCmd heartbeatCmd;
  memcpy(&heartbeatCmd, &fp->p.data[1], sizeof(heartbeatCmd));

  std::string ms = macToString(fp->mac);
  bool isNew = (discoveredDevices.find(ms) == discoveredDevices.end());
  DiscoveredDevice d{heartbeatCmd.deviceId, {}, millis()};
  memcpy(d.mac, fp->mac, 6);
  discoveredDevices[ms] = d;
  if (isNew && onDeviceDiscovered)
    onDeviceDiscovered(d);
}

void SyncManager::processGroupAnnounce(fullPacket *fp)
{
  if (fp->p.len < 1 + sizeof(GroupAnnounceCmd))
    return;
  if (memcmp(fp->mac, getOurMac(), 6) == 0)
    return;

  GroupAnnounceCmd announceCmd;
  memcpy(&announceCmd, &fp->p.data[1], sizeof(announceCmd));

  std::string ms = macToString(fp->mac);
  bool isNew = (discoveredGroups.find(announceCmd.groupId) == discoveredGroups.end());
  GroupAdvert adv;
  adv.groupId = announceCmd.groupId;
  adv.masterDeviceId = announceCmd.masterDeviceId;
  memcpy(adv.masterMac, fp->mac, 6);
  adv.lastSeen = millis();
  discoveredGroups[announceCmd.groupId] = adv;
  if (isNew && onGroupFound)
    onGroupFound(adv);
}

void SyncManager::processGroupJoin(fullPacket *fp)
{
  if (!currentGroup.isMaster)
    return;
  if (fp->p.len < 1 + sizeof(GroupJoinCmd))
    return;

  GroupJoinCmd joinCmd;
  memcpy(&joinCmd, &fp->p.data[1], sizeof(joinCmd));

  if (joinCmd.groupId != currentGroup.groupId)
    return;

  std::string ms = macToString(fp->mac);
  GroupMember gm{
      joinCmd.deviceId,
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
  if (fp->p.len < 1 + sizeof(GroupInfoCmd))
    return;

  GroupInfoCmd infoCmd;
  memcpy(&infoCmd, &fp->p.data[1], sizeof(infoCmd));

  if (infoCmd.groupId != currentGroup.groupId)
    return;

  currentGroup.masterDeviceId = infoCmd.masterDeviceId;
  currentGroup.members.clear();

  size_t off = 1 + sizeof(infoCmd);
  for (uint8_t i = 0; i < infoCmd.memberCount; i++)
  {
    if (off + sizeof(GroupInfoMember) > fp->p.len)
      break;

    GroupInfoMember member;
    memcpy(&member, &fp->p.data[off], sizeof(member));
    off += sizeof(member);

    std::string ms;
    char buf[3];
    for (int b = 0; b < 6; b++)
    {
      sprintf(buf, "%02X", member.mac[b]);
      ms += buf;
    }

    GroupMember gm{
        member.deviceId,
        {0},
    };
    memcpy(gm.mac, member.mac, 6);
    currentGroup.members[ms] = gm;
  }
}

void SyncManager::processGroupLeave(fullPacket *fp)
{
  // Only masters should process leave messages
  if (!currentGroup.isMaster)
    return;
  if (fp->p.len < 1 + sizeof(GroupLeaveCmd))
    return;

  GroupLeaveCmd leaveCmd;
  memcpy(&leaveCmd, &fp->p.data[1], sizeof(leaveCmd));

  // Verify it's for our group
  if (leaveCmd.groupId != currentGroup.groupId)
    return;

  // Find and remove the leaving member
  std::string macStr = macToString(fp->mac);
  auto memberIt = currentGroup.members.find(macStr);
  if (memberIt != currentGroup.members.end())
  {
    Serial.println("[GroupLeave] Device 0x" + String(leaveCmd.deviceId, HEX) + " left the group");
    currentGroup.members.erase(memberIt);

    // Broadcast updated group info to remaining members
    sendGroupInfo();
  }
}

void SyncManager::processTimeRequest(fullPacket *fp)
{
  if (!currentGroup.isMaster)
    return;
  if (fp->p.len < 1 + sizeof(TimeRequestCmd))
    return;

  TimeRequestCmd requestCmd;
  memcpy(&requestCmd, &fp->p.data[1], sizeof(requestCmd));

  uint32_t now = millis();

  Serial.println("[TimeSync] Master processing time request, responding with time: " + String(now));

  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_TIME_RESPONSE;

  TimeResponseCmd responseCmd;
  responseCmd.requestTimestamp = requestCmd.requestTimestamp;
  responseCmd.masterTimestamp = now;

  memcpy(&pkt.data[1], &responseCmd, sizeof(responseCmd));
  pkt.len = 1 + sizeof(responseCmd);
  wireless.send(&pkt, fp->mac);
}

void SyncManager::processTimeResponse(fullPacket *fp)
{
  if (currentGroup.isMaster || currentGroup.groupId == 0)
    return;
  if (fp->p.len < 1 + sizeof(TimeResponseCmd))
    return;

  TimeResponseCmd responseCmd;
  memcpy(&responseCmd, &fp->p.data[1], sizeof(responseCmd));

  uint32_t now = millis();
  uint32_t rtt = now - responseCmd.requestTimestamp;
  int32_t newOff = (int32_t)responseCmd.masterTimestamp + (int32_t)(rtt / 2) - (int32_t)now;

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

void SyncManager::processEffectState(fullPacket *fp)
{
  // Only slaves should process effect state from master
  if (currentGroup.isMaster || currentGroup.groupId == 0)
    return;

  if (fp->p.len < 1 + sizeof(EffectSyncState))
    return;

  EffectSyncState newState;
  memcpy(&newState, &fp->p.data[1], sizeof(newState));

  // Update our local state
  currentEffectState = newState;

  // Notify the application
  if (onEffectSync)
  {
    onEffectSync(currentEffectState);
  }

  Serial.println("[EffectSync] Received effect state from master");
}

void SyncManager::sendHeartbeat()
{
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_HEARTBEAT;

  HeartbeatCmd heartbeatCmd;
  heartbeatCmd.deviceId = ourDeviceId;

  memcpy(&pkt.data[1], &heartbeatCmd, sizeof(heartbeatCmd));
  pkt.len = 1 + sizeof(heartbeatCmd);
  wireless.send(&pkt, BROADCAST_MAC);
}

void SyncManager::sendGroupAnnounce()
{
  if (!currentGroup.isMaster)
    return;
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_GROUP_ANNOUNCE;

  GroupAnnounceCmd announceCmd;
  announceCmd.groupId = currentGroup.groupId;
  announceCmd.masterDeviceId = currentGroup.masterDeviceId;

  memcpy(&pkt.data[1], &announceCmd, sizeof(announceCmd));
  pkt.len = 1 + sizeof(announceCmd);
  wireless.send(&pkt, BROADCAST_MAC);
}

void SyncManager::sendGroupInfo()
{
  if (!currentGroup.isMaster)
    return;
  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_GROUP_INFO;

  GroupInfoCmd infoCmd;
  infoCmd.groupId = currentGroup.groupId;
  infoCmd.masterDeviceId = currentGroup.masterDeviceId;
  infoCmd.memberCount = currentGroup.members.size();

  memcpy(&pkt.data[1], &infoCmd, sizeof(infoCmd));
  size_t off = 1 + sizeof(infoCmd);

  for (auto &kv : currentGroup.members)
  {
    if (off + sizeof(GroupInfoMember) > sizeof(pkt.data))
      break;

    GroupInfoMember member;
    member.deviceId = kv.second.deviceId;
    memcpy(member.mac, kv.second.mac, 6);

    memcpy(&pkt.data[off], &member, sizeof(member));
    off += sizeof(member);
  }
  pkt.len = off;
  wireless.send(&pkt, BROADCAST_MAC);
}

void SyncManager::sendEffectState()
{
  if (!currentGroup.isMaster || !effectSyncEnabled)
    return;

  data_packet pkt;
  pkt.type = SYNC_MSG_TYPE;
  pkt.data[0] = SYNC_EFFECT_STATE;
  memcpy(&pkt.data[1], &currentEffectState, sizeof(currentEffectState));
  pkt.len = 1 + sizeof(currentEffectState);
  wireless.send(&pkt, BROADCAST_MAC);

  // Serial.println("[EffectSync] Broadcasting effect state to group");
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

void SyncManager::checkMemberTimeout(uint32_t now)
{
  // Only check member timeouts if we're in a group
  if (currentGroup.groupId == 0)
    return;

  std::vector<std::string> membersToRemove;

  // Check each group member for timeout
  for (const auto &memberPair : currentGroup.members)
  {
    const std::string &macStr = memberPair.first;
    const GroupMember &member = memberPair.second;

    // Skip checking our own device
    if (member.deviceId == ourDeviceId)
      continue;

    // Check if this member is still in discovered devices and hasn't timed out
    auto discoveredIt = discoveredDevices.find(macStr);
    if (discoveredIt == discoveredDevices.end() ||
        (now - discoveredIt->second.lastSeen > GROUP_MEMBER_TIMEOUT))
    {
      // Member has timed out - mark for removal
      membersToRemove.push_back(macStr);
      Serial.println("[MemberTimeout] Removing timed out member: Device 0x" +
                     String(member.deviceId, HEX) + " (MAC: " + String(macStr.c_str()) + ")");
    }
  }

  // Remove timed out members
  bool membersWereRemoved = false;
  for (const std::string &macStr : membersToRemove)
  {
    currentGroup.members.erase(macStr);
    membersWereRemoved = true;
  }

  // If we're the master and members were removed, broadcast updated group info
  if (currentGroup.isMaster && membersWereRemoved)
  {
    Serial.println("[MemberTimeout] " + String(membersToRemove.size()) +
                   " member(s) removed. Current group size: " + String(currentGroup.members.size()));
    sendGroupInfo();
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

  // Check if we found any groups to join
  if (!discoveredGroups.empty())
  {
    Serial.println("[AutoJoin] Found group, joining...");
    joinGroup(discoveredGroups.begin()->first);
    autoJoinStartTime = 0;
  }
  else if (now - autoJoinStartTime >= autoJoinTimeout)
  {
    // Auto-join timeout reached, no groups found
    Serial.println("[AutoJoin] Timeout reached, no groups found");
    autoJoinStartTime = now; // Reset timer to keep searching
  }
}

void SyncManager::checkAutoCreate(uint32_t now)
{
  if (currentGroup.groupId != 0)
    return;

  if (autoCreateStartTime == 0)
  {
    autoCreateStartTime = now;
    return;
  }

  // Check if timeout reached for creating a new group
  if (now - autoCreateStartTime >= autoCreateTimeout)
  {
    Serial.println("[AutoCreate] Timeout reached, creating new group");
    createGroup();
    autoCreateStartTime = 0;
  }
}

uint32_t SyncManager::generateDeviceId()
{

  if (ourDeviceId != 0)
    return ourDeviceId;

  if (deviceInfo.serialNumber != 0)
  {
    return deviceInfo.serialNumber;
  }

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

// Effect sync methods
void SyncManager::setEffectSyncState(const EffectSyncState &state)
{
  currentEffectState = state;

  // if (millis() % 1000 < 10)
  // {
  //   currentEffectState.print();
  // }

  // If we're the master and effect sync is enabled, broadcast the state
  if (currentGroup.isMaster && effectSyncEnabled)
  {
    // sendEffectState();
  }
}

const EffectSyncState &SyncManager::getEffectSyncState() const
{
  return currentEffectState;
}

void SyncManager::enableEffectSync(bool enabled)
{
  effectSyncEnabled = enabled;
}

bool SyncManager::isEffectSyncEnabled() const
{
  return effectSyncEnabled;
}

// Preferences Management
void SyncManager::loadPreferences()
{
  // Load auto-join preferences
  autoJoinEnabled = preferences.getBool("sync_aj_en", false);
  autoJoinTimeout = preferences.getUInt("sync_aj_to", 10000);

  // Load auto-create preferences
  autoCreateEnabled = preferences.getBool("sync_ac_en", false);
  autoCreateTimeout = preferences.getUInt("sync_ac_to", 30000);

  Serial.println("[SyncManager] Loaded preferences:");
  Serial.println(String("  Auto-join: ") + (autoJoinEnabled ? "ENABLED" : "DISABLED") + " (timeout: " + String(autoJoinTimeout) + "ms)");
  Serial.println(String("  Auto-create: ") + (autoCreateEnabled ? "ENABLED" : "DISABLED") + " (timeout: " + String(autoCreateTimeout) + "ms)");
}

void SyncManager::saveAutoJoinPreferences()
{
  preferences.putBool("sync_aj_en", autoJoinEnabled);
  preferences.putUInt("sync_aj_to", autoJoinTimeout);
  Serial.println("[SyncManager] Auto-join preferences saved");
}

void SyncManager::saveAutoCreatePreferences()
{
  preferences.putBool("sync_ac_en", autoCreateEnabled);
  preferences.putUInt("sync_ac_to", autoCreateTimeout);
  Serial.println("[SyncManager] Auto-create preferences saved");
}

void SyncManager::printAutoSettings() const
{
  Serial.println(F("\n=== AUTO SETTINGS ==="));

  Serial.println(F("Auto-Join:"));
  Serial.println(String(F("  Enabled: ")) + (autoJoinEnabled ? "YES" : "NO"));
  Serial.println(String(F("  Timeout: ")) + String(autoJoinTimeout) + F("ms"));
  if (autoJoinEnabled && autoJoinStartTime > 0)
  {
    uint32_t elapsed = millis() - autoJoinStartTime;
    Serial.println(String(F("  Running for: ")) + String(elapsed) + F("ms"));
  }

  Serial.println(F("\nAuto-Create:"));
  Serial.println(String(F("  Enabled: ")) + (autoCreateEnabled ? "YES" : "NO"));
  Serial.println(String(F("  Timeout: ")) + String(autoCreateTimeout) + F("ms"));
  if (autoCreateEnabled && autoCreateStartTime > 0)
  {
    uint32_t elapsed = millis() - autoCreateStartTime;
    Serial.println(String(F("  Running for: ")) + String(elapsed) + F("ms"));
  }

  Serial.println(F("======================"));
}