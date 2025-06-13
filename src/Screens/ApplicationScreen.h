#pragma once

#include "config.h"
#include "IO/Display.h"
#include "IO/GPIO.h"
#include "IO/Menu.h"
#include "Application.h"
#include "IO/LED/LEDStripManager.h"
#include "IO/StatusLed.h"

class ApplicationScreen : public Screen
{
public:
  ApplicationScreen(String _name);

  Menu menu = Menu(MenuSize::Medium);

  MenuItemBack backItem;

  Application *app;

  // Mode control
  std::vector<String> modeItems = {"Normal", "Test", "Remote", "Off"};
  MenuItemSelect modeSelectItem = MenuItemSelect("Mode", modeItems, 0);

  // Effects control
  MenuItemToggle leftIndicatorItem = MenuItemToggle("Left Ind", &leftIndicatorActive, true);
  MenuItemToggle rightIndicatorItem = MenuItemToggle("Right Ind", &rightIndicatorActive, true);

  // Headlight controls
  std::vector<String> headlightModeItems = {"Off", "Startup", "On"};
  MenuItemSelect headlightModeItem = MenuItemSelect("H.Mode", headlightModeItems, 0);
  MenuItemToggle headlightSplitItem = MenuItemToggle("H.Split", &headlightSplit, true);
  MenuItemToggle headlightRItem = MenuItemToggle("H.R", &headlightR, true);
  MenuItemToggle headlightGItem = MenuItemToggle("H.G", &headlightG, true);
  MenuItemToggle headlightBItem = MenuItemToggle("H.B", &headlightB, true);

  // Taillight controls
  std::vector<String> taillightModeItems = {"Off", "Startup", "On", "Dim"};
  MenuItemSelect taillightModeItem = MenuItemSelect("T.Mode", taillightModeItems, 0);
  MenuItemToggle taillightSplitItem = MenuItemToggle("T.Split", &taillightSplit, true);

  // Additional effects
  MenuItemToggle rgbEffectItem = MenuItemToggle("RGB", &rgbEffectActive, true);
  MenuItemToggle nightriderEffectItem = MenuItemToggle("N.Rider", &nightriderEffectActive, true);
  MenuItemToggle policeEffectItem = MenuItemToggle("Police", &policeEffectActive, true);
  std::vector<String> policeModeItems = {"Slow", "Fast"};
  MenuItemSelect policeModeItem = MenuItemSelect("P.Mode", policeModeItems, 0);
  MenuItemToggle pulseWaveEffectItem = MenuItemToggle("Pulse", &pulseWaveEffectActive, true);
  MenuItemToggle auroraEffectItem = MenuItemToggle("Aurora", &auroraEffectActive, true);
  MenuItemToggle solidColorEffectItem = MenuItemToggle("Solid", &solidColorEffectActive, true);

  // Solid color controls
  std::vector<String> solidColorPresetItems = {"Off", "Red", "Green", "Blue", "White", "Yellow", "Cyan", "Magenta", "Orange", "Purple", "Lime", "Pink", "Teal", "Indigo", "Gold", "Silver", "Custom"};
  MenuItemSelect solidColorPresetItem = MenuItemSelect("SC Col", solidColorPresetItems, 0);
  MenuItemNumber<uint8_t> solidColorRItem = MenuItemNumber<uint8_t>("SC R", &solidColorR, 0, 255, 5);
  MenuItemNumber<uint8_t> solidColorGItem = MenuItemNumber<uint8_t>("SC G", &solidColorG, 0, 255, 5);
  MenuItemNumber<uint8_t> solidColorBItem = MenuItemNumber<uint8_t>("SC B", &solidColorB, 0, 255, 5);

  // Test inputs (for test mode)
  MenuItemToggle accOnItem = MenuItemToggle("ACC", &accOnOverride, true);
  MenuItemToggle indicatorLeftItem = MenuItemToggle("Ind L", &indicatorLeftOverride, true);
  MenuItemToggle indicatorRightItem = MenuItemToggle("Ind R", &indicatorRightOverride, true);
  MenuItemToggle headlightItem = MenuItemToggle("H.Light", &headlightOverride, true);
  MenuItemToggle brakeItem = MenuItemToggle("Brake", &brakeOverride, true);
  MenuItemToggle reverseItem = MenuItemToggle("Reverse", &reverseOverride, true);

  // Sequence triggers
  MenuItemAction unlockSequenceItem = MenuItemAction("Unlock Seq", 1, [&]()
                                                     { triggerUnlockSequence(); });
  MenuItemAction lockSequenceItem = MenuItemAction("Lock Seq", 1, [&]()
                                                   { triggerLockSequence(); });
  MenuItemAction rgbSequenceItem = MenuItemAction("RGB Seq", 1, [&]()
                                                  { triggerRGBSequence(); });
  MenuItemAction nightRiderSequenceItem = MenuItemAction("NR Seq", 1, [&]()
                                                         { triggerNightRiderSequence(); });

  void draw() override;
  void update() override;
  void onEnter() override;

private:
  // Effect states
  bool leftIndicatorActive = false;
  bool rightIndicatorActive = false;

  int headlightMode = 0;
  bool headlightSplit = false;
  bool headlightR = false;
  bool headlightG = false;
  bool headlightB = false;

  int taillightMode = 0;
  bool taillightSplit = false;

  bool rgbEffectActive = false;
  bool nightriderEffectActive = false;
  bool policeEffectActive = false;
  PoliceMode policeMode = PoliceMode::SLOW;
  bool pulseWaveEffectActive = false;
  bool auroraEffectActive = false;
  bool solidColorEffectActive = false;
  SolidColorPreset solidColorPreset = SolidColorPreset::OFF;
  uint8_t solidColorR = 255;
  uint8_t solidColorG = 255;
  uint8_t solidColorB = 255;

  // Test mode input overrides
  bool accOnOverride = false;
  bool indicatorLeftOverride = false;
  bool indicatorRightOverride = false;
  bool headlightOverride = false;
  bool brakeOverride = false;
  bool reverseOverride = false;

  // Helper methods
  void updateEffectsFromApplication();
  void applyEffectsToApplication();
  void updateModeDisplay();
  void triggerUnlockSequence();
  void triggerLockSequence();
  void triggerRGBSequence();
  void triggerNightRiderSequence();
};

ApplicationScreen::ApplicationScreen(String _name) : Screen(_name)
{
  app = Application::getInstance();

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
  modeSelectItem.setOnChange([&]()
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
  leftIndicatorItem.setOnChange([&]()
                                { applyEffectsToApplication(); });
  rightIndicatorItem.setOnChange([&]()
                                 { applyEffectsToApplication(); });
  rgbEffectItem.setOnChange([&]()
                            { applyEffectsToApplication(); });
  nightriderEffectItem.setOnChange([&]()
                                   { applyEffectsToApplication(); });
  policeEffectItem.setOnChange([&]()
                               { applyEffectsToApplication(); });
  pulseWaveEffectItem.setOnChange([&]()
                                  { applyEffectsToApplication(); });
  auroraEffectItem.setOnChange([&]()
                               { applyEffectsToApplication(); });
  solidColorEffectItem.setOnChange([&]()
                                   { applyEffectsToApplication(); });

  // Mode change callbacks
  headlightModeItem.setOnChange([&]()
                                {
                                  headlightMode = headlightModeItem.getCurrentIndex();
                                  applyEffectsToApplication(); });
  taillightModeItem.setOnChange([&]()
                                {
                                  taillightMode = taillightModeItem.getCurrentIndex();
                                  applyEffectsToApplication(); });
  policeModeItem.setOnChange([&]()
                             {
                               policeMode = static_cast<PoliceMode>(policeModeItem.getCurrentIndex());
                               applyEffectsToApplication(); });
  solidColorPresetItem.setOnChange([&]()
                                   {
                                     solidColorPreset = static_cast<SolidColorPreset>(solidColorPresetItem.getCurrentIndex());
                                     applyEffectsToApplication(); });

  // Split and color callbacks
  headlightSplitItem.setOnChange([&]()
                                 { applyEffectsToApplication(); });
  taillightSplitItem.setOnChange([&]()
                                 { applyEffectsToApplication(); });
  headlightRItem.setOnChange([&]()
                             { applyEffectsToApplication(); });
  headlightGItem.setOnChange([&]()
                             { applyEffectsToApplication(); });
  headlightBItem.setOnChange([&]()
                             { applyEffectsToApplication(); });

  // Solid color RGB callbacks
  solidColorRItem.setOnChange([&]()
                              { applyEffectsToApplication(); });
  solidColorGItem.setOnChange([&]()
                              { applyEffectsToApplication(); });
  solidColorBItem.setOnChange([&]()
                              { applyEffectsToApplication(); });

  // Test input callbacks
  accOnItem.setOnChange([&]()
                        { applyEffectsToApplication(); });
  indicatorLeftItem.setOnChange([&]()
                                { applyEffectsToApplication(); });
  indicatorRightItem.setOnChange([&]()
                                 { applyEffectsToApplication(); });
  headlightItem.setOnChange([&]()
                            { applyEffectsToApplication(); });
  brakeItem.setOnChange([&]()
                        { applyEffectsToApplication(); });
  reverseItem.setOnChange([&]()
                          { applyEffectsToApplication(); });

  // Enable fast update for color sliders
  solidColorRItem.setFastUpdate(true);
  solidColorGItem.setFastUpdate(true);
  solidColorBItem.setFastUpdate(true);
}

void ApplicationScreen::draw()
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

void ApplicationScreen::update()
{
  updateEffectsFromApplication();
  menu.update();
}

void ApplicationScreen::onEnter()
{
  updateEffectsFromApplication();
  updateModeDisplay();
}

void ApplicationScreen::updateEffectsFromApplication()
{
  // This would update the UI from the current application state
  // Implementation depends on how the Application class exposes its state
  // For now, we'll keep track of state internally and apply changes
}

void ApplicationScreen::applyEffectsToApplication()
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

void ApplicationScreen::updateModeDisplay()
{
  // Update mode display based on current application mode
  // This should reflect the actual application state
}

void ApplicationScreen::triggerUnlockSequence()
{
  // Trigger unlock sequence
  display.showNotification("Unlock sequence triggered", 1500);
}

void ApplicationScreen::triggerLockSequence()
{
  // Trigger lock sequence
  display.showNotification("Lock sequence triggered", 1500);
}

void ApplicationScreen::triggerRGBSequence()
{
  // Trigger RGB sequence
  display.showNotification("RGB sequence triggered", 1500);
}

void ApplicationScreen::triggerNightRiderSequence()
{
  // Trigger Night Rider sequence
  display.showNotification("Night Rider sequence triggered", 1500);
}