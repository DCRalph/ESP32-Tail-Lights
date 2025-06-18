#include "SyncScreen.h"
#include "IO/Display.h"
#include "IO/GPIO.h"
#include "IO/Menu.h"
#include "Sync/SyncManager.h"
#include "IO/ScreenManager.h"

namespace SyncScreenNamespace
{
  // Sync state variables
  static SyncManager *syncMgr;

  // Selection tracking and state
  static uint32_t selectedDeviceId = 0;
  static uint32_t selectedGroupId = 0;
  // timing
  static uint64_t lastSyncUpdate = 0;
  static uint64_t lastSyncAutoRefresh = 0;

  // Forward declarations
  void syncRefreshData(bool showNotification = true);
  void syncCreateGroup(uint32_t groupId = 0);
  void joinSelectedGroup();
  void leaveSyncGroup();
  void updateThisDeviceDisplay();
  void updateCurrentGroupDisplay();
  void updateDevicesDisplay();
  void updateGroupsDisplay();
  void updateGroupMembersDisplay();
  void showDeviceDetail(uint32_t deviceId);
  void showGroupDetail(uint32_t groupId);
  String formatMacAddress(const uint8_t *mac);
  String formatTimeDuration(uint32_t milliseconds);
  void requestSyncDevices();
  void requestSyncGroups();
  void requestSyncGroupInfo();
  void requestSyncStatus();
  void joinSyncGroup(uint32_t groupId);
  void setSyncMode(SyncMode mode);
  void requestSyncModeStatus();

  // Menu instances
  static Menu menu = Menu(MenuSize::Medium);
  static Menu syncThisDeviceMenu = Menu(MenuSize::Small);
  static Menu syncCurrentGroupMenu = Menu(MenuSize::Small);
  static Menu groupMembersMenu = Menu(MenuSize::Small);
  static Menu syncDevicesMenu = Menu(MenuSize::Medium);
  static Menu deviceDetailMenu = Menu(MenuSize::Small);
  static Menu syncGroupsMenu = Menu(MenuSize::Medium);
  static Menu groupDetailMenu = Menu(MenuSize::Small);

  // Main menu items
  static MenuItemBack backItem;
  static MenuItemAction syncRefreshItem = MenuItemAction("Refresh", 1, []()
                                                         { syncRefreshData(); });

  // This Device section
  static MenuItemSubmenu syncThisDeviceUIItem = MenuItemSubmenu("This Device", &syncThisDeviceMenu);
  static MenuItemBack syncThisDeviceBackItem;
  static MenuItem thisDeviceIdItem = MenuItem("ID: ---");
  static MenuItem thisDeviceGroupItem = MenuItem("Group: ---");
  static MenuItem thisDeviceStatusItem = MenuItem("Status: ---");
  static std::vector<String> syncModeItems = {"Solo", "Join", "Host"};
  static int syncModeIndex = 0;
  static MenuItemSelect syncModeItem = MenuItemSelect("Mode", syncModeItems);
  static MenuItemAction leaveGroupItem = MenuItemAction("Leave Group", 1, []()
                                                        { leaveSyncGroup(); });

  // Current Group section
  static MenuItemSubmenu syncCurrentGroupUIItem = MenuItemSubmenu("Current Group", &syncCurrentGroupMenu);
  static MenuItemBack syncCurrentGroupBackItem;
  static MenuItem currentGroupIdItem = MenuItem("ID: ---");
  static MenuItem currentGroupMasterItem = MenuItem("Master: ---");
  static MenuItem currentGroupMembersItem = MenuItem("Members: ---");
  static MenuItem currentGroupSyncItem = MenuItem("Sync: ---");

  // Group Members submenu (for current group)
  static MenuItemSubmenu groupMembersUIItem = MenuItemSubmenu("View Members", &groupMembersMenu);
  static MenuItemBack groupMembersBackItem;

  // Discovered Devices section
  static MenuItemSubmenu syncDevicesUIItem = MenuItemSubmenu("Devices", &syncDevicesMenu);
  static MenuItemBack syncDevicesBackItem;
  // Device items will be statically created (max 8)
  static MenuItem deviceItems[8] = {
      MenuItem("Device 1: ---"),
      MenuItem("Device 2: ---"),
      MenuItem("Device 3: ---"),
      MenuItem("Device 4: ---"),
      MenuItem("Device 5: ---"),
      MenuItem("Device 6: ---"),
      MenuItem("Device 7: ---"),
      MenuItem("Device 8: ---")};

  // Device Detail submenu
  static MenuItemSubmenu deviceDetailUIItem = MenuItemSubmenu("Device Detail", &deviceDetailMenu);
  static MenuItemBack deviceDetailBackItem;
  static MenuItem deviceDetailIdItem = MenuItem("ID: ---");
  static MenuItem deviceDetailMacItem = MenuItem("MAC: ---");
  static MenuItem deviceDetailLastSeenItem = MenuItem("Last: ---");
  static MenuItem deviceDetailStatusItem = MenuItem("Status: ---");
  static MenuItem deviceDetailGroupItem = MenuItem("Group: ---");

  // Discovered Groups section
  static MenuItemSubmenu syncGroupsUIItem = MenuItemSubmenu("Groups", &syncGroupsMenu);
  static MenuItemBack syncGroupsBackItem;
  // Group items will be statically created (max 4)
  static MenuItem groupItems[4] = {
      MenuItem("Group 1: ---"),
      MenuItem("Group 2: ---"),
      MenuItem("Group 3: ---"),
      MenuItem("Group 4: ---")};

  // Group Detail submenu
  static MenuItemSubmenu groupDetailUIItem = MenuItemSubmenu("Group Detail", &groupDetailMenu);
  static MenuItemBack groupDetailBackItem;
  static MenuItem groupDetailIdItem = MenuItem("ID: ---");
  static MenuItem groupDetailMasterItem = MenuItem("Master: ---");
  static MenuItem groupDetailMacItem = MenuItem("MAC: ---");
  static MenuItem groupDetailLastSeenItem = MenuItem("Last: ---");
  static MenuItem groupDetailStatusItem = MenuItem("Status: ---");
  static MenuItemAction groupJoinItem = MenuItemAction("Join Group", 1, []()
                                                       { joinSelectedGroup(); });

  // Group Management
  static MenuItemAction syncCreateGroupItem = MenuItemAction("Create Group", 1, []()
                                                             { syncCreateGroup(); });

  // onEnter function
  void syncScreenOnEnter()
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
    syncThisDeviceMenu.addMenuItem(&syncModeItem);
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
    syncThisDeviceUIItem.addFunc(1, []()
                                 {
      requestSyncStatus();
      requestSyncModeStatus();
      updateThisDeviceDisplay(); });

    syncCurrentGroupUIItem.addFunc(1, []()
                                   {
      requestSyncGroupInfo();
      updateCurrentGroupDisplay(); });

    syncDevicesUIItem.addFunc(1, []()
                              {
      requestSyncDevices();
      updateDevicesDisplay(); });

    syncGroupsUIItem.addFunc(1, []()
                             {
      requestSyncGroups();
      updateGroupsDisplay(); });

    groupMembersUIItem.addFunc(1, []()
                               { updateGroupMembersDisplay(); });

    // Set up device selection callbacks
    for (int i = 0; i < 8; i++)
    {
      deviceItems[i].addFunc(1, [i]()
                             {
                               // This is simplified - would need device list management
                               showDeviceDetail(i + 1000); // Dummy device ID
                             });
    }

    // Set up group selection callbacks
    for (int i = 0; i < 4; i++)
    {
      groupItems[i].addFunc(1, [i]()
                            {
                              // This is simplified - would need group list management
                              showGroupDetail(i + 2000); // Dummy group ID
                            });
    }

    syncMgr = SyncManager::getInstance();
    syncModeIndex = static_cast<int>(syncMgr->getSyncMode());
    syncModeItem.setCurrentIndex(syncModeIndex);
    syncRefreshData();
  }

  // onExit function
  void syncScreenOnExit()
  {
    // Clean up allocated memory
  }

  // draw function
  void syncScreenDraw()
  {
    menu.draw();
  }

  // update function
  void syncScreenUpdate()
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

  // Helper method implementations
  void syncRefreshData(bool showNotification)
  {
    requestSyncStatus();
    requestSyncDevices();
    requestSyncGroups();
    requestSyncGroupInfo();
    requestSyncModeStatus();

    if (showNotification)
    {
      display.showNotification("Refreshing...", 1000);
    }
  }

  void syncCreateGroup(uint32_t groupId)
  {
    if (syncMgr->getGroupId() != 0)
    {
      display.showNotification("Leave group first", 1500);
      return;
    }

    syncMgr->createGroup(groupId);
    display.showNotification("Creating group...", 1500);
  }

  void leaveSyncGroup()
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

  void joinSelectedGroup()
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

  void joinSyncGroup(uint32_t groupId)
  {
    if (syncMgr->getGroupId() != 0)
    {
      syncMgr->leaveGroup();
    }
    syncMgr->joinGroup(groupId);
  }

  void updateThisDeviceDisplay()
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

    syncModeItem.setCurrentIndex(static_cast<int>(syncMgr->getSyncMode()));
  }

  void updateCurrentGroupDisplay()
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

  void updateDevicesDisplay()
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

  void updateGroupsDisplay()
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

  void updateGroupMembersDisplay()
  {
    // Simplified implementation
    display.showNotification("Group members view", 1000);
  }

  void showDeviceDetail(uint32_t deviceId)
  {
    selectedDeviceId = deviceId;
    deviceDetailIdItem.setName("ID: " + String(deviceId, HEX));
    deviceDetailMacItem.setName("MAC: ---");
    deviceDetailLastSeenItem.setName("Last: ---");
    deviceDetailStatusItem.setName("Status: Unknown");
    deviceDetailGroupItem.setName("Group: Unknown");
  }

  void showGroupDetail(uint32_t groupId)
  {
    selectedGroupId = groupId;
    groupDetailIdItem.setName("ID: " + String(groupId, HEX));
    groupDetailMasterItem.setName("Master: ---");
    groupDetailMacItem.setName("MAC: ---");
    groupDetailLastSeenItem.setName("Last: ---");
    groupDetailStatusItem.setName("Status: Available");
  }

  String formatMacAddress(const uint8_t *mac)
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

  String formatTimeDuration(uint32_t milliseconds)
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

  void requestSyncDevices()
  {
    // Request updated device list from SyncManager
    // The actual implementation would depend on how SyncManager exposes this
  }

  void requestSyncGroups()
  {
    // Request updated group list from SyncManager
  }

  void requestSyncGroupInfo()
  {
    // Request current group info from SyncManager
  }

  void requestSyncStatus()
  {
    // Request detailed status from SyncManager
  }

  void setSyncMode(SyncMode mode)
  {
    syncMgr->setSyncMode(mode);
    display.showNotification(String("Mode: ") + syncMgr->getSyncModeString(), 1500);
  }

  void requestSyncModeStatus()
  {
    // Current sync mode is always available from syncMgr->getSyncMode()
    // No need to cache it like the old auto-join/auto-create bools
  }

} // namespace SyncScreenNamespace

// Define the SyncScreen Screen2 instance
const Screen2 SyncScreen = {
    F("Sync Manager"),
    F("Sync Manager"),
    SyncScreenNamespace::syncScreenDraw,
    SyncScreenNamespace::syncScreenUpdate,
    SyncScreenNamespace::syncScreenOnEnter,
    SyncScreenNamespace::syncScreenOnExit};