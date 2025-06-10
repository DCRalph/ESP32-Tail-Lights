#include "BrakeLightEffect.h"
#include <cmath>

// Ease‑in quadratic: f(t) = t².
// (Since fadeProgress changes from 1 to 0, this curve makes the brightness drop quickly at
// the beginning; when fadeProgress reaches 0, the brightness is off.)
static inline float easeInQuadratic(float t)
{
  return t * t;
}

BrakeLightEffect::BrakeLightEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      lastUpdate(0),
      brakeActive(false),
      // When active, fadeProgress is 1. When brakes are released it counts down.
      fadeProgress(1.0f),
      // Full brightness when braking; when released this is immediately set to 0.3.
      baseBrightness(0.0f),
      // Fade-out duration in seconds (adjust as needed).
      fadeDuration(0.6f)
{
}

//
// IMPORTANT: Only update state when the brake state actually changes.
// If setActive(false) is called each frame, the fade will never progress!
//
void BrakeLightEffect::setActive(bool active)
{
  // Only update if the state is changing.
  if (active == brakeActive)
  {
    return;
  }

  brakeActive = active;

  if (brakeActive)
  {
    // When brakes are pressed: full brightness.
    baseBrightness = 1.0f;
    fadeProgress = 1.0f;
  }
  else
  {
    // When brakes are released: drop base brightness immediately to 0.3
    // and start the fade (fadeProgress counts down from 1 to 0).
    baseBrightness = 0.4f;
    fadeProgress = 1.0f;
  }
}

bool BrakeLightEffect::isActive() const
{
  return brakeActive;
}

void BrakeLightEffect::setIsReversing(bool reversing)
{
  isReversing = reversing;
}

bool BrakeLightEffect::getIsReversing() const
{
  return isReversing;
}

void BrakeLightEffect::update(LEDStrip *strip)
{
  unsigned long currentTime = millis();
  if (lastUpdate == 0)
  {
    lastUpdate = currentTime;
    return;
  }

  // Compute elapsed time in seconds.
  unsigned long dtMillis = currentTime - lastUpdate;
  float dtSeconds = dtMillis / 1000.0f;
  lastUpdate = currentTime;

  if (!brakeActive)
  {
    // Decrease fadeProgress linearly over fadeDuration seconds.
    fadeProgress -= dtSeconds / fadeDuration;
    if (fadeProgress < 0.0f)
    {
      fadeProgress = 0.0f;
    }
  }
  else
  {
    // When brakes are pressed, keep fadeProgress at full.
    fadeProgress = 1.0f;
  }
}

void BrakeLightEffect::render(LEDStrip *strip, Color *buffer)
{

  if (isReversing)
  {
    if (brakeActive)
    {
      int numLeds = strip->getNumLEDs() / 5;

      for (uint16_t i = 0; i < numLeds; i++)
      {
        buffer[i] = Color(255, 0, 0);
        buffer[strip->getNumLEDs() - i - 1] = Color(255, 0, 0);
      }
    }

    return;
  }

  if (brakeActive)
  {
    for (uint16_t i = 0; i < strip->getNumLEDs(); i++)
    {
      buffer[i] = Color(255, 0, 0);
    }

    return;
  }

  if (fadeProgress <= 0.0f)
  {
    return;
  }

  // if reversing only light up the edges

  uint16_t numLEDs = strip->getNumLEDs();
  uint16_t mid = numLEDs / 2;

  // Compute the fade factor using the ease‑in quadratic curve.
  float fadeFactor = easeInQuadratic(fadeProgress);
  // Overall brightness is the base brightness (1.0 when braking, 0.3 when released)
  // scaled by the fade factor.
  float overallBrightness = baseBrightness * fadeFactor;

  // Spatial fading: LEDs farther from the center fade faster.
  // Adjust spatialExponent to change how aggressively the fade is applied.
  constexpr float spatialExponent = 2.0f;

  for (uint16_t i = 0; i < numLEDs; i++)
  {
    // Calculate the normalized distance from the center (0 at center, 1 at edge).
    float normDist = (mid > 0)
                         ? fabs(static_cast<float>(i) - mid) / mid
                         : 0.0f;
    // The spatial factor lowers brightness for LEDs farther from the center.
    // changing the exponent will change how aggressively the fade is applied for.
    float spatialFactor = pow(1.0f - normDist, spatialExponent);

    // Final LED brightness is the product of overall brightness and spatial factor.
    float ledBrightness = overallBrightness * spatialFactor;

    uint8_t redVal = static_cast<uint8_t>(255 * ledBrightness);
    if (redVal > 10)
      buffer[i] = Color(redVal, 0, 0);
  }
}

void BrakeLightEffect::onDisable()
{
  brakeActive = false;
}