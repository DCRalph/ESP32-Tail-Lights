#pragma once

#include "../Effects.h"
#include <stdint.h>

class LowBeamEffect : public LEDEffect
{
public:
  // Constructs a low beam light effect
  LowBeamEffect(LEDManager *_ledManager, uint8_t priority = 0,
                bool transparent = false);
  virtual void update() override;
  virtual void render(std::vector<Color> &buffer) override;

  // Set whether the low beam is active
  void setActive(bool active);
  bool isActive() const;

private:
  unsigned long lastUpdate;
  bool beamActive;
  float intensity;
  float maxIntensity;
  float rampDuration; // Time to reach full brightness in seconds
};