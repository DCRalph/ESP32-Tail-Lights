#pragma once

#include "../Effects.h"
#include <stdint.h>

enum class HeadlightEffectMode
{
  Off,
  Startup,
  CarOn,
};

class HeadlightEffect : public LEDEffect
{
public:
  HeadlightEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDSegment *segment) override;
  virtual void render(LEDSegment *segment, Color *buffer) override;
  virtual void onDisable() override;


  bool isActive();

  void setOff();
  void setStartup();
  void setCarOn();
  void setMode(HeadlightEffectMode mode);
  void setMode(int mode);
  HeadlightEffectMode getMode();

  void setSplit(bool split);
  bool getSplit();

  void setColor(bool r, bool g, bool b);
  void getColor(bool &r, bool &g, bool &b);

private:
  // bool active;               // Is the effect active?
  HeadlightEffectMode mode;
  int phase;                 // Phase 0: filling from outside at half brightness
                             // Phase 1: filling from outside at full brightness
                             // Phase 2: final steady state (all LEDs at full brightness)
  unsigned long phase_start; // Timestamp (in ms) when the current phase started
  unsigned long lastUpdate;

  // Duration parameters (in seconds)
  float T0; // half brightness fill duration
  float T1; // delay duration
  float T2; // full brightness fill duration

  float T10; // filling entire strip to full brightness duration

  float T20; // fade full white strip to off by starting from the edges and moving inward duration

  float T13; // transition from full strip to split mode duration
  float T14; // transition from split mode to full strip duration

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

  // Phase 13 (transition from full strip to split mode)
  float phase_13_progress;

  // Phase 14 (transition from split mode to full strip)
  float phase_14_progress;

  bool red;
  bool blue;
  bool green;

  bool split;

  Color _getColor(LEDSegment *segment, int i, int size);

  // Rainbow mode variables
  float baseHueCenter; // Base hue at center (0-360)
  float baseHueEdge;   // Base hue at edges (0-360)
  float hueCenter;     // Current hue at center (0-360)
  float hueEdge;       // Current hue at edges (0-360)
  float hueOffset;     // Accumulated hue offset for animation
  float rainbowSpeed;  // Speed of color cycling in degrees per second
};