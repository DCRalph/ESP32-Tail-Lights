#include "BothIndicatorsSequence.h"

BothIndicatorsSequence::BothIndicatorsSequence(int _flashes)
    : SequenceBase("BothIndicators-" + String(_flashes), 10000),
      requiredFlashes(_flashes),
      flashCounter(0),
      firstFlashTime(0),
      lastCombinedState(false),
      debounceTime(50),
      lastDebounceTime(0),
      accOnState(false),
      leftIndicatorState(false),
      rightIndicatorState(false)
{
}

bool BothIndicatorsSequence::update()
{
  unsigned long currentTime = millis();

  // Only process if ACC is off.
  if (!accOnState)
  {
    bool combinedState = (leftIndicatorState && rightIndicatorState);

    // Check for rising edge of combined state (with debounce).
    if (combinedState && !lastCombinedState &&
        (currentTime - lastDebounceTime >= debounceTime))
    {
      if (firstFlashTime == 0)
      {
        // record when we saw the first flash
        firstFlashTime = currentTime;
      }
      flashCounter++;
      start();
      lastDebounceTime = currentTime;
    }

    lastCombinedState = combinedState;

    // Check if we got the required number of flashes.
    if (flashCounter >= requiredFlashes)
    {
      return true;
    }
  }
  return false;
}

void BothIndicatorsSequence::VReset()
{
  flashCounter = 0;
  firstFlashTime = 0;
  lastCombinedState = false; 
  lastDebounceTime = millis();
}

void BothIndicatorsSequence::setInputs(bool accOnState, bool leftIndicatorState, bool rightIndicatorState)
{
  this->accOnState = accOnState;
  this->leftIndicatorState = leftIndicatorState;
  this->rightIndicatorState = rightIndicatorState;
}