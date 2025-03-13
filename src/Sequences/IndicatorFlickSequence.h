#pragma once

#include <Arduino.h>
#include "SequenceBase.h"

enum IndicatorSide
{
  LEFT_SIDE,
  RIGHT_SIDE
};

class IndicatorFlickSequence : public SequenceBase
{
public:
  // Constructor: provide timeout (in ms) if needed.
  IndicatorFlickSequence(IndicatorSide startSide);

  // Called in the main loop.
  // Returns true when the full sequence is detected.
  bool update() override;

  // Resets the sequence tracking so it is ready for a new sequence.
  void VReset() override;

  // Update the current input state.
  // Assumes that accOnState is true (the sequence only works when ACC is on).
  // leftIndicatorState/rightIndicatorState should be updated externally.
  void setInputs(bool accOnState, bool leftIndicatorState, bool rightIndicatorState);

private:
  // The fixed expected sequence.
  static const int SEQUENCE_LENGTH = 4;
  IndicatorSide expectedSequence[SEQUENCE_LENGTH];

  int currentIndex;                 // which step we are expecting next
  unsigned long firstFlashTime;     // time when the first flash of the sequence was detected
  const unsigned long debounceTime; // in ms
  unsigned long lastDebounceTime;

  // Input states:
  bool accOnState;
  bool leftIndicatorState;
  bool rightIndicatorState;

  // For rising edge detection on each indicator:
  bool lastLeftState;
  bool lastRightState;

  // handle hazzards
  uint64_t hazardStartTime;
};
