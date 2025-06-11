/****************************************************
 * syncMenu.cpp
 *
 * Defines the syncMenu object and implements its
 * printMenu/handleInput for managing SyncManager.
 ****************************************************/

#include "syncMenu.h"
#include "mainMenu.h"
#include "SerialMenu.h"
#include "Sync/SyncManager.h"
#include <Arduino.h>
#include <map>
#include <vector>

// Store available groups for numbered selection
static std::vector<uint32_t> availableGroups;

Menu syncMenu = {
    "Sync Manager",
    &mainMenu,
    printSyncMenu,
    handleSyncMenuInput};

void printSyncMenu(const Menu &menu)
{
  Serial.println(String(F("\nBreadcrumb: ")) + buildBreadcrumbString(&menu));
  Serial.println(F("=== SYNC MANAGER ==="));

  SyncManager *syncMgr = SyncManager::getInstance();

  // Show current status
  Serial.println(F("Current Status:"));
  if (syncMgr->getGroupId() != 0)
  {
    Serial.println(String(F("  Group: 0x")) + String(syncMgr->getGroupId(), HEX));
    Serial.println(String(F("  Role: ")) + (syncMgr->isGroupMaster() ? "MASTER" : "SLAVE"));

    const GroupInfo &groupInfo = syncMgr->getGroupInfo();
    Serial.println(String(F("  Devices: ")) + String(groupInfo.members.size()));
    Serial.println(String(F("  Time Synced: ")) + (syncMgr->isTimeSynced() ? "YES" : "NO"));
  }
  else
  {
    Serial.println(F("  No group joined"));
    Serial.println(String(F("  Auto-join: ")) + (syncMgr->isAutoJoinEnabled() ? "ENABLED" : "DISABLED"));
    Serial.println(String(F("  Auto-create: ")) + (syncMgr->isAutoCreateEnabled() ? "ENABLED" : "DISABLED"));
  }

  Serial.println();
  Serial.println(F("Actions:"));
  Serial.println(F("1) Show detailed status"));
  Serial.println(F("2) Show available groups"));
  Serial.println(F("3) Create new group"));
  Serial.println(F("4) Join group (by number)"));
  Serial.println(F("5) Leave current group"));
  Serial.println(F("6) List known devices"));
  Serial.println(F("7) Toggle auto-join"));
  Serial.println(F("8) Refresh group list"));
  Serial.println(F("9) Print device info"));
  Serial.println(F("10) Print group info"));
  Serial.println(F("11) Toggle auto-create"));
  Serial.println(F("b) Back to main menu"));
  Serial.println(F("Press Enter to refresh this menu"));
}

bool handleSyncMenuInput(Menu &menu, const String &input)
{
  SyncManager *syncMgr = SyncManager::getInstance();

  if (input == F("1"))
  {
    showSyncStatus();
    return true;
  }
  else if (input == F("2"))
  {
    showAvailableGroups();
    return true;
  }
  else if (input == F("3"))
  {
    createNewGroup();
    return true;
  }
  else if (input == F("4"))
  {
    if (availableGroups.empty())
    {
      Serial.println(F("No groups available. Use option 2 to scan for groups first."));
    }
    else
    {
      Serial.println("Enter group number (1-" + String(availableGroups.size()) + "):");
      Serial.print(F("> "));

      // Wait for input
      while (!Serial.available())
      {
        delay(10);
      }
      String groupInput = Serial.readStringUntil('\n');
      groupInput.trim();

      int groupNum = groupInput.toInt();
      if (groupNum >= 1 && groupNum <= (int)availableGroups.size())
      {
        joinGroupByNumber(groupNum - 1); // Convert to 0-based index
      }
      else
      {
        Serial.println(F("Invalid group number."));
      }
    }
    return true;
  }
  else if (input == F("5"))
  {
    if (syncMgr->getGroupId() != 0)
    {
      Serial.println(F("Leaving current group..."));
      syncMgr->leaveGroup();
      Serial.println(F("Left group successfully."));
    }
    else
    {
      Serial.println(F("Not currently in a group."));
    }
    return true;
  }
  else if (input == F("6"))
  {
    listKnownDevices();
    return true;
  }
  else if (input == F("7"))
  {
    bool currentState = syncMgr->isAutoJoinEnabled();
    syncMgr->enableAutoJoin(!currentState);
    Serial.println(String(F("Auto-join ")) + (!currentState ? "ENABLED" : "DISABLED"));
    return true;
  }
  else if (input == F("8"))
  {
    Serial.println(F("Refreshing group list..."));
    showAvailableGroups();
    return true;
  }
  else if (input == F("9"))
  {
    syncMgr->printDeviceInfo();
    return true;
  }
  else if (input == F("10"))
  {
    syncMgr->printGroupInfo();
    return true;
  }
  else if (input == F("11"))
  {
    bool currentState = syncMgr->isAutoCreateEnabled();
    syncMgr->enableAutoCreate(!currentState);
    Serial.println(String(F("Auto-create ")) + (!currentState ? "ENABLED" : "DISABLED"));
    return true;
  }
  else if (input == F("b"))
  {
    extern Menu mainMenu;
    setMenu(&mainMenu);
    return true;
  }

  return false;
}

void showSyncStatus()
{
  Serial.println(F("\n=== DETAILED SYNC STATUS ==="));

  SyncManager *syncMgr = SyncManager::getInstance();
  const GroupInfo &groupInfo = syncMgr->getGroupInfo();

  Serial.println(String(F("Device ID: 0x")) + String(syncMgr->getDeviceId(), HEX));
  Serial.println(String(F("Group ID: ")) + (groupInfo.groupId != 0 ? ("0x" + String(groupInfo.groupId, HEX)) : "None"));
  Serial.println(String(F("Role: ")) + (syncMgr->isGroupMaster() ? "MASTER" : "SLAVE"));
  Serial.println(String(F("Group Members: ")) + String(groupInfo.members.size()));
  Serial.println(String(F("Time Synced: ")) + (syncMgr->isTimeSynced() ? "YES" : "NO"));

  if (syncMgr->isTimeSynced())
  {
    Serial.println(String(F("Synced Time: ")) + String(syncMgr->getSyncedTime()));
    Serial.println(String(F("Time Offset: ")) + String(syncMgr->getTimeOffset()) + "ms");
  }

  if (groupInfo.groupId != 0)
  {
    Serial.println(String(F("Master Device: 0x")) + String(groupInfo.masterDeviceId, HEX));
  }

  Serial.println(String(F("Auto-join: ")) + (syncMgr->isAutoJoinEnabled() ? "ENABLED" : "DISABLED"));

  // Show auto-create status (only relevant when auto-join is enabled)
  if (syncMgr->isAutoJoinEnabled())
  {
    Serial.println(String(F("Auto-create: ")) + (syncMgr->isAutoCreateEnabled() ? "ENABLED" : "DISABLED"));
  }

  // Show discovered devices
  const auto &discoveredDevices = syncMgr->getDiscoveredDevices();
  Serial.println(String(F("Discovered Devices: ")) + String(discoveredDevices.size()));

  // Show discovered groups
  auto discoveredGroups = syncMgr->getDiscoveredGroups();
  Serial.println(String(F("Discovered Groups: ")) + String(discoveredGroups.size()));

  Serial.println(F("===========================\n"));

  // Print detailed information
  syncMgr->printDeviceInfo();
  syncMgr->printGroupInfo();
}

void showAvailableGroups()
{
  Serial.println(F("\n=== AVAILABLE GROUPS ==="));

  SyncManager *syncMgr = SyncManager::getInstance();
  std::vector<GroupAdvert> groups = syncMgr->getDiscoveredGroups();

  // Clear and rebuild available groups list
  availableGroups.clear();

  if (groups.empty())
  {
    Serial.println(F("No groups found."));
    Serial.println(F("Tip: Wait a few seconds for device discovery or create a new group."));
  }
  else
  {
    Serial.println(F("Found groups:"));
    int index = 1;

    for (const auto &group : groups)
    {
      availableGroups.push_back(group.groupId);

      Serial.println(String(F("  ")) + String(index) +
                     String(F(") Group 0x")) + String(group.groupId, HEX) +
                     String(F(" - Master: 0x")) + String(group.masterDeviceId, HEX) +
                     String(F(" (last seen ")) + String(millis() - group.lastSeen) + String(F("ms ago)")));
      index++;
    }

    Serial.println(F("\nUse option 4 to join a group by number."));
  }

  Serial.println(F("========================\n"));
}

void createNewGroup()
{
  Serial.println(F("\n=== CREATE NEW GROUP ==="));

  SyncManager *syncMgr = SyncManager::getInstance();

  if (syncMgr->getGroupId() != 0)
  {
    Serial.println(F("Warning: You are already in a group. Leave current group first? (y/n)"));
    Serial.print(F("> "));

    while (!Serial.available())
    {
      delay(10);
    }
    String confirm = Serial.readStringUntil('\n');
    confirm.trim();
    confirm.toLowerCase();

    if (confirm != "y" && confirm != "yes")
    {
      Serial.println(F("Group creation cancelled."));
      return;
    }

    syncMgr->leaveGroup();
    Serial.println(F("Left previous group."));
  }

  Serial.println(F("Enter group ID (hex, e.g. 12345678) or press Enter for random:"));
  Serial.print(F("> "));

  while (!Serial.available())
  {
    delay(10);
  }
  String groupInput = Serial.readStringUntil('\n');
  groupInput.trim();

  uint32_t groupId = 0;
  if (groupInput.length() > 0)
  {
    // Parse hex input
    groupId = strtoul(groupInput.c_str(), NULL, 16);
    if (groupId == 0)
    {
      Serial.println(F("Invalid hex input. Using random group ID."));
    }
  }

  syncMgr->createGroup(groupId);

  if (groupId == 0)
  {
    groupId = syncMgr->getGroupId();
  }

  Serial.println(String(F("Created group 0x")) + String(groupId, HEX));
  Serial.println(F("You are now the master of this group."));
  Serial.println(F("========================\n"));
}

void joinGroupByNumber(int groupNumber)
{
  if (groupNumber < 0 || groupNumber >= (int)availableGroups.size())
  {
    Serial.println(F("Invalid group number."));
    return;
  }

  uint32_t groupId = availableGroups[groupNumber];

  Serial.println(String(F("\n=== JOINING GROUP 0x")) + String(groupId, HEX) + F(" ==="));

  SyncManager *syncMgr = SyncManager::getInstance();

  if (syncMgr->getGroupId() != 0)
  {
    Serial.println(F("Leaving current group first..."));
    syncMgr->leaveGroup();
  }

  Serial.println(F("Joining group..."));
  syncMgr->joinGroup(groupId);

  Serial.println(String(F("Joined group 0x")) + String(groupId, HEX));
  Serial.println(F("Waiting for master election and time sync..."));
  Serial.println(F("==============================\n"));
}

void listKnownDevices()
{
  Serial.println(F("\n=== KNOWN DEVICES ==="));

  SyncManager *syncMgr = SyncManager::getInstance();
  const auto &devices = syncMgr->getDiscoveredDevices();

  if (devices.empty())
  {
    Serial.println(F("No devices discovered yet."));
    Serial.println(F("Wait a few seconds for device discovery."));
  }
  else
  {
    Serial.println(String(F("Found ")) + String(devices.size()) + F(" device(s):"));
    Serial.println();

    int index = 1;
    for (const auto &devicePair : devices)
    {
      const DiscoveredDevice &device = devicePair.second;

      Serial.println(String(F("Device ")) + String(index) + F(":"));
      Serial.println(String(F("  ID: 0x")) + String(device.deviceId, HEX));
      Serial.println(String(F("  Last Seen: ")) + String(millis() - device.lastSeen) + F("ms ago"));

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
      Serial.println(String(F("  MAC: ")) + macStr);
      Serial.println();
      index++;
    }
  }

  Serial.println(F("====================\n"));
}