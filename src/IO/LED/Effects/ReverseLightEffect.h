#pragma once

#include "../Effects.h"
#include <stdint.h>

class ReverseLightEffect : public LEDEffect
{
public:
  // Constructs the reverse light effect.
  ReverseLightEffect(uint8_t priority = 0,
                     bool transparent = false);

  virtual void update(LEDStrip *strip) override;
  virtual void render(LEDStrip *strip, Color *buffer) override;

  // Activate or disable the effect.
  void setActive(bool active);
  bool isActive() const;

  bool isAnimating() const;

private:
  bool active;
  uint64_t startTime;
  float animationSpeed; // in seconds for a full cycle (expand then contract)
  float progress;       // from 0 to 1
};
