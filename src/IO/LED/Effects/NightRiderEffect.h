#pragma once

#include "../Effects.h"
#include <vector>

class NightRiderEffect : public LEDEffect
{
public:
  // Constructs the Night Rider effect.
  // speed: movement in LED positions per second.
  // tailLength: tail length in LED units (should be > 0).
  NightRiderEffect(LEDManager *_ledManager, uint8_t priority = 0,
                   bool transparent = false);

  virtual void update() override;
  virtual void render(std::vector<Color> &buffer) override;

  // Activate or disable the effect.
  void setActive(bool active);
  bool isActive() const;

  // Customizable parameters:
  float cycleTime;
  float tailLength; // Length of the fading tail in LED units.

private:
  bool active;
  // The current position (can be fractional for smooth movement).
  float currentPos;
  // Direction of movement: true = moving forward, false = moving backward.
  bool forward;
  // Last update time (in milliseconds).
  unsigned long lastUpdateTime;
};
