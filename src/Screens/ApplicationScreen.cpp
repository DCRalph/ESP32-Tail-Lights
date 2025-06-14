#include "ApplicationScreen.h"
#include "IO/Display.h"
#include "IO/GPIO.h"
#include "IO/Menu.h"
#include "Application.h"
#include "IO/LED/LEDStripManager.h"
#include "IO/StatusLed.h"
#include "IO/ScreenManager.h"

namespace ApplicationScreenNamespace
{
  // Application state variables
  static Application *app;

  // Effect states
  static bool leftIndicatorActive = false;
  static bool rightIndicatorActive = false;

  static int headlightMode = 0;
  static bool headlightSplit = false;
  static bool headlightR = false;
  static bool headlightG = false;
  static bool headlightB = false;

  static int taillightMode = 0;
  static bool taillightSplit = false;

  static bool rgbEffectActive = false;
  static bool nightriderEffectActive = false;
  static bool policeEffectActive = false;
  static PoliceMode policeMode = PoliceMode::SLOW;
  static bool pulseWaveEffectActive = false;
  static bool auroraEffectActive = false;
  static bool solidColorEffectActive = false;
  static SolidColorPreset solidColorPreset = SolidColorPreset::OFF;
  static uint8_t solidColorR = 255;
  static uint8_t solidColorG = 255;
  static uint8_t solidColorB = 255;

  // Test mode input overrides
  static bool accOnOverride = false;
  static bool indicatorLeftOverride = false;
  static bool indicatorRightOverride = false;
  static bool headlightOverride = false;
  static bool brakeOverride = false;
  static bool reverseOverride = false;

  // Menu instance
  static Menu menu = Menu(MenuSize::Medium);

  // Menu items
  static MenuItemBack backItem;

  // Mode control
  static std::vector<String> modeItems = {"Normal", "Test", "Remote", "Off"};
  static MenuItemSelect modeSelectItem = MenuItemSelect("Mode", modeItems, 0);

  // Effects control
  static MenuItemToggle leftIndicatorItem = MenuItemToggle("Left Ind", &leftIndicatorActive, true);
  static MenuItemToggle rightIndicatorItem = MenuItemToggle("Right Ind", &rightIndicatorActive, true);

  // Headlight controls
  static std::vector<String> headlightModeItems = {"Off", "Startup", "On"};
  static MenuItemSelect headlightModeItem = MenuItemSelect("H.Mode", headlightModeItems, 0);
  static MenuItemToggle headlightSplitItem = MenuItemToggle("H.Split", &headlightSplit, true);
  static MenuItemToggle headlightRItem = MenuItemToggle("H.R", &headlightR, true);
  static MenuItemToggle headlightGItem = MenuItemToggle("H.G", &headlightG, true);
  static MenuItemToggle headlightBItem = MenuItemToggle("H.B", &headlightB, true);

  // Taillight controls
  static std::vector<String> taillightModeItems = {"Off", "Startup", "On", "Dim"};
  static MenuItemSelect taillightModeItem = MenuItemSelect("T.Mode", taillightModeItems, 0);
  static MenuItemToggle taillightSplitItem = MenuItemToggle("T.Split", &taillightSplit, true);

  // Additional effects
  static MenuItemToggle rgbEffectItem = MenuItemToggle("RGB", &rgbEffectActive, true);
  static MenuItemToggle nightriderEffectItem = MenuItemToggle("N.Rider", &nightriderEffectActive, true);
  static MenuItemToggle policeEffectItem = MenuItemToggle("Police", &policeEffectActive, true);
  static std::vector<String> policeModeItems = {"Slow", "Fast"};
  static MenuItemSelect policeModeItem = MenuItemSelect("P.Mode", policeModeItems, 0);
  static MenuItemToggle pulseWaveEffectItem = MenuItemToggle("Pulse", &pulseWaveEffectActive, true);
  static MenuItemToggle auroraEffectItem = MenuItemToggle("Aurora", &auroraEffectActive, true);
  static MenuItemToggle solidColorEffectItem = MenuItemToggle("Solid", &solidColorEffectActive, true);

  // Solid color controls
  static std::vector<String> solidColorPresetItems = {"Off", "Red", "Green", "Blue", "White", "Yellow", "Cyan", "Magenta", "Orange", "Purple", "Lime", "Pink", "Teal", "Indigo", "Gold", "Silver", "Custom"};
  static MenuItemSelect solidColorPresetItem = MenuItemSelect("SC Col", solidColorPresetItems, 0);
  static MenuItemNumber<uint8_t> solidColorRItem = MenuItemNumber<uint8_t>("SC R", &solidColorR, 0, 255, 5);
  static MenuItemNumber<uint8_t> solidColorGItem = MenuItemNumber<uint8_t>("SC G", &solidColorG, 0, 255, 5);
  static MenuItemNumber<uint8_t> solidColorBItem = MenuItemNumber<uint8_t>("SC B", &solidColorB, 0, 255, 5);

  // Test inputs (for test mode)
  static MenuItemToggle accOnItem = MenuItemToggle("ACC", &accOnOverride, true);
  static MenuItemToggle indicatorLeftItem = MenuItemToggle("Ind L", &indicatorLeftOverride, true);
  static MenuItemToggle indicatorRightItem = MenuItemToggle("Ind R", &indicatorRightOverride, true);
  static MenuItemToggle headlightItem = MenuItemToggle("H.Light", &headlightOverride, true);
  static MenuItemToggle brakeItem = MenuItemToggle("Brake", &brakeOverride, true);
  static MenuItemToggle reverseItem = MenuItemToggle("Reverse", &reverseOverride, true);

  // Forward declare sequence functions
  void triggerUnlockSequence();
  void triggerLockSequence();
  void triggerRGBSequence();
  void triggerNightRiderSequence();

  // Sequence triggers
  static MenuItemAction unlockSequenceItem = MenuItemAction("Unlock Seq", 1, []()
                                                            { triggerUnlockSequence(); });
  static MenuItemAction lockSequenceItem = MenuItemAction("Lock Seq", 1, []()
                                                          { triggerLockSequence(); });
  static MenuItemAction rgbSequenceItem = MenuItemAction("RGB Seq", 1, []()
                                                         { triggerRGBSequence(); });
  static MenuItemAction nightRiderSequenceItem = MenuItemAction("NR Seq", 1, []()
                                                                { triggerNightRiderSequence(); });

  // Helper method forward declarations
  void updateEffectsFromApplication();
  void applyEffectsToApplication();
  void updateModeDisplay();

  // Helper method implementations
  void triggerUnlockSequence()
  {
    // Trigger unlock sequence
    display.showNotification("Unlock sequence triggered", 1500);
  }

  void triggerLockSequence()
  {
    // Trigger lock sequence
    display.showNotification("Lock sequence triggered", 1500);
  }

  void triggerRGBSequence()
  {
    // Trigger RGB sequence
    display.showNotification("RGB sequence triggered", 1500);
  }

  void triggerNightRiderSequence()
  {
    // Trigger Night Rider sequence
    display.showNotification("Night Rider sequence triggered", 1500);
  }

  // onEnter function
  void applicationScreenOnEnter()
  {
    // Setup menu structure
    menu.addMenuItem(&backItem);
    menu.addMenuItem(&modeSelectItem);

    // Effects
    menu.addMenuItem(&leftIndicatorItem);
    menu.addMenuItem(&rightIndicatorItem);

    // Headlight controls
    menu.addMenuItem(&headlightModeItem);
    menu.addMenuItem(&headlightSplitItem);
    menu.addMenuItem(&headlightRItem);
    menu.addMenuItem(&headlightGItem);
    menu.addMenuItem(&headlightBItem);

    // Taillight controls
    menu.addMenuItem(&taillightModeItem);
    menu.addMenuItem(&taillightSplitItem);

    // Additional effects
    menu.addMenuItem(&rgbEffectItem);
    menu.addMenuItem(&nightriderEffectItem);
    menu.addMenuItem(&policeEffectItem);
    menu.addMenuItem(&policeModeItem);
    menu.addMenuItem(&pulseWaveEffectItem);
    menu.addMenuItem(&auroraEffectItem);
    menu.addMenuItem(&solidColorEffectItem);
    menu.addMenuItem(&solidColorPresetItem);
    menu.addMenuItem(&solidColorRItem);
    menu.addMenuItem(&solidColorGItem);
    menu.addMenuItem(&solidColorBItem);

    // Test mode inputs
    menu.addMenuItem(&accOnItem);
    menu.addMenuItem(&indicatorLeftItem);
    menu.addMenuItem(&indicatorRightItem);
    menu.addMenuItem(&headlightItem);
    menu.addMenuItem(&brakeItem);
    menu.addMenuItem(&reverseItem);

    // Sequences
    menu.addMenuItem(&unlockSequenceItem);
    menu.addMenuItem(&lockSequenceItem);
    menu.addMenuItem(&rgbSequenceItem);
    menu.addMenuItem(&nightRiderSequenceItem);

    // Set up callbacks
    modeSelectItem.setOnChange([]()
                               {
      int mode = modeSelectItem.getCurrentIndex();
      switch (mode)
      {
      case 0:
        app->enableNormalMode();
        break;
      case 1:
        app->enableTestMode();
        break;
      case 2:
        app->enableRemoteMode();
        break;
      case 3:
        app->enableOffMode();
        break;
      }
      updateModeDisplay(); });

    // Effect callbacks
    leftIndicatorItem.setOnChange([]()
                                  { applyEffectsToApplication(); });
    rightIndicatorItem.setOnChange([]()
                                   { applyEffectsToApplication(); });
    rgbEffectItem.setOnChange([]()
                              { applyEffectsToApplication(); });
    nightriderEffectItem.setOnChange([]()
                                     { applyEffectsToApplication(); });
    policeEffectItem.setOnChange([]()
                                 { applyEffectsToApplication(); });
    pulseWaveEffectItem.setOnChange([]()
                                    { applyEffectsToApplication(); });
    auroraEffectItem.setOnChange([]()
                                 { applyEffectsToApplication(); });
    solidColorEffectItem.setOnChange([]()
                                     { applyEffectsToApplication(); });

    // Mode change callbacks
    headlightModeItem.setOnChange([]()
                                  {
      headlightMode = headlightModeItem.getCurrentIndex();
      applyEffectsToApplication(); });
    taillightModeItem.setOnChange([]()
                                  {
      taillightMode = taillightModeItem.getCurrentIndex();
      applyEffectsToApplication(); });
    policeModeItem.setOnChange([]()
                               {
      policeMode = static_cast<PoliceMode>(policeModeItem.getCurrentIndex());
      applyEffectsToApplication(); });
    solidColorPresetItem.setOnChange([]()
                                     {
      solidColorPreset = static_cast<SolidColorPreset>(solidColorPresetItem.getCurrentIndex());
      applyEffectsToApplication(); });

    // Split and color callbacks
    headlightSplitItem.setOnChange([]()
                                   { applyEffectsToApplication(); });
    taillightSplitItem.setOnChange([]()
                                   { applyEffectsToApplication(); });
    headlightRItem.setOnChange([]()
                               { applyEffectsToApplication(); });
    headlightGItem.setOnChange([]()
                               { applyEffectsToApplication(); });
    headlightBItem.setOnChange([]()
                               { applyEffectsToApplication(); });

    // Solid color RGB callbacks
    solidColorRItem.setOnChange([]()
                                { applyEffectsToApplication(); });
    solidColorGItem.setOnChange([]()
                                { applyEffectsToApplication(); });
    solidColorBItem.setOnChange([]()
                                { applyEffectsToApplication(); });

    // Test input callbacks
    accOnItem.setOnChange([]()
                          { applyEffectsToApplication(); });
    indicatorLeftItem.setOnChange([]()
                                  { applyEffectsToApplication(); });
    indicatorRightItem.setOnChange([]()
                                   { applyEffectsToApplication(); });
    headlightItem.setOnChange([]()
                              { applyEffectsToApplication(); });
    brakeItem.setOnChange([]()
                          { applyEffectsToApplication(); });
    reverseItem.setOnChange([]()
                            { applyEffectsToApplication(); });

    // Enable fast update for color sliders
    solidColorRItem.setFastUpdate(true);
    solidColorGItem.setFastUpdate(true);
    solidColorBItem.setFastUpdate(true);

    // Initialize application
    app = Application::getInstance();
    updateEffectsFromApplication();
    updateModeDisplay();
  }

  // onExit function
  void applicationScreenOnExit()
  {
    // Clean up allocated memory
  }

  // draw function
  void applicationScreenDraw()
  {
    updateModeDisplay();

    // Hide/show controls based on current mode
    ApplicationMode currentMode = static_cast<ApplicationMode>(modeSelectItem.getCurrentIndex());

    bool isTestMode = (currentMode == ApplicationMode::TEST);
    bool isOffMode = (currentMode == ApplicationMode::OFF);

    // Hide most effects in OFF mode
    leftIndicatorItem.setHidden(isOffMode);
    rightIndicatorItem.setHidden(isOffMode);
    headlightModeItem.setHidden(isOffMode);
    headlightSplitItem.setHidden(isOffMode);
    headlightRItem.setHidden(isOffMode);
    headlightGItem.setHidden(isOffMode);
    headlightBItem.setHidden(isOffMode);
    taillightModeItem.setHidden(isOffMode);
    taillightSplitItem.setHidden(isOffMode);
    rgbEffectItem.setHidden(isOffMode);
    nightriderEffectItem.setHidden(isOffMode);
    policeEffectItem.setHidden(isOffMode);
    policeModeItem.setHidden(isOffMode);
    pulseWaveEffectItem.setHidden(isOffMode);
    auroraEffectItem.setHidden(isOffMode);
    solidColorEffectItem.setHidden(isOffMode);
    solidColorPresetItem.setHidden(isOffMode);
    solidColorRItem.setHidden(isOffMode);
    solidColorGItem.setHidden(isOffMode);
    solidColorBItem.setHidden(isOffMode);

    // Show test inputs only in test mode
    accOnItem.setHidden(!isTestMode);
    indicatorLeftItem.setHidden(!isTestMode);
    indicatorRightItem.setHidden(!isTestMode);
    headlightItem.setHidden(!isTestMode);
    brakeItem.setHidden(!isTestMode);
    reverseItem.setHidden(!isTestMode);

    // Show sequences in normal mode only
    unlockSequenceItem.setHidden(isTestMode || isOffMode);
    lockSequenceItem.setHidden(isTestMode || isOffMode);
    rgbSequenceItem.setHidden(isTestMode || isOffMode);
    nightRiderSequenceItem.setHidden(isTestMode || isOffMode);

    menu.draw();
  }

  // update function
  void applicationScreenUpdate()
  {
    updateEffectsFromApplication();
    menu.update();
  }

  // Helper method implementations
  void updateEffectsFromApplication()
  {
    // This would update the UI from the current application state
    // Implementation depends on how the Application class exposes its state
    // For now, we'll keep track of state internally and apply changes
  }

  void applyEffectsToApplication()
  {
    if (!app)
      return;

    LEDStripManager *ledManager = LEDStripManager::getInstance();

    // Apply effects based on current application state
    // This is a simplified version - you may need to adjust based on
    // how the Application class actually manages effects

    // For test mode, apply input overrides
    ApplicationMode currentMode = static_cast<ApplicationMode>(modeSelectItem.getCurrentIndex());
    if (currentMode == ApplicationMode::TEST)
    {
      // Apply test input overrides
      // This would require access to the Application's input system
      // Implementation depends on how inputs are exposed
    }
  }

  void updateModeDisplay()
  {
    // Update mode display based on current application mode
    // This should reflect the actual application state
  }

} // namespace ApplicationScreenNamespace

// Define the ApplicationScreen Screen2 instance
const Screen2 ApplicationScreen = {
    F("Application"),
    F("Application"),
    ApplicationScreenNamespace::applicationScreenDraw,
    ApplicationScreenNamespace::applicationScreenUpdate,
    ApplicationScreenNamespace::applicationScreenOnEnter,
    ApplicationScreenNamespace::applicationScreenOnExit};