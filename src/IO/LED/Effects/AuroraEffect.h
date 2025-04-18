#pragma once

#include "../Effects.h"
#include <stdint.h>

class AuroraEffect : public LEDEffect
{
public:
  // Constructs the Aurora Borealis effect
  AuroraEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDStrip *strip) override;
  virtual void render(LEDStrip *strip, Color *buffer) override;

  // Activate or disable the effect
  void setActive(bool active);
  bool isActive() const;

  // Customizable parameters
  float movementSpeed; // Speed of movement in cycles per second
  float waveIntensity; // Intensity of the wave motion
  float intensity;     // Maximum brightness (0.0-1.0)

  // Color range control
  float minHue;        // Minimum hue value (default: green-blue range)
  float maxHue;        // Maximum hue value
  float saturationMin; // Minimum saturation
  float saturationMax; // Maximum saturation

private:
  bool active;

  // Time tracking
  unsigned long lastUpdateTime;

  // Animation state variables
  float time; // Accumulated time for animation

  // Wave phase offsets for each component
  static const int NUM_WAVES = 3;
  float phaseOffsets[NUM_WAVES];
  float amplitudes[NUM_WAVES];
  float frequencies[NUM_WAVES];
  float hues[NUM_WAVES];

  // Helper function to compute a smooth, organic-looking wave
  float computeWave(float pos, float time, int waveIndex);
};