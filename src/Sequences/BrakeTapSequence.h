#pragma once

#include <Arduino.h>
#include "SequenceBase.h"

class BrakeTapSequence : public SequenceBase {
public:
  // Constructor: takes the desired number of brake taps to trigger the sequence.
  // timeoutMs: time in milliseconds to wait after a tap to see if another tap comes.
  BrakeTapSequence(int desiredTapCount);

  // Called in the main loop.
  // Returns true when the full sequence is detected.
  bool update() override;

  // Resets the sequence tracking so it is ready for a new sequence.
  void VReset() override;

  // Update the current input state for the brake (e.g., using a digital read or
  // similar). This should be updated externally.
  void setInput(bool brakeState);

private:
  // Desired number of taps for this sequence.
  int desiredTapCount;

  // Current number of taps observed.
  int currentTapCount;

  // Timeout after a tap (in ms) to wait for another tap.
  const unsigned long interTapTimeout;

  // The time of the last detected tap.
  unsigned long lastTapTime;

  // Debounce time to avoid multiple triggers per tap.
  const unsigned long debounceTime;
  unsigned long lastDebounceTime;

  // Current input state.
  bool brakeState;

  // Previous input state (for rising edge detection).
  bool lastBrakeState;
};
