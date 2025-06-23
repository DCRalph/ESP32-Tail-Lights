#include "Application.h"
#include "IO/Wireless.h"
#include "IO/LED/Effects.h"
#include "IO/LED/LEDStripManager.h"
#include "Sync/SyncManager.h"
#include "IO/TimeProfiler.h"

// Command constants
constexpr uint8_t CMD_PING = 0xe0;
constexpr uint8_t CMD_SET_MODE = 0xe1;
constexpr uint8_t CMD_SET_EFFECTS = 0xe2;
constexpr uint8_t CMD_GET_EFFECTS = 0xe3;
constexpr uint8_t CAR_CMD_SET_INPUTS = 0xe4;
constexpr uint8_t CAR_CMD_GET_INPUTS = 0xe5;
constexpr uint8_t CAR_CMD_TRIGGER_SEQUENCE = 0xe6;
constexpr uint8_t CAR_CMD_GET_STATS = 0xe7;

// Sync management commands
constexpr uint8_t CMD_SYNC_GET_DEVICES = 0xe8;
constexpr uint8_t CMD_SYNC_GET_GROUPS = 0xe9;
constexpr uint8_t CMD_SYNC_GET_GROUP_INFO = 0xea;
constexpr uint8_t CMD_SYNC_JOIN_GROUP = 0xeb;
constexpr uint8_t CMD_SYNC_LEAVE_GROUP = 0xec;
constexpr uint8_t CMD_SYNC_CREATE_GROUP = 0xed;
constexpr uint8_t CMD_SYNC_GET_STATUS = 0xee;
constexpr uint8_t CMD_SYNC_SET_MODE = 0xef;
constexpr uint8_t CMD_SYNC_GET_MODE = 0xf0;

// Struct definitions for wireless communication
struct PingCmd
{
  ApplicationMode mode;
  bool headlight;
  bool taillight;
  bool underglow;
  bool interior;
};

struct SetModeCmd
{
  ApplicationMode mode;
};

struct EffectsCmd
{
  bool leftIndicator;
  bool rightIndicator;

  int headlightMode;
  bool headlightSplit;
  bool headlightR;
  bool headlightG;
  bool headlightB;

  int taillightMode;
  bool taillightSplit;

  bool brake;
  bool reverse;

  bool rgb;
  bool nightrider;
  bool police;
  PoliceMode policeMode;
  bool testEffect1;
  bool testEffect2;
  bool solidColor;
  SolidColorPreset solidColorPreset;
  uint8_t solidColorR;
  uint8_t solidColorG;
  uint8_t solidColorB;
  bool colorFade;
};

struct InputsCmd
{
  bool accOn;
  bool indicatorLeft;
  bool indicatorRight;
  bool headlight;
  bool brake;
  bool reverse;
};

struct TriggerSequenceCmd
{
  uint8_t sequence;
};

// Sync management structs
struct SyncDeviceInfo
{
  uint32_t deviceId;
  uint8_t mac[6];
  uint32_t lastSeen;
  uint32_t timeSinceLastSeen;
  bool inCurrentGroup;
  bool isGroupMaster;
  bool isThisDevice;
};

struct SyncDevicesResponse
{
  uint8_t deviceCount;
  uint32_t currentTime;
  SyncDeviceInfo devices[8];
};

struct SyncGroupInfo
{
  uint32_t groupId;
  uint32_t masterDeviceId;
  uint8_t masterMac[6];
  uint32_t lastSeen;
  uint32_t timeSinceLastSeen;
  bool isCurrentGroup;
  bool canJoin;
};

struct SyncGroupsResponse
{
  uint8_t groupCount;
  uint32_t currentTime;
  uint32_t ourGroupId;
  SyncGroupInfo groups[4];
};

struct SyncGroupMemberInfo
{
  uint32_t deviceId;
  uint8_t mac[6];
  bool isGroupMaster;
  bool isThisDevice;
  uint32_t lastHeartbeat;
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
  uint32_t currentTime;
  SyncGroupMemberInfo members[6];
};

struct SyncJoinGroupCmd
{
  uint32_t groupId;
};

struct SyncCreateGroupCmd
{
  uint32_t groupId;
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
  int syncMode; // 0=SOLO, 1=JOIN, 2=HOST
};

struct SyncModeCmd
{
  uint8_t mode; // 0=SOLO, 1=JOIN, 2=HOST
};

// Setup wireless communication handlers
void Application::setupWireless()
{
  // Ping command (0xe0) - send current mode and enabled strips
  wireless.addOnReceiveFor(CMD_PING, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             LEDStripManager *ledManager = LEDStripManager::getInstance();

                             data_packet pTX = {0};
                             pTX.type = CMD_PING;

                             PingCmd pCmd;
                             pCmd.mode = mode;
                             pCmd.headlight = ledManager->isStripEnabled(LEDStripType::HEADLIGHT);
                             pCmd.taillight = ledManager->isStripEnabled(LEDStripType::TAILLIGHT);
                             pCmd.underglow = ledManager->isStripEnabled(LEDStripType::UNDERGLOW);
                             pCmd.interior = ledManager->isStripEnabled(LEDStripType::INTERIOR);

                             pTX.len = sizeof(pCmd);
                             memcpy(pTX.data, &pCmd, sizeof(pCmd));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Set mode command (0xe1)
  wireless.addOnReceiveFor(CMD_SET_MODE, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             uint8_t *data = fp->p.data;
                             uint8_t rxMode = data[0];

                             switch (rxMode)
                             {
                             case 0:
                               enableNormalMode();
                               break;
                             case 1:
                               enableTestMode();
                               break;
                             case 2:
                               enableRemoteMode();
                               break;
                             case 3:
                               enableOffMode();
                               break;
                             default:
                               break;
                             }

                             data_packet pTX = {0};
                             pTX.type = CMD_SET_MODE;
                             pTX.len = 1;

                             // Send current mode as uint8_t
                             switch (mode)
                             {
                             case ApplicationMode::NORMAL:
                               pTX.data[0] = 0;
                               break;
                             case ApplicationMode::TEST:
                               pTX.data[0] = 1;
                               break;
                             case ApplicationMode::REMOTE:
                               pTX.data[0] = 2;
                               break;
                             case ApplicationMode::OFF:
                               pTX.data[0] = 3;
                               break;
                             default:
                               pTX.data[0] = 0;
                               break;
                             }

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Get effects command (0xe3) - Returns current effects for each strip
  wireless.addOnReceiveFor(CMD_GET_EFFECTS, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             LEDStripManager *ledManager = LEDStripManager::getInstance();

                             data_packet pTX = {0};
                             pTX.type = CMD_GET_EFFECTS;

                             EffectsCmd eCmd = {0};

                             eCmd.leftIndicator = leftIndicatorEffect->isActive();
                             eCmd.rightIndicator = rightIndicatorEffect->isActive();

                             eCmd.headlightMode = headlightEffect ? static_cast<int>(headlightEffect->getMode()) : 0;
                             eCmd.headlightSplit = headlightEffect->getSplit();
                             headlightEffect->getColor(eCmd.headlightR, eCmd.headlightG, eCmd.headlightB);

                             // Use new TaillightEffect
                             eCmd.taillightMode = taillightEffect ? static_cast<int>(taillightEffect->getMode()) : 0;
                             eCmd.taillightSplit = taillightEffect->getSplit();
                             // Keep separate brake and reverse effects
                             eCmd.brake = brakeEffect->isActive();
                             eCmd.reverse = reverseLightEffect->isActive();

                             eCmd.rgb = rgbEffect->isActive();
                             eCmd.nightrider = nightriderEffect->isActive();
                             eCmd.police = policeEffect->isActive();
                             eCmd.policeMode = policeEffect->getMode();
                             eCmd.testEffect1 = pulseWaveEffect->isActive();
                             eCmd.testEffect2 = auroraEffect->isActive();
                             eCmd.solidColor = solidColorEffect->isActive();
                             eCmd.solidColorPreset = solidColorEffect->getColorPreset();
                             solidColorEffect->getCustomColor(eCmd.solidColorR, eCmd.solidColorG, eCmd.solidColorB);
                             eCmd.colorFade = colorFadeEffect->isActive();

                             pTX.len = sizeof(eCmd);
                             memcpy(pTX.data, &eCmd, sizeof(eCmd));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Set effects command (0xe2) - Set effects for each strip
  wireless.addOnReceiveFor(CMD_SET_EFFECTS, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             LEDStripManager *ledManager = LEDStripManager::getInstance();

                             EffectsCmd eCmd = {0};
                             memcpy(&eCmd, fp->p.data, sizeof(eCmd));

                             leftIndicatorEffect->setActive(eCmd.leftIndicator);
                             rightIndicatorEffect->setActive(eCmd.rightIndicator);

                             headlightEffect->setMode(eCmd.headlightMode);
                             headlightEffect->setSplit(eCmd.headlightSplit);
                             headlightEffect->setColor(eCmd.headlightR, eCmd.headlightG, eCmd.headlightB);

                             taillightEffect->setMode(eCmd.taillightMode);
                             taillightEffect->setSplit(eCmd.taillightSplit);

                             // Keep separate brake and reverse effects
                             brakeEffect->setActive(eCmd.brake);
                             brakeEffect->setIsReversing(eCmd.reverse);
                             reverseLightEffect->setActive(eCmd.reverse);

                             rgbEffect->setActive(eCmd.rgb);
                             nightriderEffect->setActive(eCmd.nightrider);
                             policeEffect->setActive(eCmd.police);
                             policeEffect->setMode(eCmd.policeMode);
                             pulseWaveEffect->setActive(eCmd.testEffect1);
                             auroraEffect->setActive(eCmd.testEffect2);
                             solidColorEffect->setActive(eCmd.solidColor);
                             solidColorEffect->setColorPreset(eCmd.solidColorPreset);
                             solidColorEffect->setCustomColor(eCmd.solidColorR, eCmd.solidColorG, eCmd.solidColorB);
                             colorFadeEffect->setActive(eCmd.colorFade);

                             SyncManager *syncMgr = SyncManager::getInstance();

                             if (syncMgr->isGroupMaster() && syncMgr->isEffectSyncEnabled())
                             {
                               EffectSyncState effectState = {};

                               effectState.rgbSyncData = rgbEffect->getSyncData();
                               effectState.nightRiderSyncData = nightriderEffect->getSyncData();
                               effectState.policeSyncData = policeEffect->getSyncData();
                               effectState.solidColorSyncData = solidColorEffect->getSyncData();
                               effectState.colorFadeSyncData = colorFadeEffect->getSyncData();

                               syncMgr->setEffectSyncState(effectState);
                               syncMgr->sendEffectState();
                             }

                             //
                           });

  wireless.addOnReceiveFor(CAR_CMD_SET_INPUTS, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             InputsCmd iCmd = {0};
                             memcpy(&iCmd, fp->p.data, sizeof(iCmd));

                             if (mode != ApplicationMode::TEST)
                               return;

                             accOnInput.override(iCmd.accOn);
                             leftIndicatorInput.override(iCmd.indicatorLeft);
                             rightIndicatorInput.override(iCmd.indicatorRight);
                             headlightInput.override(iCmd.headlight);
                             brakeInput.override(iCmd.brake);
                             reverseInput.override(iCmd.reverse);

                             //
                           });

  wireless.addOnReceiveFor(CAR_CMD_GET_INPUTS, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             InputsCmd iCmd = {0};
                             iCmd.accOn = accOnInput.get();
                             iCmd.indicatorLeft = leftIndicatorInput.get();
                             iCmd.indicatorRight = rightIndicatorInput.get();
                             iCmd.headlight = headlightInput.get();
                             iCmd.brake = brakeInput.get();
                             iCmd.reverse = reverseInput.get();

                             data_packet pTX = {0};
                             pTX.type = CAR_CMD_GET_INPUTS;
                             pTX.len = sizeof(iCmd);
                             memcpy(pTX.data, &iCmd, sizeof(iCmd));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  wireless.addOnReceiveFor(CAR_CMD_TRIGGER_SEQUENCE, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             TriggerSequenceCmd tCmd = {0};
                             memcpy(&tCmd, fp->p.data, sizeof(tCmd));

                             switch (tCmd.sequence)
                             {
                             case 0:
                               if (unlockSequence)
                                 unlockSequence->trigger();
                               break;
                             case 1:
                               if (lockSequence)
                                 lockSequence->trigger();
                               break;
                             case 2:
                               if (RGBFlickSequence)
                                 RGBFlickSequence->trigger();
                               break;
                             case 3:
                               if (nightRiderFlickSequence)
                                 nightRiderFlickSequence->trigger();
                               break;
                             }
                             //
                           });

  wireless.addOnReceiveFor(CAR_CMD_GET_STATS, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             data_packet pTX = {0};
                             pTX.type = CAR_CMD_GET_STATS;

                             AppStats stats = {0};
                             stats.updateInputTime = timeProfiler.getTimeUs("updateInputs");
                             stats.updateModeTime = timeProfiler.getTimeUs("updateMode");
                             stats.updateSyncTime = timeProfiler.getTimeUs("updateSync");
                             stats.updateEffectsTime = timeProfiler.getTimeUs("updateEffects");
                             stats.drawTime = timeProfiler.getTimeUs("drawEffects");

                             pTX.len = sizeof(AppStats);
                             memcpy(pTX.data, &stats, sizeof(AppStats));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Sync management commands

  // Get discovered devices (0xe8)
  wireless.addOnReceiveFor(CMD_SYNC_GET_DEVICES, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             SyncManager *syncMgr = SyncManager::getInstance();
                             const auto &discoveredDevices = syncMgr->getDiscoveredDevices();
                             const auto &groupInfo = syncMgr->getGroupInfo();
                             uint32_t ourDeviceId = syncMgr->getDeviceId();
                             uint32_t now = millis();

                             data_packet pTX = {0};
                             pTX.type = CMD_SYNC_GET_DEVICES;

                             SyncDevicesResponse response = {0};
                             response.deviceCount = std::min((size_t)8, discoveredDevices.size());
                             response.currentTime = now;

                             int i = 0;
                             for (const auto &devicePair : discoveredDevices)
                             {
                               if (i >= 8)
                                 break;
                               const auto &device = devicePair.second;

                               response.devices[i].deviceId = device.deviceId;
                               memcpy(response.devices[i].mac, device.mac, 6);
                               response.devices[i].lastSeen = device.lastSeen;
                               response.devices[i].timeSinceLastSeen = now - device.lastSeen;
                               response.devices[i].isThisDevice = (device.deviceId == ourDeviceId);

                               // Check if device is in our current group
                               response.devices[i].inCurrentGroup = false;
                               response.devices[i].isGroupMaster = false;

                               if (groupInfo.groupId != 0)
                               {
                                 auto memberIt = groupInfo.members.find(devicePair.first);
                                 if (memberIt != groupInfo.members.end())
                                 {
                                   response.devices[i].inCurrentGroup = true;
                                   response.devices[i].isGroupMaster = (device.deviceId == groupInfo.masterDeviceId);
                                 }
                               }

                               i++;
                             }

                             pTX.len = sizeof(response);
                             memcpy(pTX.data, &response, sizeof(response));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Get discovered groups (0xe9)
  wireless.addOnReceiveFor(CMD_SYNC_GET_GROUPS, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             SyncManager *syncMgr = SyncManager::getInstance();
                             const auto discoveredGroups = syncMgr->getDiscoveredGroups();
                             const auto &groupInfo = syncMgr->getGroupInfo();
                             uint32_t ourGroupId = groupInfo.groupId;
                             uint32_t now = millis();

                             data_packet pTX = {0};
                             pTX.type = CMD_SYNC_GET_GROUPS;

                             SyncGroupsResponse response = {0};
                             response.groupCount = std::min((size_t)4, discoveredGroups.size());
                             response.currentTime = now;
                             response.ourGroupId = ourGroupId;

                             for (int i = 0; i < response.groupCount; i++)
                             {
                               const auto &group = discoveredGroups[i];
                               response.groups[i].groupId = group.groupId;
                               response.groups[i].masterDeviceId = group.masterDeviceId;
                               memcpy(response.groups[i].masterMac, group.masterMac, 6);
                               response.groups[i].lastSeen = group.lastSeen;
                               response.groups[i].timeSinceLastSeen = now - group.lastSeen;
                               response.groups[i].isCurrentGroup = (group.groupId == ourGroupId);
                               response.groups[i].canJoin = (ourGroupId == 0 || group.groupId != ourGroupId);
                             }

                             pTX.len = sizeof(response);
                             memcpy(pTX.data, &response, sizeof(response));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Get current group info (0xea)
  wireless.addOnReceiveFor(CMD_SYNC_GET_GROUP_INFO, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             SyncManager *syncMgr = SyncManager::getInstance();
                             const auto &groupInfo = syncMgr->getGroupInfo();
                             const auto &discoveredDevices = syncMgr->getDiscoveredDevices();
                             uint32_t ourDeviceId = syncMgr->getDeviceId();
                             uint32_t now = millis();

                             data_packet pTX = {0};
                             pTX.type = CMD_SYNC_GET_GROUP_INFO;

                             SyncCurrentGroupInfo response = {0};
                             response.groupId = groupInfo.groupId;
                             response.masterDeviceId = groupInfo.masterDeviceId;
                             response.isMaster = groupInfo.isMaster;
                             response.timeSynced = syncMgr->isTimeSynced();
                             response.timeOffset = syncMgr->getTimeOffset();
                             response.syncedTime = syncMgr->getSyncedTime();
                             response.memberCount = std::min((size_t)6, groupInfo.members.size());
                             response.currentTime = now;

                             // Fill member details
                             int i = 0;
                             for (const auto &memberPair : groupInfo.members)
                             {
                               if (i >= 6)
                                 break;

                               const auto &member = memberPair.second;
                               response.members[i].deviceId = member.deviceId;
                               memcpy(response.members[i].mac, member.mac, 6);
                               response.members[i].isGroupMaster = (member.deviceId == groupInfo.masterDeviceId);
                               response.members[i].isThisDevice = (member.deviceId == ourDeviceId);

                               // Try to find heartbeat info from discovered devices
                               auto discoveredIt = discoveredDevices.find(memberPair.first);
                               if (discoveredIt != discoveredDevices.end())
                               {
                                 response.members[i].lastHeartbeat = now - discoveredIt->second.lastSeen;
                               }
                               else
                               {
                                 response.members[i].lastHeartbeat = 0; // Unknown
                               }

                               i++;
                             }

                             pTX.len = sizeof(response);
                             memcpy(pTX.data, &response, sizeof(response));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Join group (0xeb)
  wireless.addOnReceiveFor(CMD_SYNC_JOIN_GROUP, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             SyncJoinGroupCmd cmd = {0};
                             memcpy(&cmd, fp->p.data, sizeof(cmd));

                             SyncManager *syncMgr = SyncManager::getInstance();
                             syncMgr->joinGroup(cmd.groupId);

                             // Send back current group info as confirmation
                             const auto &groupInfo = syncMgr->getGroupInfo();

                             data_packet pTX = {0};
                             pTX.type = CMD_SYNC_JOIN_GROUP;
                             pTX.len = sizeof(uint32_t);
                             memcpy(pTX.data, &groupInfo.groupId, sizeof(uint32_t));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Leave group (0xec)
  wireless.addOnReceiveFor(CMD_SYNC_LEAVE_GROUP, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             SyncManager *syncMgr = SyncManager::getInstance();
                             syncMgr->leaveGroup();

                             // Send back confirmation (group ID should be 0 now)
                             data_packet pTX = {0};
                             pTX.type = CMD_SYNC_LEAVE_GROUP;
                             pTX.len = sizeof(uint32_t);
                             uint32_t groupId = 0;
                             memcpy(pTX.data, &groupId, sizeof(uint32_t));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Create group (0xed)
  wireless.addOnReceiveFor(CMD_SYNC_CREATE_GROUP, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             SyncCreateGroupCmd cmd = {0};
                             memcpy(&cmd, fp->p.data, sizeof(cmd));

                             SyncManager *syncMgr = SyncManager::getInstance();
                             syncMgr->createGroup(cmd.groupId); // 0 = auto-generate

                             // Send back created group info as confirmation
                             const auto &groupInfo = syncMgr->getGroupInfo();

                             data_packet pTX = {0};
                             pTX.type = CMD_SYNC_CREATE_GROUP;

                             SyncCurrentGroupInfo response = {0};
                             response.groupId = groupInfo.groupId;
                             response.masterDeviceId = groupInfo.masterDeviceId;
                             response.isMaster = groupInfo.isMaster;
                             response.timeSynced = syncMgr->isTimeSynced();
                             response.timeOffset = syncMgr->getTimeOffset();
                             response.memberCount = groupInfo.members.size();

                             pTX.len = sizeof(response);
                             memcpy(pTX.data, &response, sizeof(response));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Get comprehensive status (0xee)
  wireless.addOnReceiveFor(CMD_SYNC_GET_STATUS, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             SyncManager *syncMgr = SyncManager::getInstance();
                             const auto &groupInfo = syncMgr->getGroupInfo();
                             const auto &discoveredDevices = syncMgr->getDiscoveredDevices();
                             const auto discoveredGroups = syncMgr->getDiscoveredGroups();

                             data_packet pTX = {0};
                             pTX.type = CMD_SYNC_GET_STATUS;

                             SyncDetailedStatus response = {0};
                             response.deviceId = syncMgr->getDeviceId();
                             response.groupId = groupInfo.groupId;
                             response.masterDeviceId = groupInfo.masterDeviceId;
                             response.isMaster = groupInfo.isMaster;
                             response.timeSynced = syncMgr->isTimeSynced();
                             response.timeOffset = syncMgr->getTimeOffset();
                             response.syncedTime = syncMgr->getSyncedTime();
                             response.memberCount = groupInfo.members.size();
                             response.discoveredDeviceCount = std::min((size_t)255, discoveredDevices.size());
                             response.discoveredGroupCount = std::min((size_t)255, discoveredGroups.size());
                             response.syncMode = static_cast<int>(syncMgr->getSyncMode());

                             pTX.len = sizeof(response);
                             memcpy(pTX.data, &response, sizeof(response));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Set sync mode (0xef)
  wireless.addOnReceiveFor(CMD_SYNC_SET_MODE, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             SyncModeCmd cmd = {0};
                             memcpy(&cmd, fp->p.data, sizeof(cmd));

                             SyncManager *syncMgr = SyncManager::getInstance();
                             syncMgr->setSyncMode(static_cast<SyncMode>(cmd.mode));

                             // Send back confirmation
                             data_packet pTX = {0};
                             pTX.type = CMD_SYNC_SET_MODE;
                             pTX.len = sizeof(uint8_t);
                             uint8_t currentMode = static_cast<int>(syncMgr->getSyncMode());
                             memcpy(pTX.data, &currentMode, sizeof(currentMode));

                             wireless.send(&pTX, fp->mac);
                             //
                           });

  // Get sync mode (0xf0)
  wireless.addOnReceiveFor(CMD_SYNC_GET_MODE, [this](fullPacket *fp)
                           {
                             lastRemotePing = millis();

                             SyncManager *syncMgr = SyncManager::getInstance();

                             data_packet pTX = {0};
                             pTX.type = CMD_SYNC_GET_MODE;
                             pTX.len = sizeof(uint8_t);
                             uint8_t mode = static_cast<int>(syncMgr->getSyncMode());
                             memcpy(pTX.data, &mode, sizeof(mode));

                             wireless.send(&pTX, fp->mac);
                             //
                           });
}