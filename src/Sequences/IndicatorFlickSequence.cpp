#include "IndicatorFlickSequence.h"

IndicatorFlickSequence::IndicatorFlickSequence(IndicatorSide startSide)
    : SequenceBase("IndicatorFlickSequence" + String(startSide), 3000),
      currentIndex(0),
      firstFlashTime(0),
      debounceTime(50),
      lastDebounceTime(0),
      accOnState(false),
      leftIndicatorState(false),
      rightIndicatorState(false),
      lastLeftState(false),
      lastRightState(false),
      hazardStartTime(0)
{
  if (startSide == IndicatorSide::LEFT_SIDE)
  {
    expectedSequence[0] = IndicatorSide::LEFT_SIDE;
    expectedSequence[1] = IndicatorSide::RIGHT_SIDE;
    expectedSequence[2] = IndicatorSide::LEFT_SIDE;
    expectedSequence[3] = IndicatorSide::RIGHT_SIDE;
  }
  else
  {
    expectedSequence[0] = IndicatorSide::RIGHT_SIDE;
    expectedSequence[1] = IndicatorSide::LEFT_SIDE;
    expectedSequence[2] = IndicatorSide::RIGHT_SIDE;
    expectedSequence[3] = IndicatorSide::LEFT_SIDE;
  }
}

bool IndicatorFlickSequence::update()
{
  unsigned long currentTime = millis();

  // Process this sequence only when ACC is on.
  if (accOnState)
  {

    if (leftIndicatorState && rightIndicatorState)
    {
      if (hazardStartTime == 0)
      {
        // Start tracking the time since both indicators turned on.
        hazardStartTime = currentTime;
      }
      else if ((currentTime - hazardStartTime) >= 200)
      {
        reset();
        Serial.println("IndicatorFlick-" + String(expectedSequence[0]) + " Both indicators are on for 200ms. Resetting sequence.");
        return false;
      }
    }
    else
    {
      // Reset hazard timer if either indicator is off.
      hazardStartTime = 0;
    }

    // Check for rising edge for left indicator.
    bool leftEdge = leftIndicatorState && !lastLeftState &&
                    ((currentTime - lastDebounceTime) >= debounceTime);
    // Check for rising edge for right indicator.
    bool rightEdge = rightIndicatorState && !lastRightState &&
                     ((currentTime - lastDebounceTime) >= debounceTime);

    // Determine which side is expected for the current step.
    IndicatorSide expected = expectedSequence[currentIndex];

    // If the expected side's rising edge is detected, advance the sequence.
    if (expected == LEFT_SIDE && leftEdge)
    {
      if (firstFlashTime == 0)
      {
        firstFlashTime = currentTime;
      }
      start();
      currentIndex++;
      lastDebounceTime = currentTime;
    }
    else if (expected == RIGHT_SIDE && rightEdge)
    {
      if (firstFlashTime == 0)
      {
        firstFlashTime = currentTime;
      }
      start();
      currentIndex++;
      lastDebounceTime = currentTime;
    }
    // If an indicator flash (rising edge) is detected but it does not match the expected side,
    // then reset the sequence.
    else if ((leftEdge || rightEdge) && !((expected == LEFT_SIDE && leftEdge) ||
                                          (expected == RIGHT_SIDE && rightEdge)))
    {
      reset();
    }

    // Save states for the next call.
    lastLeftState = leftIndicatorState;
    lastRightState = rightIndicatorState;

    // If the sequence is complete, return true.
    if (currentIndex >= SEQUENCE_LENGTH)
    {
      return true;
    }
  }
  else
  {
    // When ACC is off, reset the sequence.
    if(currentIndex > 0 || started)
    {
      reset();
    }
  }
  return false;
}

void IndicatorFlickSequence::VReset()
{
  currentIndex = 0;
  firstFlashTime = 0;
  // Reset debounce timer so that a new flash is counted correctly.
  lastDebounceTime = millis();
  lastLeftState = false;
  lastRightState = false;
}

void IndicatorFlickSequence::setInputs(bool accOnState,
                                       bool leftIndicatorState,
                                       bool rightIndicatorState)
{
  this->accOnState = accOnState;
  this->leftIndicatorState = leftIndicatorState;
  this->rightIndicatorState = rightIndicatorState;

  // Serial.print("ACC: ");
  // Serial.print(accOnState ? "ON " : "OFF");
  // Serial.print(" Left: ");
  // Serial.print(leftIndicatorState ? "ON " : "OFF");
  // Serial.print(" Right: ");
  // Serial.println(rightIndicatorState ? "ON " : "OFF");
}
