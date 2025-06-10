#pragma once

#include "../Effects.h"
#include <stdint.h>

enum class TaillightEffectMode
{
  Off,
  Startup,
  CarOn,
  Dim,
};

class TaillightEffect : public LEDEffect
{
public:
  TaillightEffect(uint8_t priority = 0, bool transparent = false);

  virtual void update(LEDStrip *strip) override;
  virtual void render(LEDStrip *strip, Color *buffer) override;

  bool isActive();

  // Mode control methods
  void setOff();
  void setStartup();
  void setCarOn();
  void setDim();
  void setMode(TaillightEffectMode mode);
  void setMode(int mode);
  TaillightEffectMode getMode();

  // State query methods
  bool isAnimating();

private:
  // Core state variables
  TaillightEffectMode mode;
  TaillightEffectMode previousMode;
  int phase;                 // Current animation phase
  unsigned long phase_start; // Timestamp when current phase started
  unsigned long lastUpdate;  // Last update timestamp

  // Timing parameters (in seconds)
  // Startup effect timing
  float T_startup_dot;      // Red dot duration
  float T_startup_dash_out; // Dash outward duration
  float T_startup_dash_in;  // Dash inward duration
  float T_startup_fill;     // Fill sweep duration
  float T_startup_delay;    // Full red hold duration
  float T_startup_split;    // Split & fade duration

  // Transition timing
  float T_mode_transition; // General mode transition duration

  // Effect parameters
  uint16_t startup_dash_length; // Startup dash length
  uint16_t startup_edge_stop;   // Final red region size at strip ends

  // Progress tracking variables
  float phase_progress; // General phase progress (0 to 1)

  // Startup specific progress
  float startup_outward_progress;
  float startup_inward_progress;
  float startup_fill_progress;
  float startup_split_progress;

  // Mode transition progress
  float transition_progress;

  // Helper methods
  Color _getTaillightColor();
  void _updateStartupEffect(LEDStrip *strip, float elapsed);
  void _updateModeTransition(LEDStrip *strip, float elapsed);

  void _renderStartupEffect(LEDStrip *strip, Color *buffer);
  void _renderDimEffect(LEDStrip *strip, Color *buffer);
  void _renderModeTransition(LEDStrip *strip, Color *buffer);

  // Easing functions
  float _easeInOut(float t);
};