// application.h

#pragma once

#include "config.h"
#include "IO/LED/LEDStrip.h"
#include "IO/LED/Effects/BrakeLightEffect.h"
#include "IO/LED/Effects/IndicatorEffect.h"
#include "IO/LED/Effects/ReverseLightEffect.h"
#include "IO/LED/Effects/RGBEffect.h"
#include "IO/LED/Effects/NightRiderEffect.h"
#include "IO/LED/Effects/TaillightEffect.h"
#include "IO/LED/Effects/HeadlightEffect.h"
// #include "IO/LED/Effects/HeadlightStartupEffect.h"
#include "IO/LED/Effects/PoliceEffect.h"
#include "IO/LED/Effects/PulseWaveEffect.h"
#include "IO/LED/Effects/AuroraEffect.h"
#include "IO/LED/Effects/SolidColorEffect.h"

#include "Sequences/SequenceBase.h"
#include "Sequences/BothIndicatorsSequence.h"
#include "Sequences/IndicatorFlickSequence.h"
#include "Sequences/BrakeTapSequence.h"

#include "IO/GPIO.h"
#include "IO/Inputs.h"
#include "IO/Wireless.h"

#include "SerialMenu.h"
#include "IO/Inputs.h"
#include "Sync/SyncManager.h"

// static void updateInput(HVInput input)
// {
//   input.update();
// }

// static bool getInput(HVInput input)
// {
//   return input.get();
// }

// static bool isEnabled(HVInput input)
// {
//   return input.isEnabled();
// }

#define NUM_MODES 4

enum class ApplicationMode
{
  NORMAL,
  TEST,
  REMOTE,
  OFF
};

// Add menu system enums and states
enum class MenuState
{
  NORMAL_MODE, // Regular mode display
  GROUP_INFO,  // Show group information
  GROUP_MENU   // Group management menu
};

enum class GroupMenuOption
{
  CREATE_GROUP,
  JOIN_GROUP,
  LEAVE_GROUP,
  AUTO_JOIN_TOGGLE,
  BACK_TO_NORMAL
};

struct MenuContext
{
  MenuState currentState = MenuState::NORMAL_MODE;
  GroupMenuOption selectedOption = GroupMenuOption::CREATE_GROUP;
  uint32_t lastBootPress = 0;
  uint32_t groupInfoDisplayStart = 0;
  uint32_t lastSyncUpdate = 0;
  bool inGroupMenu = false;
  uint32_t groupIdToJoin = 0;
  uint8_t groupIdDigitIndex = 0;
  bool editingGroupId = false;
};

struct AppStats
{
  uint32_t loopsPerSecond;
  uint32_t updateInputTime;

  uint32_t updateModeTime;
  uint32_t updateSyncTime;

  uint32_t updateEffectsTime;
  uint32_t drawTime;
};

class Application
{
public:
  static Application *getInstance();

  Application();
  ~Application();

  // Call once to initialize the system.
  void begin();

  void setupEffects();
  void setupSequences();

  // Main loop function to be called from loop()
  void loop();

  void btnLoop();

  // Set testMode externally.
  void enableNormalMode();
  void enableTestMode();
  void enableRemoteMode();
  void enableOffMode();

  // HVInput instances
  HVInput accOnInput;          // 12v ACC
  HVInput leftIndicatorInput;  // Left indicator
  HVInput rightIndicatorInput; // Right indicator
  HVInput headlightInput;      // Headlight
  HVInput brakeInput;          // Brake
  HVInput reverseInput;        // Reverse

private:
  void updateInputs();
  void setupWireless();

  // Effect instances.
  IndicatorEffect *leftIndicatorEffect;
  IndicatorEffect *rightIndicatorEffect;
  RGBEffect *rgbEffect;
  NightRiderEffect *nightriderEffect;

  HeadlightEffect *headlightEffect;
  TaillightEffect *taillightEffect;

  BrakeLightEffect *brakeEffect;
  ReverseLightEffect *reverseLightEffect;
  PoliceEffect *policeEffect;

  // New effects
  PulseWaveEffect *pulseWaveEffect;
  AuroraEffect *auroraEffect;
  SolidColorEffect *solidColorEffect;

  // Sequences
  BothIndicatorsSequence *unlockSequence;
  BothIndicatorsSequence *lockSequence;
  IndicatorFlickSequence *RGBFlickSequence;
  IndicatorFlickSequence *nightRiderFlickSequence;

  BrakeTapSequence *brakeTapSequence3;

  // Application Mode
  ApplicationMode mode;
  ApplicationMode prevMode;

  bool appInitialized;

  // Menu system
  MenuContext menuContext;

  // Internal method to handle effect selection based on inputs.
  void handleNormalEffects();
  void handleTestEffects();
  void handleRemoteEffects();
  void handleSyncedEffects(const EffectSyncState &effectState);

  // Menu system methods
  void handleMenuNavigation();
  void displayNormalMode();
  void displayGroupInfo();
  void displayGroupMenu();
  void executeGroupMenuAction();
  void updateSyncedLEDTiming();

  uint64_t lastRemotePing;
  AppStats stats;
};
