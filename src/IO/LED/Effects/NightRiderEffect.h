#pragma once

#include "../Effects.h"

class NightRiderEffect : public LEDEffect
{
public:
  // Constructs the Night Rider effect.
  // speed: movement in LED positions per second.
  // tailLength: tail length in LED units (should be > 0).
  NightRiderEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDSegment *segment) override;
  virtual void render(LEDSegment *segment, Color *buffer) override;
  virtual void onDisable() override;

  // Activate or disable the effect.
  void setActive(bool active);
  bool isActive() const;

  // Sync functionality
  void setSyncData(NightRiderSyncData syncData);
  NightRiderSyncData getSyncData();

  // Customizable parameters:
  float cycleTime;
  float tailLength; // Length of the fading tail in LED units.

private:
  bool active;
  // The current progress in the cycle (0.0 to 1.0).
  float progress;
  // Direction of movement: true = moving forward, false = moving backward.
  bool forward;
  // Last update time (in milliseconds).
  unsigned long lastUpdateTime;

  // Sync support
  bool syncEnabled;
};
