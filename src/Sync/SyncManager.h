#pragma once

#include <Arduino.h>
#include <map>
#include <vector>
#include <functional>
#include "IO/Wireless.h"
#include "config.h"

// Constants for sync packets
constexpr uint8_t SYNC_MSG_TYPE = 0xA0;  // main msg type
constexpr uint8_t SYNC_HEARTBEAT = 0x01; // rest are subtypes
constexpr uint8_t SYNC_MASTER_ANNOUNCE = 0x02;
constexpr uint8_t SYNC_TIME_REQUEST = 0x03;
constexpr uint8_t SYNC_TIME_RESPONSE = 0x04;
constexpr uint8_t SYNC_GROUP_JOIN = 0x05;
constexpr uint8_t SYNC_GROUP_ANNOUNCE = 0x06;
constexpr uint8_t SYNC_DEVICE_INFO = 0x07;
constexpr uint8_t SYNC_MASTER_REQUEST = 0x08;

// Broadcast MAC address
const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct DeviceInfo
{
  uint32_t deviceId;   // Unique device identifier
  uint8_t mac[6];      // MAC address
  uint32_t lastSeen;   // Last time we heard from this device
  uint32_t priority;   // Higher number = higher priority for master election
  bool isMaster;       // Is this device currently the master
  uint32_t syncedTime; // Last synchronized time from this device
  uint32_t groupId;    // Group ID this device belongs to
  int8_t timeOffset;   // Time offset from master (in ms)
};

struct SyncNetworkInfo
{
  uint32_t masterDeviceId;
  uint32_t groupId;
  uint32_t syncedTime;
  size_t deviceCount;
  bool isTimeSynced;
  uint32_t avgTimeOffset;
};

class SyncManager
{
public:
  static SyncManager *getInstance();

  void begin();
  void loop();

  // Group management
  void createGroup(uint32_t groupId = 0); // 0 = auto-generate
  void joinGroup(uint32_t groupId);
  void leaveGroup();
  uint32_t getGroupId() const;

  // Auto-join functionality
  void enableAutoJoin(bool enabled = true);
  void setAutoJoinTimeout(uint32_t timeoutMs = 10000); // Wait 10s before creating own group
  bool isAutoJoinEnabled() const;

  // Time synchronization
  void requestTimeSync();
  uint32_t getSyncedTime() const;
  bool isTimeSynced() const;
  int32_t getTimeOffset() const;

  // Device management
  bool isSyncing() const;
  bool isMaster() const;
  uint32_t getDeviceId() const;
  size_t getDeviceCount() const;
  std::vector<DeviceInfo> getKnownDevices() const;

  // Network information
  SyncNetworkInfo getNetworkInfo() const;

  // Callbacks
  void setDeviceJoinCallback(std::function<void(const DeviceInfo &)> callback);
  void setDeviceLeaveCallback(std::function<void(uint32_t deviceId)> callback);
  void setMasterChangeCallback(std::function<void(uint32_t newMasterDeviceId)> callback);
  void setTimeSyncCallback(std::function<void(uint32_t syncedTime)> callback);

  // Debug and monitoring
  void printDeviceInfo();
  void printNetworkStatus();
  void printGroupInfo();

  // Synchronized LED control
  void updateSyncedLED();

private:
  SyncManager();
  ~SyncManager();

#ifdef ENABLE_SYNC
  // Core sync handlers
  void handleSyncPacket(fullPacket *fp);
  void processHeartbeat(fullPacket *fp);
  void processMasterAnnounce(fullPacket *fp);
  void processTimeRequest(fullPacket *fp);
  void processTimeResponse(fullPacket *fp);
  void processGroupJoin(fullPacket *fp);
  void processGroupAnnounce(fullPacket *fp);
  void processDeviceInfo(fullPacket *fp);

  // Periodic tasks
  void sendHeartbeat();
  void sendDeviceInfo();
  void checkMasterStatus();
  void cleanupOldDevices();
  void electMaster();
  void synchronizeTime();

  // Master operations
  void becomeMaster();
  void resignMaster();
  void broadcastGroupInfo();
  void respondToTimeRequest(const uint8_t *requestorMac);

  // Utility functions
  bool macEqual(const uint8_t *mac1, const uint8_t *mac2);
  const uint8_t *getOurMac();
  std::string macToString(const uint8_t *mac);
  uint32_t generateDeviceId();
  uint32_t generateGroupId();
  void updateDeviceFromPacket(const std::string &macStr, fullPacket *fp);

  // Auto-join functionality
  void checkAutoJoin();
  void attemptAutoJoin();
  uint32_t findBestGroupToJoin();

  // Device state
  std::map<std::string, DeviceInfo> knownDevices;
  uint32_t ourDeviceId;
  uint32_t ourPriority;
  uint32_t currentGroupId;

  // Auto-join configuration
  bool autoJoinEnabled;
  uint32_t autoJoinTimeout;
  uint32_t autoJoinStartTime;

  // Master and sync state
  bool isMasterDevice;
  bool syncActive;
  bool timeIsSynced;
  uint32_t masterSyncTime;
  int32_t ourTimeOffset;
  uint32_t lastTimeSyncRequest;

  // Timing
  uint32_t lastHeartbeat;
  uint32_t lastDeviceInfo;
  uint32_t lastMasterCheck;
  uint32_t lastCleanup;
  uint32_t lastTimeSync;
  uint64_t lastPrintTime;

  // Intervals (in ms)
  static constexpr uint32_t HEARTBEAT_INTERVAL = 1000;
  static constexpr uint32_t DEVICE_INFO_INTERVAL = 2000;
  static constexpr uint32_t MASTER_CHECK_INTERVAL = 3000;
  static constexpr uint32_t CLEANUP_INTERVAL = 5000;
  static constexpr uint32_t TIME_SYNC_INTERVAL = 10000;
  static constexpr uint32_t DEVICE_TIMEOUT = 6000;
  static constexpr uint32_t TIME_SYNC_TIMEOUT = 2000;

  // Callbacks
  std::function<void(const DeviceInfo &)> deviceJoinCallback;
  std::function<void(uint32_t)> deviceLeaveCallback;
  std::function<void(uint32_t)> masterChangeCallback;
  std::function<void(uint32_t)> timeSyncCallback;
#endif // ENABLE_SYNC
};