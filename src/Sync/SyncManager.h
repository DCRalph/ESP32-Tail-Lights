// SyncManager.h
#pragma once

#include <Arduino.h>
#include <map>
#include <vector>
#include <functional>
#include <string>
#include "IO/Wireless.h"
#include "config.h"

// Message types
constexpr uint8_t SYNC_MSG_TYPE = 0xA0;
constexpr uint8_t SYNC_HEARTBEAT = 0x01;
constexpr uint8_t SYNC_GROUP_ANNOUNCE = 0x02;
constexpr uint8_t SYNC_GROUP_JOIN = 0x03;
constexpr uint8_t SYNC_GROUP_INFO = 0x04;
constexpr uint8_t SYNC_TIME_REQUEST = 0x05;
constexpr uint8_t SYNC_TIME_RESPONSE = 0x06;

struct DiscoveredDevice
{
  uint32_t deviceId;
  uint8_t mac[6];
  uint32_t lastSeen;
};

struct GroupAdvert
{
  uint32_t groupId;
  uint32_t masterDeviceId;
  uint8_t masterMac[6];
  uint32_t lastSeen;
};

struct GroupMember
{
  uint32_t deviceId;
  uint8_t mac[6];
};

struct GroupInfo
{
  uint32_t groupId;
  uint32_t masterDeviceId;
  bool isMaster;
  // keyed by MAC string
  std::map<std::string, GroupMember> members;
  bool timeSynced;
  int32_t timeOffset;
};

class SyncManager
{
public:
  static SyncManager *getInstance();

  void begin();
  void loop();

  // Device discovery
  const std::map<std::string, DiscoveredDevice> &
  getDiscoveredDevices() const;

  // Group discovery
  std::vector<GroupAdvert> getDiscoveredGroups() const;

  // Current group state
  const GroupInfo &getGroupInfo() const;
  bool isInGroup() const;
  bool isGroupMaster() const;
  uint32_t getGroupId() const;

  // Self
  uint32_t getDeviceId() const;

  // Group operations
  void createGroup(uint32_t groupId = 0);
  void joinGroup(uint32_t groupId);
  void leaveGroup();

  // Auto‐join
  void enableAutoJoin(bool enabled = true);
  void setAutoJoinTimeout(uint32_t ms);
  bool isAutoJoinEnabled() const;

  // Auto-create (separate from auto-join)
  void enableAutoCreate(bool enabled = true);
  bool isAutoCreateEnabled() const;

  // Time sync
  void requestTimeSync();
  bool isTimeSynced() const;
  uint32_t getSyncedTime() const;
  int32_t getTimeOffset() const;

  // Callbacks
  void setDeviceDiscoveredCallback(
      std::function<void(const DiscoveredDevice &)> cb);
  void setGroupFoundCallback(
      std::function<void(const GroupAdvert &)> cb);
  void setGroupCreatedCallback(
      std::function<void(const GroupInfo &)> cb);
  void setGroupJoinedCallback(
      std::function<void(const GroupInfo &)> cb);
  void setGroupLeftCallback(std::function<void()> cb);
  void setTimeSyncCallback(
      std::function<void(uint32_t syncedTime)> cb);

  // Debug/Info functions
  void printDeviceInfo() const;
  void printGroupInfo() const;

  // Test LED for sync visualization
  void updateSyncedLED();

private:
  SyncManager();
  ~SyncManager();

  // packet handlers
  void handleSyncPacket(fullPacket *fp);
  void processHeartbeat(fullPacket *fp);
  void processGroupAnnounce(fullPacket *fp);
  void processGroupJoin(fullPacket *fp);
  void processGroupInfo(fullPacket *fp);
  void processTimeRequest(fullPacket *fp);
  void processTimeResponse(fullPacket *fp);

  // senders
  void sendHeartbeat();
  void sendGroupAnnounce();
  void sendGroupInfo();

  // periodic tasks
  void checkDiscoveryCleanup(uint32_t now);
  void checkGroupCleanup(uint32_t now);
  void checkAutoJoin(uint32_t now);

  // utilities
  uint32_t generateDeviceId();
  uint32_t generateGroupId();
  std::string macToString(const uint8_t *mac) const;
  const uint8_t *getOurMac();

  // state
  std::map<std::string, DiscoveredDevice> discoveredDevices;
  std::map<uint32_t, GroupAdvert> discoveredGroups;
  GroupInfo currentGroup;
  uint32_t ourDeviceId;

  // auto‐join
  bool autoJoinEnabled = false;
  uint32_t autoJoinTimeout = 10000;
  uint32_t autoJoinStartTime = 0;

  // auto-create
  bool autoCreateEnabled = false;

  // time‐sync
  bool timeSynced = false;
  int32_t timeOffset = 0;
  uint32_t lastTimeSync = 0;
  uint32_t lastTimeReq = 0;

  // timers
  uint32_t lastHeartbeat = 0;
  uint32_t lastGrpAnnounce = 0;
  uint32_t lastGrpInfo = 0;
  uint32_t lastDiscCleanup = 0;
  uint32_t lastGrpCleanup = 0;

  // callbacks
  std::function<void(const DiscoveredDevice &)> onDeviceDiscovered;
  std::function<void(const GroupAdvert &)> onGroupFound;
  std::function<void(const GroupInfo &)> onGroupCreated;
  std::function<void(const GroupInfo &)> onGroupJoined;
  std::function<void()> onGroupLeft;
  std::function<void(uint32_t)> onTimeSynced;

  // intervals/timeouts (ms)
  static constexpr uint32_t HEARTBEAT_INTERVAL = 1000;
  static constexpr uint32_t DISCOVERY_TIMEOUT = 6000;
  static constexpr uint32_t GROUP_ANNOUNCE_INTERVAL = 2000;
  static constexpr uint32_t GROUP_DISCOVERY_TIMEOUT = 6000;
  static constexpr uint32_t GROUP_INFO_INTERVAL = 2000;
  static constexpr uint32_t TIME_SYNC_INTERVAL = 10000;
};