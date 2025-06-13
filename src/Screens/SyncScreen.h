#pragma once

#include "config.h"
#include "IO/Display.h"
#include "IO/GPIO.h"
#include "IO/Menu.h"
#include "Sync/SyncManager.h"

// Sync management data structures (copied from example)
struct SyncDeviceInfo
{
  uint32_t deviceId;
  uint8_t mac[6];
  uint32_t lastSeen;
  uint32_t timeSinceLastSeen; // Calculated field in milliseconds
  bool inCurrentGroup;
  bool isGroupMaster;
  bool isThisDevice;
};

struct SyncDevicesResponse
{
  uint8_t deviceCount;
  uint32_t currentTime;      // Reference time for lastSeen calculations
  SyncDeviceInfo devices[8]; // Max 8 devices
};

struct SyncGroupInfo
{
  uint32_t groupId;
  uint32_t masterDeviceId;
  uint8_t masterMac[6];
  uint32_t lastSeen;
  uint32_t timeSinceLastSeen; // Calculated field in milliseconds
  bool isCurrentGroup;
  bool canJoin; // True if we're not in a group or this is not our current group
};

struct SyncGroupsResponse
{
  uint8_t groupCount;
  uint32_t currentTime;    // Reference time for lastSeen calculations
  uint32_t ourGroupId;     // Our current group ID (0 if none)
  SyncGroupInfo groups[4]; // Max 4 groups
};

struct SyncGroupMemberInfo
{
  uint32_t deviceId;
  uint8_t mac[6];
  bool isGroupMaster;
  bool isThisDevice;
  uint32_t lastHeartbeat; // 0 if unknown, otherwise time since last heartbeat
};

struct SyncCurrentGroupInfo
{
  uint32_t groupId;
  uint32_t masterDeviceId;
  bool isMaster;
  bool timeSynced;
  int32_t timeOffset;
  uint32_t syncedTime;
  uint8_t memberCount;
  uint32_t currentTime;           // Reference time
  SyncGroupMemberInfo members[6]; // Limit to 6 members to fit in packet
};

struct SyncDetailedStatus
{
  uint32_t deviceId;
  uint32_t groupId;
  uint32_t masterDeviceId;
  bool isMaster;
  bool timeSynced;
  int32_t timeOffset;
  uint32_t syncedTime;
  uint8_t memberCount;
  uint8_t discoveredDeviceCount;
  uint8_t discoveredGroupCount;
  bool autoJoinEnabled;
  bool autoCreateEnabled;
};

class SyncScreen : public Screen
{
public:
  SyncScreen(String _name);

  Menu menu = Menu(MenuSize::Medium);
  MenuItemBack backItem;

  // Main sync menu
  MenuItemAction syncRefreshItem = MenuItemAction("Refresh", 1, [&]()
                                                  { this->syncRefreshData(); });

  // This Device section
  MenuItemSubmenu syncThisDeviceUIItem = MenuItemSubmenu("This Device", &syncThisDeviceMenu);
  Menu syncThisDeviceMenu = Menu(MenuSize::Small);
  MenuItemBack syncThisDeviceBackItem;
  MenuItem thisDeviceIdItem = MenuItem("ID: ---");
  MenuItem thisDeviceGroupItem = MenuItem("Group: ---");
  MenuItem thisDeviceStatusItem = MenuItem("Status: ---");
  MenuItemToggle autoJoinItem = MenuItemToggle("Auto Join", &autoJoinEnabled, true);
  MenuItemToggle autoCreateItem = MenuItemToggle("Auto Create", &autoCreateEnabled, true);
  MenuItemAction leaveGroupItem = MenuItemAction("Leave Group", 1, [&]()
                                                 { this->leaveSyncGroup(); });

  // Current Group section
  MenuItemSubmenu syncCurrentGroupUIItem = MenuItemSubmenu("Current Group", &syncCurrentGroupMenu);
  Menu syncCurrentGroupMenu = Menu(MenuSize::Small);
  MenuItemBack syncCurrentGroupBackItem;
  MenuItem currentGroupIdItem = MenuItem("ID: ---");
  MenuItem currentGroupMasterItem = MenuItem("Master: ---");
  MenuItem currentGroupMembersItem = MenuItem("Members: ---");
  MenuItem currentGroupSyncItem = MenuItem("Sync: ---");

  // Group Members submenu (for current group)
  MenuItemSubmenu groupMembersUIItem = MenuItemSubmenu("View Members", &groupMembersMenu);
  Menu groupMembersMenu = Menu(MenuSize::Small);
  MenuItemBack groupMembersBackItem;

  // Discovered Devices section
  MenuItemSubmenu syncDevicesUIItem = MenuItemSubmenu("Devices", &syncDevicesMenu);
  Menu syncDevicesMenu = Menu(MenuSize::Medium);
  MenuItemBack syncDevicesBackItem;
  // Device items will be statically created (max 8)
  MenuItem deviceItems[8] = {
      MenuItem("Device 1: ---"),
      MenuItem("Device 2: ---"),
      MenuItem("Device 3: ---"),
      MenuItem("Device 4: ---"),
      MenuItem("Device 5: ---"),
      MenuItem("Device 6: ---"),
      MenuItem("Device 7: ---"),
      MenuItem("Device 8: ---")};

  // Device Detail submenu
  MenuItemSubmenu deviceDetailUIItem = MenuItemSubmenu("Device Detail", &deviceDetailMenu);
  Menu deviceDetailMenu = Menu(MenuSize::Small);
  MenuItemBack deviceDetailBackItem;
  MenuItem deviceDetailIdItem = MenuItem("ID: ---");
  MenuItem deviceDetailMacItem = MenuItem("MAC: ---");
  MenuItem deviceDetailLastSeenItem = MenuItem("Last: ---");
  MenuItem deviceDetailStatusItem = MenuItem("Status: ---");
  MenuItem deviceDetailGroupItem = MenuItem("Group: ---");

  // Discovered Groups section
  MenuItemSubmenu syncGroupsUIItem = MenuItemSubmenu("Groups", &syncGroupsMenu);
  Menu syncGroupsMenu = Menu(MenuSize::Medium);
  MenuItemBack syncGroupsBackItem;
  // Group items will be statically created (max 4)
  MenuItem groupItems[4] = {
      MenuItem("Group 1: ---"),
      MenuItem("Group 2: ---"),
      MenuItem("Group 3: ---"),
      MenuItem("Group 4: ---")};

  // Group Detail submenu
  MenuItemSubmenu groupDetailUIItem = MenuItemSubmenu("Group Detail", &groupDetailMenu);
  Menu groupDetailMenu = Menu(MenuSize::Small);
  MenuItemBack groupDetailBackItem;
  MenuItem groupDetailIdItem = MenuItem("ID: ---");
  MenuItem groupDetailMasterItem = MenuItem("Master: ---");
  MenuItem groupDetailMacItem = MenuItem("MAC: ---");
  MenuItem groupDetailLastSeenItem = MenuItem("Last: ---");
  MenuItem groupDetailStatusItem = MenuItem("Status: ---");
  MenuItemAction groupJoinItem = MenuItemAction("Join Group", 1, [&]()
                                                { this->joinSelectedGroup(); });

  // Group Management
  MenuItemAction syncCreateGroupItem = MenuItemAction("Create Group", 1, [&]()
                                                      { this->syncCreateGroup(); });

  void draw() override;
  void update() override;
  void onEnter() override;

  // Sync management methods
  void syncRefreshData(bool showNotification = true);
  void syncCreateGroup(uint32_t groupId = 0);
  void joinSelectedGroup();
  void leaveSyncGroup();

  // UI update methods
  void updateThisDeviceDisplay();
  void updateCurrentGroupDisplay();
  void updateDevicesDisplay();
  void updateGroupsDisplay();
  void updateGroupMembersDisplay();
  void showDeviceDetail(uint32_t deviceId);
  void showGroupDetail(uint32_t groupId);

  // Utility methods
  String formatMacAddress(const uint8_t *mac);
  String formatTimeDuration(uint32_t milliseconds);

private:
  SyncManager *syncMgr;

  // Sync data storage
  SyncDevicesResponse syncDevices;
  SyncGroupsResponse syncGroups;
  SyncCurrentGroupInfo syncCurrentGroup;
  SyncDetailedStatus syncStatus;

  // Selection tracking and state
  uint32_t selectedDeviceId = 0;
  uint32_t selectedGroupId = 0;
  bool autoJoinEnabled = false;
  bool autoCreateEnabled = false;

  // timing
  uint64_t lastSyncUpdate = 0;
  uint64_t lastSyncAutoRefresh = 0;

  // Helper methods
  void requestSyncDevices();
  void requestSyncGroups();
  void requestSyncGroupInfo();
  void requestSyncStatus();
  void joinSyncGroup(uint32_t groupId);
  void setAutoJoin(bool enabled);
  void setAutoCreate(bool enabled);
  void requestAutoJoinStatus();
  void requestAutoCreateStatus();
};