#pragma once

#include "../Effects.h"
#include <stdint.h>

enum class HeadlightStartupEffectMode
{
  Off,
  TurningOff,
  Startup,
  CarOn,
};

class HeadlightStartupEffect : public LEDEffect
{
public:
  // Constructs the HeadlightStartupEffect.
  // strip: pointer to the strip.
  // priority, transparent: passed on to the base class.
  HeadlightStartupEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDStrip *strip) override;
  virtual void render(LEDStrip *strip, Color *buffer) override;
  // void setActive(bool active);
  bool isActive();

  void setOff();
  void setStartup();
  void setCarOn();
  void setMode(HeadlightStartupEffectMode mode);
  void setMode(int mode);
  HeadlightStartupEffectMode getMode();

private:
  // bool active;               // Is the effect active?
  HeadlightStartupEffectMode mode;
  int phase;                 // Phase 0: filling from outside at half brightness
                             // Phase 1: filling from outside at full brightness
                             // Phase 2: final steady state (all LEDs at full brightness)
  unsigned long phase_start; // Timestamp (in ms) when the current phase started

  // Duration parameters (in seconds)
  float T0; // half brightness fill duration
  float T1; // delay duration
  float T2; // full brightness fill duration

  float T10; // filling entire strip to full brightness duration

  float T20; // fade full white strip to off by starting from the edges and moving inward duration

  // Effect parameters
  uint16_t headlight_size; // number of LEDs in the headlight section from edge

  uint8_t ledsStepSize; // How many LEDs to fill at once

  // Progress tracking variables
  // Phase 0 (half brightness fill)
  float phase_0_progress;
  int phase_0_single_led_index;           // Index of the single LED that we are on
  float phase_0_single_led_progress;      // Progress for half brightness fill (0 to 1)
  unsigned long phase_0_start_single_led; // Timestamp (in ms) when the current phase started

  // Phase 1 (delay)
  float phase_1_progress;

  // Phase 2 (full brightness fill)
  float phase_2_progress; // Progress for full brightness fill (0 to 1)

  // Phase 10 (filling entire strip to full brightness)
  float phase_10_progress;

  // Phase 20 (fade full white strip to off by starting from the edges and moving inward)
  float phase_20_progress;
};