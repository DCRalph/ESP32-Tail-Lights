#pragma once
#include <Arduino.h>
#include "SequenceBase.h"

class BothIndicatorsSequence : public SequenceBase
{
public:
  BothIndicatorsSequence(int _flashes);

  // Call this in the main loop.
  // Assumes that accOnState, leftIndicatorState and rightIndicatorState are updated globals.
  // For example, you could update these variables from Application::handleNormalEffects().
  bool update() override;
  void VReset() override;

  void setInputs(bool accOnState, bool leftIndicatorState, bool rightIndicatorState);

private:
  int requiredFlashes;
  int flashCounter;
  unsigned long firstFlashTime;
  bool lastCombinedState;
  const unsigned long debounceTime; // in ms
  unsigned long lastDebounceTime;

  bool accOnState;
  bool leftIndicatorState;
  bool rightIndicatorState;
};
