#pragma once

#include "../Effects.h"
#include <stdint.h>

// Enum to define different speed modes for the police effect
enum class PoliceMode
{
  SLOW,
  FAST
};

class PoliceEffect : public LEDEffect
{
public:
  // Constructs the police light effect
  PoliceEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDStrip *strip) override;
  virtual void render(LEDStrip *strip, Color *buffer) override;

  // Activate or disable the effect
  void setActive(bool active);
  bool isActive() const;

  // Set the speed mode (SLOW or FAST)
  void setMode(PoliceMode mode);
  PoliceMode getMode() const;

private:
  bool active;
  PoliceMode mode;

  // Animation parameters
  unsigned long lastUpdateTime;
  float flashProgress;   // Tracks the current position in the flash cycle (0-1)
  float cycleProgress;   // Tracks which color is currently displayed
  uint16_t currentFlash; // Tracks the current flash count in FAST mode

  // Configuration parameters
  float fastSpeed;                  // Flash cycle speed in seconds (fast mode)
  float slowSpeed;                  // Flash cycle speed in seconds (slow mode)
  uint16_t fastModeFlashesPerCycle; // Number of flashes per cycle

  // Colors
  Color blueColor;
  Color redColor;
};