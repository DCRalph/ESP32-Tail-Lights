#pragma once

#include "../Effects.h"
#include <stdint.h>

// Fancy, blink-based indicator effect that fades in from the center edge.
// The indicator region is defined as a quarter of the LED strip.
// The left indicator’s inner edge is at index (regionLength-1) and the right indicator’s
// inner edge is at index (numLEDs - regionLength).
class IndicatorEffect : public LEDEffect
{
public:
  enum Side
  {
    LEFT = 0,
    RIGHT = 1
  };

  // Constructs an indicator effect for a given LED strip.
  // All timing and color parameters are customizable.
  IndicatorEffect(LEDManager *_ledManager, Side side, uint8_t priority = 1,
                  bool transparent = false);

  virtual void update() override;
  virtual void render(std::vector<Color> &buffer) override;

  void setOtherIndicator(IndicatorEffect *otherIndicator);
  IndicatorEffect *getOtherIndicator() const;

  // Activate or deactivate the indicator.
  void setActive(bool active);
  bool isActive() const;

  void syncWithOtherIndicator();

  // Customizable parameters:
  unsigned long blinkCycle; // Total blink period in ms (default: 1000)
  unsigned long fadeInTime; // Duration of fade in (ms) during the on phase (default: 300)
  // Base color for the indicator (default: amber/yellow).
  uint8_t baseR;
  uint8_t baseG;
  uint8_t baseB;

private:
  Side side;
  bool indicatorActive;
  bool bigIndicator; // Whether the indicator is the big mode

  IndicatorEffect *otherIndicator; // Pointer to the other indicator effect.
  bool synced;                     // Whether the indicator has been synced with the other indicator.

  uint64_t activatedTime; // Time when the indicator was last activated.

  // Computed fade factor (0.0 to 1.0) for the current blink cycle.
  // It increases linearly during the fade in period and then resets.
  float fadeFactor;
};
