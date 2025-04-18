#include "Application.h"
#include "IO/Wireless.h"
#include "IO/LED/Effects.h"
#include "IO/LED/LEDStripManager.h"

// Command constants
constexpr uint8_t CMD_PING = 0xe0;
constexpr uint8_t CMD_SET_MODE = 0xe1;
constexpr uint8_t CMD_SET_EFFECTS = 0xe2;
constexpr uint8_t CMD_GET_EFFECTS = 0xe3;

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
  bool testEffect1;
  bool testEffect2;

  bool leftIndicator;
  bool rightIndicator;

  bool headlight;
  bool headlightSplit;
  bool headlightR;
  bool headlightG;
  bool headlightB;

  bool brake;
  bool reverse;

  bool rgb;
  bool nightrider;
  bool startup;
  bool police;
  PoliceMode policeMode;
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

                             // Determine the number of enabled strips

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

                             eCmd.testEffect1 = pulseWaveEffect->isActive();
                             eCmd.testEffect2 = auroraEffect->isActive();

                             eCmd.leftIndicator = leftIndicatorEffect->isActive();
                             eCmd.rightIndicator = rightIndicatorEffect->isActive();
                             eCmd.rgb = rgbEffect->isActive();
                             eCmd.nightrider = nightriderEffect->isActive();
                             eCmd.startup = taillightStartupEffect->isActive() || headlightStartupEffect->isActive();
                             eCmd.police = policeEffect->isActive();
                             eCmd.policeMode = policeEffect->getMode();

                             eCmd.headlight = headlightEffect->isActive();
                             eCmd.headlightSplit = headlightEffect->getSplit();
                             headlightEffect->getColor(eCmd.headlightR, eCmd.headlightG, eCmd.headlightB);

                             eCmd.brake = brakeEffect->isActive();
                             eCmd.reverse = reverseLightEffect->isActive();

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

                             pulseWaveEffect->setActive(eCmd.testEffect1);
                             auroraEffect->setActive(eCmd.testEffect2);

                             leftIndicatorEffect->setActive(eCmd.leftIndicator);
                             rightIndicatorEffect->setActive(eCmd.rightIndicator);

                             rgbEffect->setActive(eCmd.rgb);
                             nightriderEffect->setActive(eCmd.nightrider);
                             taillightStartupEffect->setActive(eCmd.startup);
                             headlightStartupEffect->setActive(eCmd.startup);

                             policeEffect->setActive(eCmd.police);
                             policeEffect->setMode(eCmd.policeMode);

                             headlightEffect->setActive(eCmd.headlight);
                             headlightEffect->setSplit(eCmd.headlightSplit);
                             headlightEffect->setColor(eCmd.headlightR, eCmd.headlightG, eCmd.headlightB);

                             brakeEffect->setActive(eCmd.brake);
                             brakeEffect->setIsReversing(eCmd.reverse);
                             reverseLightEffect->setActive(eCmd.reverse);

                             //
                           });
}