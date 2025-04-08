#pragma once

#include "../Effects.h"
#include <stdint.h>

class StartupEffect : public LEDEffect
{
public:
  // Constructs the StartupEffect.
  // strip: pointer to the strip.
  // priority, transparent: passed on to the base class.
  StartupEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDStrip *strip) override;
  virtual void render(LEDStrip *strip, Color *buffer) override;
  void setActive(bool active);
  bool isActive();

private:
  bool active;               // Is the effect active?
  uint8_t phase;             // Phase 0: red dot; 1: dash out; 2: dash bounce back;
                             // 3: fill sweep; 4: full hold; 5: split & fade; 6: final steady state
  unsigned long phase_start; // Timestamp (in ms) when the current phase started

  // Duration parameters (in seconds)
  float T0;       // red dot duration
  float T1a;      // dash outward phase duration
  float T1b;      // dash bounce back duration
  float T2;       // fill sweep duration
  float T2_delay; // delay with full red
  float T3;       // split & fade duration

  // Effect parameters
  uint16_t dash_length; // number of LEDs in each dash
  uint16_t edge_stop;   // final red region size at each strip end

  // Progress tracking variables
  float outward_progress; // Progress for outward movement (0 to 1)
  float inward_progress;  // Progress for inward movement (0 to 1)
  float fill_progress;    // Progress for sweep fill (0 to 1)
  float split_progress;   // Progress for split & fade (0 to 1)
};