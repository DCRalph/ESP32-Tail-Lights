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
#include "IO/LED/Effects/PoliceEffect.h"
#include "IO/LED/Effects/PulseWaveEffect.h"
#include "IO/LED/Effects/AuroraEffect.h"
#include "IO/LED/Effects/SolidColorEffect.h"
#include "IO/LED/Effects/ColorFadeEffect.h"
#include "IO/LED/Effects/CommitEffect.h"
#include "IO/LED/Effects/ServiceLightsEffect.h"

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
#include "IO/BLE.h"

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
  friend class BLEManager;

public:
  static Application *getInstance();

  Application();

  // Call once to initialize the system.
  void begin();

  void setupEffects();
  void setupSequences();

  ApplicationMode getMode();

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

  // Effect instances.
  // Core effects
  IndicatorEffect *leftIndicatorEffect = nullptr;
  IndicatorEffect *rightIndicatorEffect = nullptr;

  HeadlightEffect *headlightEffect = nullptr;
  TaillightEffect *taillightEffect = nullptr;

  BrakeLightEffect *brakeEffect = nullptr;
  ReverseLightEffect *reverseLightEffect = nullptr;

  // Other effects
  RGBEffect *rgbEffect = nullptr;
  NightRiderEffect *nightriderEffect = nullptr;
  PoliceEffect *policeEffect = nullptr;
  PulseWaveEffect *pulseWaveEffect = nullptr;
  AuroraEffect *auroraEffect = nullptr;
  SolidColorEffect *solidColorEffect = nullptr;
  ColorFadeEffect *colorFadeEffect = nullptr;
  CommitEffect *commitEffect = nullptr;
  ServiceLightsEffect *serviceLightsEffect = nullptr;

  // Sequences
  BothIndicatorsSequence *unlockSequence = nullptr;
  BothIndicatorsSequence *lockSequence = nullptr;
  IndicatorFlickSequence *RGBFlickSequence = nullptr;
  IndicatorFlickSequence *nightRiderFlickSequence = nullptr;
  BrakeTapSequence *brakeTapSequence3 = nullptr;

  // Application Mode
  ApplicationMode mode;
  ApplicationMode prevMode;

private:
  void updateInputs();
  void setupWireless();
  void setupBLE();

  bool appInitialized;

  // Menu system
  MenuContext menuContext;

  // Internal method to handle effect selection based on inputs.
  void handleNormalEffects();
  void handleTestEffects();
  void handleRemoteEffects();
  void handleSyncedEffects(const EffectSyncState &effectState);

  uint64_t lastRemotePing;
  AppStats stats;
};
