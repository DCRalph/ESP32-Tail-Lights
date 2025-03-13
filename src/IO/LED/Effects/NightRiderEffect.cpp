#include "NightRiderEffect.h"
#include <cmath>

NightRiderEffect::NightRiderEffect(LEDManager *_ledManager,
                                   uint8_t priority, bool transparent)
    : LEDEffect(_ledManager, priority, transparent),
      active(false),
      cycleTime(5.0f),
      tailLength(15.0f),
      currentPos(0.0f),
      forward(true),
      lastUpdateTime(0)
{
}

void NightRiderEffect::setActive(bool _active)
{
  if (active == _active)
    return;

  active = _active;

  if (active)
  {
    // Reset the position and direction.
    currentPos = 0.0f;
    forward = true;
    lastUpdateTime = millis();
  }
  else
  {
  }
}

bool NightRiderEffect::isActive() const
{
  return active;
}

void NightRiderEffect::update() {
  if (!active) return;

  unsigned long currentTime = millis();
  if (lastUpdateTime == 0) {
    lastUpdateTime = currentTime;
    return;
  }

  // Calculate elapsed time in seconds.
  unsigned long dtMillis = currentTime - lastUpdateTime;
  float dtSeconds = dtMillis / 1000.0f;
  lastUpdateTime = currentTime;

  uint16_t numLEDs = ledManager->getNumLEDs();
  if (numLEDs < 2 || cycleTime <= 0.0f) return;

  // Compute the total distance for a full cycle (there and back).
  // The head goes from index 0 to index (numLEDs-1) and then back to 0.
  float fullCycleDistance = 2.0f * (numLEDs - 1);
  // Calculate speed (in LED positions per second).
  float positionPerSecond = fullCycleDistance / cycleTime;
  // Calculate step size.
  float step = positionPerSecond * dtSeconds;

  // Update position based on direction.
  if (forward) {
    currentPos += step;
    if (currentPos >= numLEDs - 1) {
      // Overshoot correction and change direction.
      currentPos = (numLEDs - 1) - (currentPos - (numLEDs - 1));
      forward = false;
    }
  } else {
    currentPos -= step;
    if (currentPos <= 0) {
      // Overshoot correction and change direction.
      currentPos = -currentPos;
      forward = true;
    }
  }
}

void NightRiderEffect::render(std::vector<Color>& buffer) {
  if (!active) return;

  uint16_t numLEDs = ledManager->getNumLEDs();
  // Clear the buffer (turn off all LEDs).
  for (uint16_t i = 0; i < numLEDs; i++) {
    buffer[i] = Color(0, 0, 0);
  }

  // Define the head color: bright red.
  const float headBrightness = 1.0f;
  const uint8_t red = 255;
  const uint8_t green = 0;
  const uint8_t blue = 0;

  // Draw the head (main bright LED).
  int headIndex = static_cast<int>(roundf(currentPos));
  if (headIndex >= 0 && headIndex < static_cast<int>(numLEDs)) {
    buffer[headIndex] = Color(red, green, blue);
  }

  // Draw the tail with a fading brightness.
  for (int i = 0; i < static_cast<int>(numLEDs); i++) {
    // Skip the head LED.
    if (i == headIndex) continue;
    // Compute the distance from the head.
    float distance = fabs(i - currentPos);
    if (distance <= tailLength) {
      // Brightness diminishes linearly with distance.
      float brightness = headBrightness * (1.0f - (distance / tailLength));
      uint8_t tailRed = static_cast<uint8_t>(red * brightness);
      uint8_t tailGreen = static_cast<uint8_t>(green * brightness);
      uint8_t tailBlue = static_cast<uint8_t>(blue * brightness);

      // Set the LED if the computed brightness is greater than its current value.
      if (tailRed > buffer[i].r || tailGreen > buffer[i].g ||
          tailBlue > buffer[i].b) {
        buffer[i] = Color(tailRed, tailGreen, tailBlue);
      }
    }
  }
}