#pragma once

#include "../Effects.h"
#include <stdint.h>

class BrakeLightEffect : public LEDEffect
{
public:
  // Constructs a brake light effect for a given number of LEDs.
  BrakeLightEffect(uint8_t priority = 0, bool transparent = false);
  virtual void update(LEDSegment *segment) override;
  virtual void render(LEDSegment *segment, Color *buffer) override;
  virtual void onDisable() override;

  // Set whether the brakes are active.
  void setActive(bool active);
  bool isActive() const;
  void setIsReversing(bool reversing);
  bool getIsReversing() const;

private:
  unsigned long lastUpdate;

  bool brakeActive;
  bool isReversing;

  float fadeProgress;
  float baseBrightness;
  float fadeDuration;
};
