#pragma once

#include <Arduino.h>
#include <map>
#include <vector>
#include "IO/Wireless.h"

// Constants for sync packets
constexpr uint8_t SYNC_MSG_TYPE = 0xA0;
constexpr uint8_t SYNC_HEARTBEAT = 0x01;
constexpr uint8_t SYNC_MASTER_ANNOUNCE = 0x02;
constexpr uint8_t SYNC_EFFECT_STATE = 0x03;
constexpr uint8_t SYNC_MASTER_REQUEST = 0x04;
constexpr uint8_t SYNC_MASTER_RESIGN = 0x05;

// Broadcast MAC address
const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct DeviceInfo
{
  uint8_t mac[6];
  uint32_t lastSeen;
  uint32_t priority; // Higher number = higher priority
  bool isMaster;
};

struct EffectSyncState
{
  bool leftIndicator;
  bool rightIndicator;
  bool rgb;
  bool nightrider;
  bool startup;
  bool police;
  bool pulseWave;
  bool aurora;
  // Add other effects as needed
};

class SyncManager
{
public:
  static SyncManager *getInstance();

  void begin();
  void loop();

  // Set a callback for when effect states change due to sync
  void setEffectChangeCallback(std::function<void(const EffectSyncState &)> callback);

  // Update local effect states and notify other devices if we're master
  void updateEffectStates(const EffectSyncState &newState);

  // Check if we're currently in sync with other devices
  bool isSyncing() const;

  // Check if we're the master device
  bool isMaster() const;

  // Get known devices count
  size_t getDeviceCount() const;

private:
  SyncManager();
  ~SyncManager();

  // Handles incoming sync packets
  void handleSyncPacket(fullPacket *fp);

  // Periodic tasks
  void sendHeartbeat();
  void checkMasterStatus();
  void cleanupOldDevices();
  void electMaster();

  // Become the master device
  void becomeMaster();

  // Synchronize effect state as master
  void broadcastEffectState();

  // Check MAC equality
  bool macEqual(const uint8_t *mac1, const uint8_t *mac2);

  // Get our MAC address
  const uint8_t *getOurMac();

  // Map of known devices by MAC address
  std::map<std::string, DeviceInfo> knownDevices;

  // Current effect state
  EffectSyncState currentEffects;

  // Flags and timing
  bool isMasterDevice;
  bool syncActive;
  uint32_t lastHeartbeat;
  uint32_t lastEffectSync;
  uint32_t lastMasterCheck;
  uint32_t lastCleanup;
  uint32_t ourPriority; // Used for master election, generated at startup

  // Intervals (in ms)
  static constexpr uint32_t HEARTBEAT_INTERVAL = 1000;
  static constexpr uint32_t EFFECT_SYNC_INTERVAL = 500;
  static constexpr uint32_t MASTER_CHECK_INTERVAL = 2000;
  static constexpr uint32_t CLEANUP_INTERVAL = 5000;
  static constexpr uint32_t DEVICE_TIMEOUT = 5000; // Timeout for considering a device gone

  // Callback for effect state changes
  std::function<void(const EffectSyncState &)> effectChangeCallback;
};