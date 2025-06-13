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

SyncScreen::SyncScreen(String _name) : Screen(_name)
{

  // Setup main sync menu
  menu.addMenuItem(&backItem);
  menu.addMenuItem(&syncRefreshItem);
  menu.addMenuItem(&syncThisDeviceUIItem);
  menu.addMenuItem(&syncCurrentGroupUIItem);
  menu.addMenuItem(&syncDevicesUIItem);
  menu.addMenuItem(&syncGroupsUIItem);
  menu.addMenuItem(&syncCreateGroupItem);

  // Setup This Device submenu
  syncThisDeviceMenu.addMenuItem(&syncThisDeviceBackItem);
  syncThisDeviceMenu.addMenuItem(&thisDeviceIdItem);
  syncThisDeviceMenu.addMenuItem(&thisDeviceGroupItem);
  syncThisDeviceMenu.addMenuItem(&thisDeviceStatusItem);
  syncThisDeviceMenu.addMenuItem(&autoJoinItem);
  syncThisDeviceMenu.addMenuItem(&autoCreateItem);
  syncThisDeviceMenu.addMenuItem(&leaveGroupItem);
  syncThisDeviceMenu.setParentMenu(&menu);

  // Setup Current Group submenu
  syncCurrentGroupMenu.addMenuItem(&syncCurrentGroupBackItem);
  syncCurrentGroupMenu.addMenuItem(&currentGroupIdItem);
  syncCurrentGroupMenu.addMenuItem(&currentGroupMasterItem);
  syncCurrentGroupMenu.addMenuItem(&currentGroupMembersItem);
  syncCurrentGroupMenu.addMenuItem(&currentGroupSyncItem);
  syncCurrentGroupMenu.addMenuItem(&groupMembersUIItem);
  syncCurrentGroupMenu.setParentMenu(&menu);

  // Setup Group Members submenu
  groupMembersMenu.addMenuItem(&groupMembersBackItem);
  groupMembersMenu.setParentMenu(&syncCurrentGroupMenu);

  // Setup devices submenu
  syncDevicesMenu.addMenuItem(&syncDevicesBackItem);
  for (int i = 0; i < 8; i++)
  {
    syncDevicesMenu.addMenuItem(&deviceItems[i]);
  }
  syncDevicesMenu.setParentMenu(&menu);

  // Setup device detail submenu
  deviceDetailMenu.addMenuItem(&deviceDetailBackItem);
  deviceDetailMenu.addMenuItem(&deviceDetailIdItem);
  deviceDetailMenu.addMenuItem(&deviceDetailMacItem);
  deviceDetailMenu.addMenuItem(&deviceDetailLastSeenItem);
  deviceDetailMenu.addMenuItem(&deviceDetailStatusItem);
  deviceDetailMenu.addMenuItem(&deviceDetailGroupItem);
  deviceDetailMenu.setParentMenu(&syncDevicesMenu);

  // Setup groups submenu
  syncGroupsMenu.addMenuItem(&syncGroupsBackItem);
  for (int i = 0; i < 4; i++)
  {
    syncGroupsMenu.addMenuItem(&groupItems[i]);
  }
  syncGroupsMenu.setParentMenu(&menu);

  // Setup group detail submenu
  groupDetailMenu.addMenuItem(&groupDetailBackItem);
  groupDetailMenu.addMenuItem(&groupDetailIdItem);
  groupDetailMenu.addMenuItem(&groupDetailMasterItem);
  groupDetailMenu.addMenuItem(&groupDetailMacItem);
  groupDetailMenu.addMenuItem(&groupDetailLastSeenItem);
  groupDetailMenu.addMenuItem(&groupDetailStatusItem);
  groupDetailMenu.addMenuItem(&groupJoinItem);
  groupDetailMenu.setParentMenu(&syncGroupsMenu);

  // Set up callbacks
  syncThisDeviceUIItem.addFunc(1, [&]()
                               { 
                                 requestSyncStatus();
                                 requestAutoJoinStatus();
                                 requestAutoCreateStatus();
                                 updateThisDeviceDisplay(); });

  syncCurrentGroupUIItem.addFunc(1, [&]()
                                 { 
                                   requestSyncGroupInfo();
                                   updateCurrentGroupDisplay(); });

  syncDevicesUIItem.addFunc(1, [&]()
                            { 
                             requestSyncDevices(); 
                             updateDevicesDisplay(); });

  syncGroupsUIItem.addFunc(1, [&]()
                           { 
                            requestSyncGroups(); 
                            updateGroupsDisplay(); });

  groupMembersUIItem.addFunc(1, [&]()
                             { updateGroupMembersDisplay(); });

  // Set up auto join/create callbacks
  autoJoinItem.setOnChange([&]()
                           { setAutoJoin(autoJoinEnabled); });

  autoCreateItem.setOnChange([&]()
                             { setAutoCreate(autoCreateEnabled); });

  // Set up device selection callbacks
  for (int i = 0; i < 8; i++)
  {
    deviceItems[i].addFunc(1, [this, i]()
                           { 
                             if (i < syncDevices.deviceCount && !deviceItems[i].isHidden()) {
                               showDeviceDetail(syncDevices.devices[i].deviceId);
                             } });
  }

  // Set up group selection callbacks
  for (int i = 0; i < 4; i++)
  {
    groupItems[i].addFunc(1, [this, i]()
                          { 
                            if (i < syncGroups.groupCount && !groupItems[i].isHidden()) {
                              showGroupDetail(syncGroups.groups[i].groupId);
                            } });
  }
}

void SyncScreen::draw()
{
  menu.draw();
}

void SyncScreen::update()
{
  menu.update();

  // Auto-refresh sync data when sync menus are active
  if (millis() - lastSyncAutoRefresh > 2000)
  {
    lastSyncAutoRefresh = millis();
    // Check if any sync-related menu is currently active
    Menu *currentMenu = menu.getActiveSubmenu();
    bool syncMenuActive = (currentMenu == &menu);

    if (syncMenuActive)
    {
      syncRefreshData(false); // Don't show notification for auto-refresh
    }
  }
}

void SyncScreen::onEnter()
{
  syncMgr = SyncManager::getInstance();

  syncRefreshData();
}

// Sync management methods
void SyncScreen::syncRefreshData(bool showNotification)
{
  requestSyncStatus();
  requestSyncDevices();
  requestSyncGroups();
  requestSyncGroupInfo();
  requestAutoJoinStatus();
  requestAutoCreateStatus();

  if (showNotification)
  {
    display.showNotification("Refreshing...", 1000);
  }
}

void SyncScreen::syncCreateGroup(uint32_t groupId)
{
  if (syncMgr->getGroupId() != 0)
  {
    display.showNotification("Leave group first", 1500);
    return;
  }

  syncMgr->createGroup(groupId);
  display.showNotification("Creating group...", 1500);
}

void SyncScreen::leaveSyncGroup()
{
  if (syncMgr->getGroupId() != 0)
  {
    syncMgr->leaveGroup();
    display.showNotification("Leaving group...", 1500);
  }
  else
  {
    display.showNotification("Not in group", 1500);
  }
}

void SyncScreen::joinSelectedGroup()
{
  if (selectedGroupId != 0)
  {
    joinSyncGroup(selectedGroupId);
    display.showNotification("Joining group...", 1500);
  }
  else
  {
    display.showNotification("No group selected", 1500);
  }
}

void SyncScreen::joinSyncGroup(uint32_t groupId)
{
  if (syncMgr->getGroupId() != 0)
  {
    syncMgr->leaveGroup();
  }
  syncMgr->joinGroup(groupId);
}

// UI update methods
void SyncScreen::updateThisDeviceDisplay()
{
  const GroupInfo &groupInfo = syncMgr->getGroupInfo();

  thisDeviceIdItem.setName("ID: " + String(syncMgr->getDeviceId(), HEX));

  if (groupInfo.groupId == 0)
  {
    thisDeviceGroupItem.setName("Group: None");
    thisDeviceStatusItem.setName("Status: Solo");
  }
  else
  {
    thisDeviceGroupItem.setName("Group: " + String(groupInfo.groupId, HEX));
    String statusText = syncMgr->isGroupMaster() ? "Master" : "Member";
    if (syncMgr->isTimeSynced())
    {
      statusText += " (Synced)";
    }
    thisDeviceStatusItem.setName("Status: " + statusText);
  }

  autoJoinEnabled = syncMgr->isAutoJoinEnabled();
  autoCreateEnabled = syncMgr->isAutoCreateEnabled();
}

void SyncScreen::updateCurrentGroupDisplay()
{
  const GroupInfo &groupInfo = syncMgr->getGroupInfo();

  if (groupInfo.groupId == 0)
  {
    currentGroupIdItem.setName("ID: None");
    currentGroupMasterItem.setName("Master: ---");
    currentGroupMembersItem.setName("Members: 0");
    currentGroupSyncItem.setName("Sync: ---");
    groupMembersUIItem.setHidden(true);
  }
  else
  {
    currentGroupIdItem.setName("ID: " + String(groupInfo.groupId, HEX));
    currentGroupMasterItem.setName("Master: " + String(groupInfo.masterDeviceId, HEX));
    currentGroupMembersItem.setName("Members: " + String(groupInfo.members.size()));

    String syncText = syncMgr->isTimeSynced() ? "OK" : "No";
    if (syncMgr->isTimeSynced() && syncMgr->getTimeOffset() != 0)
    {
      syncText += " (" + String(syncMgr->getTimeOffset()) + "ms)";
    }
    currentGroupSyncItem.setName("Sync: " + syncText);
    groupMembersUIItem.setHidden(groupInfo.members.size() == 0);
  }
}

void SyncScreen::updateDevicesDisplay()
{
  auto discoveredDevices = syncMgr->getDiscoveredDevices();

  // Update device items - only show populated ones
  int deviceIndex = 0;
  for (const auto &devicePair : discoveredDevices)
  {
    if (deviceIndex >= 8)
      break;

    const DiscoveredDevice &device = devicePair.second;
    String deviceText = String(device.deviceId, HEX);

    // Add status indicators
    uint32_t timeSince = millis() - device.lastSeen;
    deviceText += " - " + formatTimeDuration(timeSince);

    deviceItems[deviceIndex].setName(deviceText);
    deviceItems[deviceIndex].setHidden(false);
    deviceIndex++;
  }

  // Hide remaining items
  for (int i = deviceIndex; i < 8; i++)
  {
    deviceItems[i].setHidden(true);
  }
}

void SyncScreen::updateGroupsDisplay()
{
  auto discoveredGroups = syncMgr->getDiscoveredGroups();

  // Update group items - only show populated ones
  int groupIndex = 0;
  for (const auto &group : discoveredGroups)
  {
    if (groupIndex >= 4)
      break;

    String groupText = "Group " + String(group.groupId, HEX);

    if (group.groupId == syncMgr->getGroupId())
    {
      groupText += " (Current)";
    }
    else
    {
      groupText += " (Available)";
    }

    uint32_t timeSince = millis() - group.lastSeen;
    groupText += " - " + formatTimeDuration(timeSince);

    groupItems[groupIndex].setName(groupText);
    groupItems[groupIndex].setHidden(false);
    groupIndex++;
  }

  // Hide remaining items
  for (int i = groupIndex; i < 4; i++)
  {
    groupItems[i].setHidden(true);
  }
}

void SyncScreen::updateGroupMembersDisplay()
{
  // Clear existing member items (except back button)
  while (groupMembersMenu.items.size() > 1)
  {
    groupMembersMenu.items.pop_back();
  }

  const GroupInfo &groupInfo = syncMgr->getGroupInfo();

  if (groupInfo.groupId == 0 || groupInfo.members.size() == 0)
  {
    return;
  }

  // Add member items dynamically for current group
  // Note: This is simplified - in practice you'd want to manage dynamic items more carefully
}

void SyncScreen::showDeviceDetail(uint32_t deviceId)
{
  selectedDeviceId = deviceId;

  auto discoveredDevices = syncMgr->getDiscoveredDevices();

  // Find device by ID
  const DiscoveredDevice *device = nullptr;
  for (const auto &devicePair : discoveredDevices)
  {
    if (devicePair.second.deviceId == deviceId)
    {
      device = &devicePair.second;
      break;
    }
  }

  if (device != nullptr)
  {
    deviceDetailIdItem.setName("ID: " + String(device->deviceId, HEX));
    deviceDetailMacItem.setName(formatMacAddress(device->mac));

    uint32_t timeSince = millis() - device->lastSeen;
    deviceDetailLastSeenItem.setName("Last: " + formatTimeDuration(timeSince));

    deviceDetailStatusItem.setName("Status: Other Device");
    deviceDetailGroupItem.setName("Group: Unknown");

    // Activate the device detail submenu
    syncDevicesMenu.setActiveSubmenu(&deviceDetailMenu);
  }
}

void SyncScreen::showGroupDetail(uint32_t groupId)
{
  selectedGroupId = groupId;

  auto discoveredGroups = syncMgr->getDiscoveredGroups();

  for (const auto &group : discoveredGroups)
  {
    if (group.groupId == groupId)
    {
      groupDetailIdItem.setName("ID: " + String(group.groupId, HEX));
      groupDetailMasterItem.setName("Master: " + String(group.masterDeviceId, HEX));
      groupDetailMacItem.setName(formatMacAddress(group.masterMac));

      uint32_t timeSince = millis() - group.lastSeen;
      groupDetailLastSeenItem.setName("Last: " + formatTimeDuration(timeSince));

      bool isCurrentGroup = (group.groupId == syncMgr->getGroupId());
      String statusText = isCurrentGroup ? "Current Group" : "Available";
      groupDetailStatusItem.setName("Status: " + statusText);

      // Show/hide join button based on availability
      groupJoinItem.setHidden(isCurrentGroup);

      // Activate the group detail submenu
      syncGroupsMenu.setActiveSubmenu(&groupDetailMenu);
      break;
    }
  }
}

// Utility methods
String SyncScreen::formatMacAddress(const uint8_t *mac)
{
  String result = "";
  for (int i = 0; i < 6; i++)
  {
    if (i > 0)
      result += ":";
    if (mac[i] < 16)
      result += "0";
    result += String(mac[i], HEX);
  }
  result.toUpperCase();
  return result;
}

String SyncScreen::formatTimeDuration(uint32_t milliseconds)
{
  if (milliseconds < 1000)
    return String(milliseconds) + "ms";
  else if (milliseconds < 60000)
    return String(milliseconds / 1000) + "s";
  else if (milliseconds < 3600000)
    return String(milliseconds / 60000) + "m";
  else
    return String(milliseconds / 3600000) + "h";
}

// Helper methods
void SyncScreen::requestSyncDevices()
{
  // Request updated device list from SyncManager
  // The actual implementation would depend on how SyncManager exposes this
}

void SyncScreen::requestSyncGroups()
{
  // Request updated group list from SyncManager
}

void SyncScreen::requestSyncGroupInfo()
{
  // Request current group info from SyncManager
}

void SyncScreen::requestSyncStatus()
{
  // Request detailed status from SyncManager
}

void SyncScreen::setAutoJoin(bool enabled)
{
  syncMgr->enableAutoJoin(enabled);
  display.showNotification("Auto Join updated", 1000);
}

void SyncScreen::setAutoCreate(bool enabled)
{
  syncMgr->enableAutoCreate(enabled);
  display.showNotification("Auto Create updated", 1000);
}

void SyncScreen::requestAutoJoinStatus()
{
  autoJoinEnabled = syncMgr->isAutoJoinEnabled();
}

void SyncScreen::requestAutoCreateStatus()
{
  autoCreateEnabled = syncMgr->isAutoCreateEnabled();
}