#pragma once

#include "../Effects.h"
#include <stdint.h>

class HighBeamEffect : public LEDEffect
{
public:
  // Constructs a high beam light effect
  HighBeamEffect(LEDManager *_ledManager, uint8_t priority = 0,
                 bool transparent = false);
  virtual void update() override;
  virtual void render(std::vector<Color> &buffer) override;

  // Set whether the high beam is active
  void setActive(bool active);
  bool isActive() const;

private:
  unsigned long lastUpdate;
  bool beamActive;
  float intensity;
  float maxIntensity;
  float rampDuration; // Time to reach full brightness in seconds
};