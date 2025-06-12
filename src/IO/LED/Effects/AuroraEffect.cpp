#include "AuroraEffect.h"
#include <cmath>
#include <Arduino.h>
#include "../LEDStrip.h"

AuroraEffect::AuroraEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      active(false),
      lastUpdateTime(0),
      time(0.0f),
      movementSpeed(0.2f),
      waveIntensity(0.6f),
      intensity(0.85f),
      minHue(140.0f),
      maxHue(270.0f),
      saturationMin(0.7f),
      saturationMax(1.0f)
{

  phaseOffsets[0] = 0.0f;
  amplitudes[0] = 1.0f;
  frequencies[0] = 1.5f;
  hues[0] = 160.0f; // Green-blue

  // Second wave - slower, wider movements
  phaseOffsets[1] = 2.1f;
  amplitudes[1] = 0.7f;
  frequencies[1] = 0.8f;
  hues[1] = 200.0f; // Blue

  // Third wave - fast, subtle detail
  phaseOffsets[2] = 4.2f;
  amplitudes[2] = 0.4f;
  frequencies[2] = 3.0f;
  hues[2] = 240.0f; // Purple-blue
}

void AuroraEffect::setActive(bool _active)
{
  active = _active;
  if (active && lastUpdateTime == 0)
  {
    lastUpdateTime = millis();

    // Randomize phase offsets when activating for variety
    for (int i = 0; i < NUM_WAVES; i++)
    {
      phaseOffsets[i] = random(0, 1000) / 100.0f;
    }
  }
}

bool AuroraEffect::isActive() const
{
  return active;
}

float AuroraEffect::computeWave(float pos, float time, int waveIndex)
{
  // Different wave patterns for each component to create organic movement
  switch (waveIndex)
  {
  case 0:
    // Main sine wave
    return amplitudes[waveIndex] * 0.5f * (1.0f + sinf(2.0f * PI * (frequencies[waveIndex] * pos + time + phaseOffsets[waveIndex])));

  case 1:
    // Secondary wave with more complex shape (composite of two sine waves)
    return amplitudes[waveIndex] * 0.5f * (sinf(2.0f * PI * (frequencies[waveIndex] * pos + time * 0.7f + phaseOffsets[waveIndex])) + 0.3f * sinf(2.0f * PI * (frequencies[waveIndex] * 3 * pos + time * 1.3f)));

  case 2:
    // Fast subtle details using a different function
    return amplitudes[waveIndex] * 0.5f * (1.0f + sinf(2.0f * PI * (frequencies[waveIndex] * pos + time * 1.5f + phaseOffsets[waveIndex] + pos * 3.0f)));

  default:
    return 0.0f;
  }
}

void AuroraEffect::update(LEDStrip *strip)
{
  if (!active)
    return;

  unsigned long currentTime = millis();
  if (lastUpdateTime == 0)
  {
    lastUpdateTime = currentTime;
    return;
  }

  // Calculate elapsed time in seconds
  unsigned long dtMillis = currentTime - lastUpdateTime;
  float dtSeconds = dtMillis / 1000.0f;
  lastUpdateTime = currentTime;

  // Update animation time
  time += movementSpeed * dtSeconds;

  // Periodically adjust the hues slightly to create color variation over time
  if (fmod(time, 10.0f) < 0.1f) // Every ~10 seconds
  {
    for (int i = 0; i < NUM_WAVES; i++)
    {
      // Slowly drift the hues within their color families
      hues[i] += (random(-10, 10) / 10.0f);

      // Keep hues within the desired range
      if (hues[i] < minHue)
        hues[i] = minHue;
      if (hues[i] > maxHue)
        hues[i] = maxHue;
    }
  }
}

void AuroraEffect::render(LEDStrip *strip, Color *buffer)
{
  if (!active)
    return;

  uint16_t numLEDs = strip->getNumLEDs();
  bool isHeadlight = (strip->getType() == LEDStripType::HEADLIGHT);
  uint16_t midPoint = numLEDs / 2;

  // For headlights, we'll only process the first half and mirror it
  uint16_t ledsToProcess = isHeadlight ? midPoint + (numLEDs % 2) : numLEDs;

  for (uint16_t i = 0; i < ledsToProcess; i++)
  {
    // Calculate normalized position [0, 1] along the strip
    float pos;

    if (isHeadlight)
    {
      // For headlights, calculate distance from center (0 at center, 1 at edge)
      pos = static_cast<float>(abs(midPoint - i)) / midPoint;
    }
    else
    {
      // For other strips, maintain original behavior
      pos = static_cast<float>(i) / (numLEDs - 1);
    }

    // Calculate the combined influence of the different wave components
    float totalWave = 0.0f;
    float maxPossibleValue = 0.0f;

    for (int w = 0; w < NUM_WAVES; w++)
    {
      totalWave += computeWave(pos, time, w);
      maxPossibleValue += amplitudes[w];
    }

    // Normalize the combined wave value
    float waveValue = totalWave / maxPossibleValue;

    // Apply the wave intensity parameter
    waveValue = waveValue * waveIntensity + (1.0f - waveIntensity) * 0.5f;

    // Determine which color component is most dominant at this position
    int dominantWave = 0;
    float maxWaveValue = 0.0f;

    for (int w = 0; w < NUM_WAVES; w++)
    {
      float value = computeWave(pos, time, w);
      if (value > maxWaveValue)
      {
        maxWaveValue = value;
        dominantWave = w;
      }
    }

    // Get the hue from the dominant wave
    float hue = hues[dominantWave];

    // Vary saturation based on wave intensity
    float saturation = saturationMin + (saturationMax - saturationMin) * waveValue;

    // Vary brightness based on wave value (brighter at peaks)
    float brightness = waveValue * intensity;

    // Create the color
    Color color = Color::hsv2rgb(hue, saturation, brightness);

    // Apply to buffer
    buffer[i] = color;

    // For headlights, mirror to the other side
    if (isHeadlight && i != midPoint)
    { // Skip mirroring the center LED
      uint16_t mirrorIndex = numLEDs - i - 1;
      buffer[mirrorIndex] = color;
    }
  }
}

void AuroraEffect::onDisable()
{
  active = false;
}