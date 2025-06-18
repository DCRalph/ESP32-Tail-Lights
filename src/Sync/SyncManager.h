// SyncManager.h
#pragma once

#include <Arduino.h>
#include <map>
#include <vector>
#include <functional>
#include <string>
#include "IO/Wireless.h"
#include "config.h"

#include "IO/LED/Types.h"

// Message types
constexpr uint8_t SYNC_MSG_TYPE = 0xA0;
constexpr uint8_t SYNC_HEARTBEAT = 0x01;
constexpr uint8_t SYNC_GROUP_ANNOUNCE = 0x02;
constexpr uint8_t SYNC_GROUP_JOIN = 0x03;
constexpr uint8_t SYNC_GROUP_INFO = 0x04;
constexpr uint8_t SYNC_TIME_REQUEST = 0x05;
constexpr uint8_t SYNC_TIME_RESPONSE = 0x06;
constexpr uint8_t SYNC_EFFECT_STATE = 0x07;
constexpr uint8_t SYNC_GROUP_LEAVE = 0x08;

// Sync modes - simplified from complex auto-join/auto-create system
enum class SyncMode : uint8_t
{
  SOLO,
  JOIN,
  HOST
};

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

struct GroupLeaveCmd
{
  uint32_t groupId;
  uint32_t deviceId;
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

// Sync command structs for maintainability
struct HeartbeatCmd
{
  uint32_t deviceId;
};

struct GroupAnnounceCmd
{
  uint32_t groupId;
  uint32_t masterDeviceId;
};

struct GroupJoinCmd
{
  uint32_t groupId;
  uint32_t deviceId;
};

struct GroupInfoCmd
{
  uint32_t groupId;
  uint32_t masterDeviceId;
  uint8_t memberCount;
  // Members follow as GroupInfoMember array
};

struct GroupInfoMember
{
  uint32_t deviceId;
  uint8_t mac[6];
};

struct TimeRequestCmd
{
  uint32_t requestTimestamp;
};

struct TimeResponseCmd
{
  uint32_t requestTimestamp;
  uint32_t masterTimestamp;
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

  // Simplified mode system replacing auto-join and auto-create
  void setSyncMode(SyncMode mode);
  SyncMode getSyncMode();
  String getSyncModeString();
  String getSyncModeString(SyncMode mode);

  // Time sync
  void requestTimeSync();
  bool isTimeSynced() const;
  uint32_t getSyncedTime() const;
  int32_t getTimeOffset() const;

  static uint32_t syncMillis();

  // Effect sync
  void setEffectSyncState(const EffectSyncState &state);
  const EffectSyncState &getEffectSyncState() const;
  void enableEffectSync(bool enabled = true);
  bool isEffectSyncEnabled() const;

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
  void setEffectSyncCallback(
      std::function<void(const EffectSyncState &)> cb);

  // Debug/Info functions
  void printDeviceInfo();
  void printGroupInfo();
  void printSyncModeInfo();

  // senders
  void sendHeartbeat();
  void sendGroupAnnounce();
  void sendGroupInfo();
  void sendEffectState();

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
  void processGroupLeave(fullPacket *fp);
  void processTimeRequest(fullPacket *fp);
  void processTimeResponse(fullPacket *fp);
  void processEffectState(fullPacket *fp);

  // periodic tasks
  void checkDiscoveryCleanup(uint32_t now);
  void checkGroupCleanup(uint32_t now);
  void checkMemberTimeout(uint32_t now);
  void checkJoinMode(uint32_t now);

  // preferences management
  void loadPreferences();
  void saveSyncModePreferences();

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

  // Simplified sync mode system
  SyncMode syncMode = SyncMode::SOLO;

  // time‐sync
  bool timeSynced = false;
  int32_t timeOffset = 0;
  uint32_t lastTimeSync = 0;
  uint32_t lastTimeReq = 0;

  // effect sync
  bool effectSyncEnabled = true;
  EffectSyncState currentEffectState = {};
  uint32_t lastEffectSync = 0;

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
  std::function<void(const EffectSyncState &)> onEffectSync;

  // intervals/timeouts (ms)
  static constexpr uint32_t HEARTBEAT_INTERVAL = 1000;
  static constexpr uint32_t DISCOVERY_TIMEOUT = 6000;
  static constexpr uint32_t GROUP_ANNOUNCE_INTERVAL = 2000;
  static constexpr uint32_t GROUP_DISCOVERY_TIMEOUT = 6000;
  static constexpr uint32_t GROUP_INFO_INTERVAL = 2000;
  static constexpr uint32_t TIME_SYNC_INTERVAL = 10000;
  static constexpr uint32_t EFFECT_SYNC_INTERVAL = 1000;
  static constexpr uint32_t GROUP_MEMBER_TIMEOUT = 8000; // Time after which a member is considered inactive
};