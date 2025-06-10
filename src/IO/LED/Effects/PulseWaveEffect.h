#pragma once

#include "../Effects.h"
#include <stdint.h>

class PulseWaveEffect : public LEDEffect
{
public:
  // Constructs the Pulse Wave effect
  PulseWaveEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDStrip *strip) override;
  virtual void render(LEDStrip *strip, Color *buffer) override;
  virtual void onDisable() override;

  // Activate or disable the effect
  void setActive(bool active);
  bool isActive() const;

  // Customizable parameters
  float waveSpeed;       // Speed of the wave in cycles per second
  float pulseFrequency;  // Frequency of the pulse waves
  float colorCycleSpeed; // Speed at which colors cycle in degrees per second
  float colorSaturation; // Color saturation (0.0-1.0)
  float intensity;       // Maximum brightness (0.0-1.0)

private:
  bool active;

  // Time tracking
  unsigned long lastUpdateTime;

  // Animation state
  float phase;      // Current phase of the wave animation
  float colorPhase; // Current phase of the color cycle

  // Color parameters
  float baseHue;  // Base hue for the effect (0-360)
  float hueRange; // Range of hue variation (0-360)

  // Creates smooth waves
  float computeWave(float pos, float phase, float frequency);
};