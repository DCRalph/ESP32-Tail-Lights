#include "BrakeTapSequence.h"

BrakeTapSequence::BrakeTapSequence(int desiredTapCount)
    : SequenceBase("BrakeTapSequence", 1000 * desiredTapCount),
      desiredTapCount(desiredTapCount),
      currentTapCount(0),
      interTapTimeout(500),
      lastTapTime(0),
      debounceTime(50),
      lastDebounceTime(0),
      brakeState(false),
      lastBrakeState(false)
{
}

void BrakeTapSequence::VReset()
{
  currentTapCount = 0;
  lastTapTime = 0;
  lastDebounceTime = millis();
  lastBrakeState = false;
}

void BrakeTapSequence::setInput(bool brakeState)
{
  // Update the input state from external sensor/logic.
  this->brakeState = brakeState;
}

bool BrakeTapSequence::update()
{
  unsigned long currentTime = millis();

  // Check for rising edge of the brake tap.
  bool risingEdge = (brakeState && !lastBrakeState &&
                     (currentTime - lastDebounceTime >= debounceTime));

  if (risingEdge)
  {
    // Increase the tap count.
    currentTapCount++;
    lastTapTime = currentTime;
    lastDebounceTime = currentTime;
    start();
  }

  // Save the current state for next call.
  lastBrakeState = brakeState;

  // If at least one tap has occurred, check if enough time has passed without a new tap.
  if (currentTapCount > 0)
  {
    if (currentTime - lastTapTime >= interTapTimeout)
    {
      // The inter-tap period has elapsed.
      if (currentTapCount == desiredTapCount)
      {
        // Correct number of taps detected.
        return true;
      }
      else
      {
        // The tap count does not match the desired count.
        reset();
      }
    }
  }

  return false;
}
