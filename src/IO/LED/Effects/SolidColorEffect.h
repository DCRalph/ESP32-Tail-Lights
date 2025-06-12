#pragma once

#include "../Effects.h"
#include <stdint.h>

class SolidColorEffect : public LEDEffect
{
public:
  // Constructs the solid color effect
  SolidColorEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDStrip *strip) override;
  virtual void render(LEDStrip *strip, Color *buffer) override;
  virtual void onDisable() override;

  // Activate or disable the effect
  void setActive(bool active);
  bool isActive() const;

  // Set color by preset
  void setColorPreset(SolidColorPreset preset);
  SolidColorPreset getColorPreset() const;

  // Set custom color (RGB values 0-255)
  void setCustomColor(uint8_t r, uint8_t g, uint8_t b);
  void getCustomColor(uint8_t &r, uint8_t &g, uint8_t &b) const;

  bool isUsingCustomColor() const;

  // Sync functionality
  void setSyncData(SolidColorSyncData syncData);
  SolidColorSyncData getSyncData();

  // Get the actual color being displayed
  Color getCurrentColor() const;

private:
  bool active;
  SolidColorPreset colorPreset;
  Color customColor;

  // Convert preset to Color
  Color presetToColor(SolidColorPreset preset) const;
};