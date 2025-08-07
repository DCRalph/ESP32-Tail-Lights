#include "PoliceEffect.h"
#include <Arduino.h>
#include <math.h>

// Helper function: Easing function for smoother transitions
static float easeInOut(float t)
{
  return 0.5f * (1.0f - cosf(t * PI));
}

PoliceEffect::PoliceEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      active(false),
      mode(PoliceMode::FAST),
      lastUpdateTime(0),
      flashProgress(0.0f),
      cycleProgress(0.0f),
      fastSpeed(0.5f),            // 0.5 seconds per full flash cycle in fast mode
      slowSpeed(1.0f),            // 1.0 seconds per full flash cycle in slow mode
      fastModeFlashesPerCycle(3), // 3 flashes per color cycle
      blueColor(Color(0, 0, 255)),
      redColor(Color(255, 0, 0)),
      currentFlash(0)
{
  name = "Police";
}

void PoliceEffect::setActive(bool a)
{
  if (active == a)
    return;

  active = a;
  if (active)
  {
    // Reset animation state when activating
    lastUpdateTime = millis();
    flashProgress = 0.0f;
    cycleProgress = 0.0f;
    currentFlash = 0;
  }
}

bool PoliceEffect::isActive() const
{
  return active;
}

void PoliceEffect::setSyncData(PoliceSyncData syncData)
{
  active = syncData.active;
  mode = syncData.mode;
  flashProgress = syncData.flashProgress;
  cycleProgress = syncData.cycleProgress;
  currentFlash = syncData.currentFlash;
}

PoliceSyncData PoliceEffect::getSyncData()
{
  return PoliceSyncData{
      .active = active,
      .mode = mode,
      .flashProgress = flashProgress,
      .cycleProgress = cycleProgress,
      .currentFlash = currentFlash};
}

void PoliceEffect::setMode(PoliceMode m)
{
  mode = m;
}

PoliceMode PoliceEffect::getMode() const
{
  return mode;
}

void PoliceEffect::update(LEDSegment *segment)
{
  if (!active)
    return;

  unsigned long currentTime = millis();

  // First update - initialize time
  if (lastUpdateTime == 0)
  {
    lastUpdateTime = currentTime;
    return;
  }

  // Calculate the elapsed time in seconds
  float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
  lastUpdateTime = currentTime;

  // Determine the speed based on the current mode
  float cycleDuration = (mode == PoliceMode::FAST) ? fastSpeed : slowSpeed;

  if (mode == PoliceMode::SLOW)
  {
    // In slow mode, simple toggle between colors
    cycleProgress += deltaTime / cycleDuration;
    if (cycleProgress >= 1.0f)
    {
      cycleProgress = 0.0f;
    }
  }
  else if (mode == PoliceMode::FAST)
  {
    // Update the flash progress (controls the on/off of each flash)
    // Each flash consists of an on state and an off state (complete cycle)
    flashProgress += deltaTime / (cycleDuration / fastModeFlashesPerCycle);
    if (flashProgress >= 1.0f)
    {
      flashProgress = 0.0f;

      // Move to the next flash
      currentFlash++;

      // After flashesPerCycle flashes, switch sides
      if (currentFlash >= fastModeFlashesPerCycle)
      {
        currentFlash = 0;
        cycleProgress = (cycleProgress < 0.5f) ? 0.5f : 0.0f;
      }
    }
  }
}

void PoliceEffect::render(LEDSegment *segment, Color *buffer)
{
  if (!active)
    return;

  uint16_t numLEDs = segment->getNumLEDs();
  uint16_t half = numLEDs / 2;

  if (mode == PoliceMode::SLOW)
  {
    // In SLOW mode: alternate between colors on each side with no fade
    bool isRed = cycleProgress < 0.5f;

    for (uint16_t i = 0; i < numLEDs; i++)
    {
      if (i < half) // Left side
      {
        buffer[i] = isRed ? redColor : Color();
      }
      else // Right side
      {
        buffer[i] = isRed ? Color() : blueColor;
      }
    }
  }
  else // FAST mode
  {
    // In FAST mode: flash one side for fastModeFlashesPerCycle times, then the other side
    bool isRedCycle = cycleProgress < 0.5f;

    // Flash intensity based on flash progress
    // 0-0.5: On, 0.5-1.0: Off
    float flashIntensity = (flashProgress < 0.5f) ? 1.0f : 0.0f;

    for (uint16_t i = 0; i < numLEDs; i++)
    {
      if (isRedCycle)
      {
        // Red on left side
        if (i < half)
        {
          buffer[i] = redColor * flashIntensity;
        }
        else
        {
          buffer[i] = Color(0, 0, 0); // Right side off during red cycle
        }
      }
      else
      {
        // Blue on right side
        if (i >= half)
        {
          buffer[i] = blueColor * flashIntensity;
        }
        else
        {
          buffer[i] = Color(0, 0, 0); // Left side off during blue cycle
        }
      }
    }
  }
}

void PoliceEffect::onDisable()
{
  active = false;
}