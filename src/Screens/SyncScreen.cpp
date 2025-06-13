#include "SyncScreen.h"

SyncScreen::SyncScreen(String _name) : Screen(_name)
{
  syncMgr = SyncManager::getInstance();

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