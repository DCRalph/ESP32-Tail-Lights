#include "PulseWaveEffect.h"
#include <cmath>
#include "../LEDStrip.h"

PulseWaveEffect::PulseWaveEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      active(false),
      lastUpdateTime(0),
      phase(0.0f),
      colorPhase(0.0f),
      baseHue(140.0f),        // Start with a blue-green base
      hueRange(120.0f),       // Cover 120 degrees of the color wheel
      waveSpeed(0.5f),        // Half a cycle per second
      pulseFrequency(2.0f),   // Two wave peaks across the strip
      colorCycleSpeed(20.0f), // 20 degrees per second (full color cycle in 18 seconds)
      colorSaturation(1.0f),  // Full saturation
      intensity(1.0f)         // Full brightness
{
}

void PulseWaveEffect::setActive(bool _active)
{
  active = _active;
  if (active && lastUpdateTime == 0)
  {
    lastUpdateTime = millis();
  }
}

bool PulseWaveEffect::isActive() const
{
  return active;
}

float PulseWaveEffect::computeWave(float pos, float phase, float frequency)
{
  // Creates a smooth sinusoidal wave that varies from 0 to 1
  // pos: position along the strip [0, 1]
  // phase: current animation phase [0, 2π]
  // frequency: number of wave cycles across the strip

  // Calculate the wave pattern
  // Use negative phase to make waves move away from index 0 (or center in mirrored mode)
  float value = 0.5f * (1.0f + sinf(2.0f * PI * (frequency * pos - phase)));

  // Add a gentle fade toward the edges for a more natural look
  float edgeFade = 0.7f + 0.3f * (1.0f - powf(2.0f * (pos - 0.5f), 2));

  return value * edgeFade;
}

void PulseWaveEffect::update(LEDStrip *strip)
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

  // Update the animation phase
  // Positive phase increment makes waves appear to move away from index 0
  phase += waveSpeed * dtSeconds;
  if (phase > 1.0f)
  {
    phase -= 1.0f; // Keep phase in [0, 1) range
  }

  // Update the color cycle phase
  colorPhase += colorCycleSpeed * dtSeconds;
  while (colorPhase >= 360.0f)
  {
    colorPhase -= 360.0f; // Keep in [0, 360) range
  }
}

void PulseWaveEffect::render(LEDStrip *strip, Color *buffer)
{
  if (!active)
    return;

  uint16_t numLEDs = strip->getNumLEDs();
  bool isMirrored = false;

  if (strip->getType() == LEDStripType::TAILLIGHT)
  {
    isMirrored = true;
  }

  uint16_t midPoint = numLEDs / 2;

  for (uint16_t i = 0; i < numLEDs; i++)
  {
    float normalizedPos;

    if (isMirrored)
    {
      // Calculate position based on distance from center (0 at center, 1 at edges)
      normalizedPos = static_cast<float>(abs(static_cast<int>(i) - static_cast<int>(midPoint))) / midPoint;
    }
    else
    {
      // Normalized position [0, 1] along the strip
      normalizedPos = static_cast<float>(i) / (numLEDs - 1);
    }

    // Compute wave value at this position
    float waveVal = computeWave(normalizedPos, phase, pulseFrequency);

    // Calculate the hue based on position and current color phase
    float posHue = baseHue + colorPhase + (normalizedPos * hueRange);
    while (posHue >= 360.0f)
    {
      posHue -= 360.0f;
    }

    // Compute color with the calculated parameters
    Color color = Color::hsv2rgb(
        posHue,             // Hue varies by position and time
        colorSaturation,    // Full saturation
        waveVal * intensity // Brightness varies with the wave
    );

    // Apply to buffer
    buffer[i] = color;
  }
}

void PulseWaveEffect::onDisable()
{
  active = false;
}