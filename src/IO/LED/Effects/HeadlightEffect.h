#pragma once

#include "../Effects.h"
#include <stdint.h>

class HeadlightEffect : public LEDEffect
{
public:
  // Constructs a high beam light effect
  HeadlightEffect(uint8_t priority = 0,
                  bool transparent = false);
  virtual void update(LEDStrip *strip) override;
  virtual void render(LEDStrip *strip, std::vector<Color> &buffer) override;

  // Set whether the high beam is active
  void setActive(bool active);
  bool isActive() const;

  // Set/get whether the headlights are split
  void setSplit(bool split);
  bool getSplit() const;

  // Set/get the headlight color
  void setColor(bool r, bool g, bool b);
  void getColor(bool &r, bool &g, bool &b) const;

private:
  unsigned long lastUpdate;
  bool headlightActive;
  bool split;
  int ledPerSide; // number of leds per side when in split mode

  // colored headlight. if all false default to white.
  bool red;
  bool blue;
  bool green;

  // turn on/off fade variables
  float intensity;
  float maxIntensity;
  float rampDuration; // Time to reach full brightness in seconds

  // Animation variables
  float progress;       // Animation progress from 0.0 to 1.0
  float animationSpeed; // Time in seconds for a full animation cycle

  // Rainbow mode variables
  float baseHueCenter; // Base hue at center (0-360)
  float baseHueEdge;   // Base hue at edges (0-360)
  float hueCenter;     // Current hue at center (0-360)
  float hueEdge;       // Current hue at edges (0-360)
  float hueOffset;     // Accumulated hue offset for animation
  float rainbowSpeed;  // Speed of color cycling in degrees per second
};