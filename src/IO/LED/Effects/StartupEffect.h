#pragma once

#include "../Effects.h"
#include <stdint.h>
#include <vector>

class StartupEffect : public LEDEffect
{
public:
  // Constructs the StartupEffect.
  // ledManager: pointer to the LEDManager.
  // priority, transparent: passed on to the base class.
  StartupEffect(LEDManager *ledManager, uint8_t priority = 0, bool transparent = false);

  virtual void update() override;
  virtual void render(std::vector<Color> &buffer) override;
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

  float center;      // center of the LED strip (as a float)

  // Internal state variables
  float left_dash_pos;  // current left dash position (float index)
  float right_dash_pos; // current right dash position (float index)
  float fill_progress;  // progress for sweep fill (0 to 1)
  float split_progress; // progress for split & fade (0 to 1)
};