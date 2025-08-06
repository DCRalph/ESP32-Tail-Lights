#include "ServiceLightsEffect.h"
#include <Arduino.h>
#include <math.h>

// Helper function: Easing function for smoother transitions
static float easeInOut(float t)
{
  return 0.5f * (1.0f - cosf(t * PI));
}

ServiceLightsEffect::ServiceLightsEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      active(false),
      mode(ServiceLightsMode::FAST),
      lastUpdateTime(0),
      flashProgress(0.0f),
      cycleProgress(0.0f),
      fastSpeed(0.5f),            // 0.5 seconds per full flash cycle in fast mode
      slowSpeed(1.0f),            // 1.0 seconds per full flash cycle in slow mode
      fastModeFlashesPerCycle(3), // 3 flashes per color cycle
      color(Color::ORANGE),
      currentFlash(0)
{
}

void ServiceLightsEffect::setActive(bool a)
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

bool ServiceLightsEffect::isActive() const
{
  return active;
}

void ServiceLightsEffect::setSyncData(ServiceLightsSyncData syncData)
{
  active = syncData.active;
  mode = syncData.mode;
  flashProgress = syncData.progress;
  cycleProgress = syncData.cycleProgress;
  currentFlash = syncData.currentFlash;
  fastSpeed = syncData.fastSpeed;
  slowSpeed = syncData.slowSpeed;
  fastModeFlashesPerCycle = syncData.fastModeFlashesPerCycle;
  color = Color(syncData.colorR, syncData.colorG, syncData.colorB);
}

ServiceLightsSyncData ServiceLightsEffect::getSyncData()
{
  return ServiceLightsSyncData{
      .active = active,
      .mode = mode,
      .progress = flashProgress,
      .cycleProgress = cycleProgress,
      .currentFlash = currentFlash,
      .fastSpeed = fastSpeed,
      .slowSpeed = slowSpeed,
      .fastModeFlashesPerCycle = fastModeFlashesPerCycle,
      .colorR = color.r,
      .colorG = color.g,
      .colorB = color.b};
}

void ServiceLightsEffect::setMode(ServiceLightsMode m)
{
  mode = m;
}

ServiceLightsMode ServiceLightsEffect::getMode() const
{
  return mode;
}

void ServiceLightsEffect::setColor(Color c)
{
  color = c;
}

Color ServiceLightsEffect::getColor() const
{
  return color;
}

void ServiceLightsEffect::setFastSpeed(float speed)
{
  fastSpeed = speed;
}

float ServiceLightsEffect::getFastSpeed() const
{
  return fastSpeed;
}

void ServiceLightsEffect::setSlowSpeed(float speed)
{
  slowSpeed = speed;
}

float ServiceLightsEffect::getSlowSpeed() const
{
  return slowSpeed;
}

void ServiceLightsEffect::setFastModeFlashesPerCycle(uint16_t flashes)
{
  fastModeFlashesPerCycle = flashes;
}

uint16_t ServiceLightsEffect::getFastModeFlashesPerCycle() const
{
  return fastModeFlashesPerCycle;
}

void ServiceLightsEffect::update(LEDSegment *segment)
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

  if (mode == ServiceLightsMode::SLOW)
  {
    updateSlowMode(segment, deltaTime);
  }
  else if (mode == ServiceLightsMode::FAST)
  {
    updateFastMode(segment, deltaTime);
  }
  else if (mode == ServiceLightsMode::ALTERNATE)
  {
    updateAlternateMode(segment, deltaTime);
  }
  else if (mode == ServiceLightsMode::STROBE)
  {
    updateStrobeMode(segment, deltaTime);
  }
  else if (mode == ServiceLightsMode::SCROLL)
  {
    updateScrollMode(segment, deltaTime);
  }
}

void ServiceLightsEffect::render(LEDSegment *segment, Color *buffer)
{
  if (!active)
    return;

  if (mode == ServiceLightsMode::SLOW)
  {
    renderSlowMode(segment, buffer);
  }
  else if (mode == ServiceLightsMode::FAST)
  {
    renderFastMode(segment, buffer);
  }
  else if (mode == ServiceLightsMode::ALTERNATE)
  {
    renderAlternateMode(segment, buffer);
  }
  else if (mode == ServiceLightsMode::STROBE)
  {
    renderStrobeMode(segment, buffer);
  }
  else if (mode == ServiceLightsMode::SCROLL)
  {
    renderScrollMode(segment, buffer);
  }
}

void ServiceLightsEffect::onDisable()
{
  active = false;
}

// main logic for each mode
// ##############################################################

void ServiceLightsEffect::updateSlowMode(LEDSegment *segment, float deltaTime)
{
  // In slow mode, simple toggle between colors
  cycleProgress += deltaTime / slowSpeed;
  if (cycleProgress >= 1.0f)
  {
    cycleProgress = 0.0f;
  }
}

void ServiceLightsEffect::updateFastMode(LEDSegment *segment, float deltaTime)
{
  // Update the flash progress (controls the on/off of each flash)
  // Each flash consists of an on state and an off state (complete cycle)
  flashProgress += deltaTime / (fastSpeed / fastModeFlashesPerCycle);
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

void ServiceLightsEffect::updateAlternateMode(LEDSegment *segment, float deltaTime)
{
  // In alternate mode, use slowSpeed to control alternating between sides
  cycleProgress += deltaTime / slowSpeed;
  if (cycleProgress >= 1.0f)
  {
    cycleProgress = 0.0f;
  }
}

void ServiceLightsEffect::updateStrobeMode(LEDSegment *segment, float deltaTime)
{
  // Update flash progress for strobing effect (faster strobing)
  flashProgress += deltaTime / 0.1f; // 0.1 second strobe cycle (fast strobe)
  if (flashProgress >= 1.0f)
  {
    flashProgress = 0.0f;
  }

  // Update cycle progress for color switching (every 1 second)
  cycleProgress += deltaTime / 1.0f; // 1 second per color switch
  if (cycleProgress >= 1.0f)
  {
    cycleProgress = 0.0f;
  }
}

void ServiceLightsEffect::updateScrollMode(LEDSegment *segment, float deltaTime)
{
  // In scroll mode, cycle progress controls the scroll position
  // Use slowSpeed to control scroll speed
  cycleProgress += deltaTime / slowSpeed;
  if (cycleProgress >= 1.0f)
  {
    cycleProgress = 0.0f; // Wrap around
  }
}

void ServiceLightsEffect::renderSlowMode(LEDSegment *segment, Color *buffer)
{
  uint16_t numLEDs = segment->getNumLEDs();
  uint16_t half = numLEDs / 2;

  // In SLOW mode: alternate between colors on each side with no fade
  bool isRed = cycleProgress < 0.5f;

  for (uint16_t i = 0; i < numLEDs; i++)
  {
    if (i < half) // Left side
    {
      buffer[i] = isRed ? color : Color();
    }
    else // Right side
    {
      buffer[i] = isRed ? Color() : color;
    }
  }
}

void ServiceLightsEffect::renderFastMode(LEDSegment *segment, Color *buffer)
{
  uint16_t numLEDs = segment->getNumLEDs();
  uint16_t half = numLEDs / 2;

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
        buffer[i] = color * flashIntensity;
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
        buffer[i] = color * flashIntensity;
      }
      else
      {
        buffer[i] = Color(0, 0, 0); // Left side off during blue cycle
      }
    }
  }
}

void ServiceLightsEffect::renderAlternateMode(LEDSegment *segment, Color *buffer)
{
  uint16_t numLEDs = segment->getNumLEDs();
  uint16_t half = numLEDs / 2;

  // In ALTERNATE mode: one side color val, other side white, alternating based on slowSpeed
  bool isColorOnLeft = cycleProgress < 0.5f;

  for (uint16_t i = 0; i < numLEDs; i++)
  {
    if (i < half) // Left side
    {
      buffer[i] = isColorOnLeft ? color : Color::WHITE;
    }
    else // Right side
    {
      buffer[i] = isColorOnLeft ? Color::WHITE : color;
    }
  }
}

void ServiceLightsEffect::renderStrobeMode(LEDSegment *segment, Color *buffer)
{
  uint16_t numLEDs = segment->getNumLEDs();

  // In STROBE mode: strobe between color val and white, swap every 1 second
  bool useColorVal = cycleProgress < 0.5f;
  Color currentColor = useColorVal ? color : Color::WHITE;

  // Strobe intensity based on flash progress
  // 0-0.5: On, 0.5-1.0: Off
  float strobeIntensity = (flashProgress < 0.5f) ? 1.0f : 0.0f;

  for (uint16_t i = 0; i < numLEDs; i++)
  {
    buffer[i] = currentColor * strobeIntensity;
  }
}

void ServiceLightsEffect::renderScrollMode(LEDSegment *segment, Color *buffer)
{
  uint16_t numLEDs = segment->getNumLEDs();
  uint16_t half = numLEDs / 2;

  bool isMirrored = false;

  if (segment->getParentStrip()->getType() == LEDStripType::HEADLIGHT ||
      segment->getParentStrip()->getType() == LEDStripType::UNDERGLOW)
  {
    isMirrored = true;
  }

  if (isMirrored)
  {
    half = numLEDs - half;
  }

  // In SCROLL mode: scroll half color val, half white from left to right like lighthouse
  // cycleProgress (0-1) determines the scroll position
  int scrollOffset = (int)(cycleProgress * numLEDs);

  for (uint16_t i = 0; i < numLEDs; i++)
  {
    int patternPos;
    uint16_t patternSize;

    if (isMirrored)
    {
      // For mirrored mode: start from middle and move inwards (reversed direction)
      uint16_t center = numLEDs / 2;
      uint16_t distanceFromCenter;

      if (i < center)
      {
        // Left side: distance increases as we go left from center
        distanceFromCenter = center - i - 1;
      }
      else
      {
        // Right side: distance increases as we go right from center
        distanceFromCenter = i - center;
      }

      // Reverse the direction by subtracting from max distance
      uint16_t maxDistance = center;
      distanceFromCenter = maxDistance - distanceFromCenter - 1;

      // Apply scroll offset to the reversed distance from center
      patternPos = (distanceFromCenter + scrollOffset) % center;
      patternSize = center / 2; // Quarter of total strip (half of each half)
    }
    else
    {
      // Normal mode: scroll from left to right
      patternPos = (i + scrollOffset) % numLEDs;
      patternSize = half; // Half of total strip
    }

    // Color section takes up quarter (or half for normal mode)
    if (patternPos < patternSize)
    {
      buffer[i] = color;
    }
    else
    {
      buffer[i] = Color::WHITE;
    }
  }
}
