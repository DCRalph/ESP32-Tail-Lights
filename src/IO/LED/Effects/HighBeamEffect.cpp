#include "HighBeamEffect.h"
#include <Arduino.h>
#include <cmath>

// Ease in function for smooth lighting transitions
static inline float easeIn(float t)
{
  return t * t;
}

HighBeamEffect::HighBeamEffect(uint8_t priority,
                               bool transparent)
    : LEDEffect(priority, transparent),
      lastUpdate(0),
      beamActive(false),
      intensity(0.0f),
      maxIntensity(1.0f),
      rampDuration(0.3f) // Ramp up time in seconds
{
}

void HighBeamEffect::setActive(bool active)
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
    // Reset current intensity when turning off
    intensity = 0.0f;
  }
}

bool HighBeamEffect::isActive() const
{
  return beamActive;
}

void HighBeamEffect::update(LEDStrip *strip)
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
    intensity -= dtSeconds / (rampDuration * 0.5f); // Faster fade-out
    if (intensity < 0.0f)
    {
      intensity = 0.0f;
    }
  }
}

void HighBeamEffect::render(LEDStrip *strip, std::vector<Color> &buffer)
{
  if (!beamActive && intensity <= 0.0f)
  {
    return;
  }

  uint16_t numLEDs = strip->getNumLEDs();

  // Apply easing for smoother transition
  float smoothIntensity = easeIn(intensity);

  // Color values - bright white with slight blue tint for high beams
  const uint8_t baseRed = 255;
  const uint8_t baseGreen = 255;
  const uint8_t baseBlue = 255;

  // High beam fills the entire LED strip with bright white light
  for (uint16_t i = 0; i < numLEDs; i++)
  {
    buffer[i].r = static_cast<uint8_t>(baseRed * smoothIntensity);
    buffer[i].g = static_cast<uint8_t>(baseGreen * smoothIntensity);
    buffer[i].b = static_cast<uint8_t>(baseBlue * smoothIntensity);
  }
}