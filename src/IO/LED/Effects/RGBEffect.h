#pragma once

#include "../Effects.h"
#include <stdint.h>
#include <vector>

class RGBEffect : public LEDEffect
{
public:
  // Constructs the RGB (rainbow) effect. The effect will map the hue from the center to the edges.
  RGBEffect(LEDManager *_ledManager, uint8_t priority = 0, bool transparent = false);

  virtual void update() override;
  virtual void render(std::vector<Color> &buffer) override;

  // Activate or disable the effect.
  void setActive(bool active);
  bool isActive() const;

  // Customizable parameters:
  // Base hue values defining the range, in degrees [0,360).
  float baseHueCenter; // Base hue at the center
  float baseHueEdge;   // Base hue at the edges

  // Speed at which the hue range moves (degrees per second)
  float speed;

private:
  bool active;

  // Currently computed hues (animated).
  float hueCenter;
  float hueEdge;
  // Tracks the cumulative hue offset (in degrees).
  float hueOffset;

  // Last update time for animation calculation.
  unsigned long lastUpdateTime;
};
