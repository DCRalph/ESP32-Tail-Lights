#pragma once

#include "../Effects.h"
#include "../Types.h"
#include <stdint.h>

class ColorFadeEffect : public LEDEffect
{
public:
  // Constructs the ColorFade effect with default timing parameters
  ColorFadeEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDStrip *strip) override;
  virtual void render(LEDStrip *strip, Color *buffer) override;
  virtual void onDisable() override;

  // Activate or disable the effect
  void setActive(bool active);
  bool isActive() const;

  // Sync data methods for network synchronization
  void setSyncData(ColorFadeSyncData syncData);
  ColorFadeSyncData getSyncData();

  // Customizable parameters
  float holdTime; // Time to hold each color (seconds)
  float fadeTime; // Time to fade between colors (seconds)

private:
  bool active;

  // Animation state
  float progress;            // Current progress through the cycle [0.0, 1.0]
  uint8_t currentColorIndex; // Index of the current color in the list
  bool inFadePhase;          // true if currently fading, false if holding

  // Timing
  unsigned long lastUpdateTime;

  // Hardcoded color list
  static const Color colorList[];
  static const uint8_t numColors;

  // Helper methods
  Color interpolateColors(const Color &fromColor, const Color &toColor, float t);
  uint8_t getNextColorIndex();
};