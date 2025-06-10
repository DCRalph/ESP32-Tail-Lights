#include "Application.h"
#include "IO/Wireless.h"
#include "IO/LED/Effects.h"
#include "IO/LED/LEDStripManager.h"

// Command constants
constexpr uint8_t CMD_PING = 0xe0;
constexpr uint8_t CMD_SET_MODE = 0xe1;
constexpr uint8_t CMD_SET_EFFECTS = 0xe2;
constexpr uint8_t CMD_GET_EFFECTS = 0xe3;
constexpr uint8_t CAR_CMD_SET_INPUTS = 0xe4;
constexpr uint8_t CAR_CMD_GET_INPUTS = 0xe5;
constexpr uint8_t CAR_CMD_TRIGGER_SEQUENCE = 0xe6;
constexpr uint8_t CAR_CMD_GET_STATS = 0xe7;

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

                             pTX.len = sizeof(AppStats);
                             memcpy(pTX.data, &stats, sizeof(AppStats));

                             wireless.send(&pTX, fp->mac);
                             //
                           });
}