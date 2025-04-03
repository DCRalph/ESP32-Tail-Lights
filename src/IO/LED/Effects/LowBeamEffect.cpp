#include "LowBeamEffect.h"
#include <Arduino.h>
#include <cmath>

// Ease in function for smooth lighting transitions
static inline float easeIn(float t)
{
  return t * t;
}

LowBeamEffect::LowBeamEffect(LEDManager *_ledManager, uint8_t priority,
                             bool transparent)
    : LEDEffect(_ledManager, priority, transparent),
      lastUpdate(0),
      beamActive(false),
      intensity(0.0f),
      maxIntensity(0.7f), // Lower max intensity compared to high beam
      rampDuration(0.4f)  // Slightly slower ramp up time
{
}

void LowBeamEffect::setActive(bool active)
{
  // Only update if state is changing
  if (active == beamActive)
  {
    return;
  }

  beamActive = active;

  // Start with current intensity to allow smooth transitions
  if (!beamActive)
  {
    // Reset intensity when turning off
    intensity = 0.0f;
  }
}

bool LowBeamEffect::isActive() const
{
  return beamActive;
}

void LowBeamEffect::update()
{
  unsigned long currentTime = millis();
  if (lastUpdate == 0)
  {
    lastUpdate = currentTime;
    return;
  }

  // Compute elapsed time in seconds
  unsigned long dtMillis = currentTime - lastUpdate;
  float dtSeconds = dtMillis / 1000.0f;
  lastUpdate = currentTime;

  // Update intensity based on active state
  if (beamActive)
  {
    // Increase intensity to maximum
    intensity += dtSeconds / rampDuration;
    if (intensity > maxIntensity)
    {
      intensity = maxIntensity;
    }
  }
  else
  {
    // Decrease intensity to zero
    intensity -= dtSeconds / (rampDuration * 0.7f);
    if (intensity < 0.0f)
    {
      intensity = 0.0f;
    }
  }
}

void LowBeamEffect::render(std::vector<Color> &buffer)
{
  if (!beamActive && intensity <= 0.0f)
  {
    return;
  }

  uint16_t numLEDs = ledManager->getNumLEDs();

  // Apply easing for smoother transition
  float smoothIntensity = easeIn(intensity);

  // Color values - warm white for low beams
  // Lower blue component for warmer color
  const uint8_t baseRed = 255;
  const uint8_t baseGreen = 240;
  const uint8_t baseBlue = 200;

  // Low beam uses a more focused pattern
  uint16_t centerStart = numLEDs / 3;
  uint16_t centerEnd = numLEDs - centerStart;

  for (uint16_t i = 0; i < numLEDs; i++)
  {
    float localIntensity = smoothIntensity;

    // If outside the center range, reduce intensity for a more focused beam effect
    if (i < centerStart || i > centerEnd)
    {
      // Calculate distance from center zone (0.0 to 1.0)
      float distance;
      if (i < centerStart)
      {
        distance = static_cast<float>(centerStart - i) / centerStart;
      }
      else
      {
        distance = static_cast<float>(i - centerEnd) / (numLEDs - centerEnd);
      }

      // Apply distance-based falloff
      localIntensity *= (1.0f - (distance * 0.7f));
    }

    buffer[i].r = static_cast<uint8_t>(baseRed * localIntensity);
    buffer[i].g = static_cast<uint8_t>(baseGreen * localIntensity);
    buffer[i].b = static_cast<uint8_t>(baseBlue * localIntensity);
  }
}