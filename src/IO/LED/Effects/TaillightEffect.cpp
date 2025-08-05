#include "TaillightEffect.h"
#include <cmath>
#include <Arduino.h>

TaillightEffect::TaillightEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      mode(TaillightEffectMode::Off),
      previousMode(TaillightEffectMode::Off),
      phase(-1),
      phase_start(0),
      lastUpdate(0),
      split(false),
      // Startup timing (from TaillightStartupEffect)
      T_startup_dot(0.0f),
      T_startup_dash_out(0.6f),
      T_startup_dash_in(0.6f),
      T_startup_fill(0.6f),
      T_startup_delay(0.3f),
      T_startup_split(0.6f),
      // Transition timing
      T_mode_transition(0.5f),
      // Effect parameters
      startup_dash_length(15),
      startup_edge_stop(15),
      // Progress variables
      phase_progress(0.0f),
      startup_outward_progress(0.0f),
      startup_inward_progress(0.0f),
      startup_fill_progress(0.0f),
      startup_split_progress(0.0f),
      transition_progress(0.0f)
{
}

bool TaillightEffect::isActive()
{
  return mode != TaillightEffectMode::Off || phase != -1;
}

void TaillightEffect::setOff()
{
  if (mode == TaillightEffectMode::Off)
    return;

  previousMode = mode;
  mode = TaillightEffectMode::Off;
  phase = -1;
  phase_start = 0;
}

void TaillightEffect::setStartup()
{
  if (mode == TaillightEffectMode::Startup)
    return;

  previousMode = mode;
  mode = TaillightEffectMode::Startup;
  phase = 0;
  phase_start = millis();

  // Reset startup progress
  startup_outward_progress = 0.0f;
  startup_inward_progress = 0.0f;
  startup_fill_progress = 0.0f;
  startup_split_progress = 0.0f;
}

void TaillightEffect::setCarOn()
{
  if (mode == TaillightEffectMode::CarOn)
    return;

  previousMode = mode;
  mode = TaillightEffectMode::CarOn;
  phase = 10; // CarOn steady state
  phase_start = millis();
}

void TaillightEffect::setDim()
{
  if (mode == TaillightEffectMode::Dim)
    return;

  previousMode = mode;
  mode = TaillightEffectMode::Dim;
  phase = 20; // Dim steady state
  phase_start = millis();
}

void TaillightEffect::setMode(TaillightEffectMode newMode)
{
  switch (newMode)
  {
  case TaillightEffectMode::Off:
    setOff();
    break;
  case TaillightEffectMode::Startup:
    setStartup();
    break;
  case TaillightEffectMode::CarOn:
    setCarOn();
    break;
  case TaillightEffectMode::Dim:
    setDim();
    break;
  }
}

void TaillightEffect::setMode(int mode)
{
  if (mode == 0)
    setOff();
  else if (mode == 1)
    setStartup();
  else if (mode == 2)
    setCarOn();
  else if (mode == 3)
    setDim();
  else
    setOff();
}

TaillightEffectMode TaillightEffect::getMode()
{
  return mode;
}

void TaillightEffect::setSplit(bool split)
{
  this->split = split;
}

bool TaillightEffect::getSplit()
{
  return split;
}

bool TaillightEffect::isAnimating()
{
  return phase != -1 && mode == TaillightEffectMode::Startup;
}

void TaillightEffect::update(LEDSegment *segment)
{
  if (mode == TaillightEffectMode::Off && phase == -1)
    return;

  unsigned long now = millis();
  if (phase_start == 0)
    phase_start = now;

  unsigned long dtMillis = now - lastUpdate;
  float dtSeconds = dtMillis / 1000.0f;
  lastUpdate = now;

  float elapsed = (now - phase_start) / 1000.0f;

  // Handle mode-specific updates
  switch (mode)
  {
  case TaillightEffectMode::Startup:
    _updateStartupEffect(segment, elapsed);
    break;

  case TaillightEffectMode::CarOn:
  case TaillightEffectMode::Dim:
  case TaillightEffectMode::Off:
    // Steady states - no updates needed
    break;
  }
}

void TaillightEffect::_updateStartupEffect(LEDSegment *segment, float elapsed)
{
  if (phase == 0) // Red dot phase
  {
    if (elapsed >= T_startup_dot)
    {
      phase = 1;
      phase_start = millis();
    }
  }
  else if (phase == 1) // Dash outward
  {
    startup_outward_progress = std::min(elapsed / T_startup_dash_out, 1.0f);
    startup_outward_progress = _easeInOut(startup_outward_progress);

    if (startup_outward_progress >= 1.0f)
    {
      phase = 2;
      phase_start = millis();
    }
  }
  else if (phase == 2) // Dash inward
  {
    startup_inward_progress = std::min(elapsed / T_startup_dash_in, 1.0f);
    startup_inward_progress = _easeInOut(startup_inward_progress);

    if (startup_inward_progress >= 1.0f)
    {
      phase = 3;
      phase_start = millis();
    }
  }
  else if (phase == 3) // Fill sweep
  {
    startup_fill_progress = std::min(elapsed / T_startup_fill, 1.0f);

    if (startup_fill_progress >= 1.0f)
    {
      phase = 4;
      phase_start = millis();
    }
  }
  else if (phase == 4) // Delay with full red
  {
    if (elapsed >= T_startup_delay)
    {
      phase = 5;
      phase_start = millis();
    }
  }
  else if (phase == 5) // Split & fade
  {
    startup_split_progress = std::min(elapsed / T_startup_split, 1.0f);

    if (startup_split_progress >= 1.0f)
    {
      phase = 6; // Final steady state
      phase_start = millis();
    }
  }
}

void TaillightEffect::render(LEDSegment *segment, Color *buffer)
{
  if (mode == TaillightEffectMode::Off && phase == -1)
    return;

  uint16_t numLEDs = segment->getNumLEDs();

  // Handle normal mode rendering
  switch (mode)
  {
  case TaillightEffectMode::Startup:
    _renderStartupEffect(segment, buffer);
    break;

  case TaillightEffectMode::CarOn:
    // CarOn mode is all off by default (as requested)
    for (int i = 0; i < numLEDs; i++)
    {
      buffer[i] = Color(0, 0, 0);
    }
    break;

  case TaillightEffectMode::Dim:
    _renderDimEffect(segment, buffer);
    break;

  case TaillightEffectMode::Off:
  default:
    // All off
    for (int i = 0; i < numLEDs; i++)
    {
      buffer[i] = Color(0, 0, 0);
    }
    break;
  }
}

void TaillightEffect::_renderStartupEffect(LEDSegment *segment, Color *buffer)
{
  uint16_t numLEDs = segment->getNumLEDs();
  float center = numLEDs / 2.0f;
  Color color = _getTaillightColor();

  if (phase == 0) // Red dot at center
  {
    int idx = (int)round(center);
    if (idx >= 0 && idx < numLEDs)
      buffer[idx] = color;
  }
  else if (phase == 1 || phase == 2) // Dash phases
  {
    float progress = (phase == 1) ? startup_outward_progress : (1.0f - startup_inward_progress);

    float left_dash_pos = center - (progress * center);
    float right_dash_pos = center + (progress * center);

    int left_start = (int)round(left_dash_pos);
    int right_start = (int)round(right_dash_pos);

    int left_size = (int)std::min((float)startup_dash_length, center - left_start);
    int right_size = (int)std::min((float)startup_dash_length, right_start - center);

    // Adjust sizes if near the edges
    if (left_start < startup_dash_length)
    {
      left_size -= startup_dash_length - left_start;
      if (left_size < 2)
        left_size = 2;
    }
    if (right_start + startup_dash_length > numLEDs)
    {
      right_size -= (right_start + startup_dash_length) - numLEDs;
      if (right_size < 2)
        right_size = 2;
    }

    for (uint16_t i = 0; i < numLEDs; i++)
    {
      if (i >= left_start && i < left_start + left_size)
        buffer[i] = color;
      else if (i >= right_start - right_size && i < right_start)
        buffer[i] = color;
    }
  }
  else if (phase == 3) // Fill sweep
  {
    float p = startup_fill_progress;
    p = _easeInOut(p);

    float new_dash_length = (p <= 0.2f) ? (p * 5.0f * startup_dash_length) : startup_dash_length;
    p = (1 - ((float)new_dash_length / numLEDs)) * p + ((float)new_dash_length / numLEDs);

    for (int i = 0; i < numLEDs; i++)
    {
      if (i <= center)
      {
        if ((center - i) <= p * center)
          buffer[i] = color;
      }
      else
      {
        if ((i - center) <= p * (numLEDs - 1 - center))
          buffer[i] = color;
      }
    }
  }
  else if (phase == 4) // Full red
  {
    for (int i = 0; i < numLEDs; i++)
    {
      buffer[i] = color;
    }
  }
  else if (phase >= 5) // Split & fade
  {
    float p = startup_split_progress;
    p = _easeInOut(p);

    float left_cutoff = center - p * (center - startup_edge_stop);
    float right_cutoff = center + p * ((numLEDs - startup_edge_stop) - center);

    for (int i = 0; i < numLEDs; i++)
    {
      if (i < left_cutoff || i >= right_cutoff)
        buffer[i] = color;
      else
        buffer[i] = Color(0, 0, 0);
    }
  }
}

void TaillightEffect::_renderDimEffect(LEDSegment *segment, Color *buffer)
{
  uint16_t numLEDs = segment->getNumLEDs();

  // Dim red for all LEDs
  for (uint16_t i = 0; i < numLEDs; i++)
  {
    buffer[i] = Color(20, 0, 0); // Dim red
  }
}

Color TaillightEffect::_getTaillightColor()
{
  return Color(255, 0, 0); // Red for taillights
}

float TaillightEffect::_easeInOut(float t)
{
  return 3 * t * t - 2 * t * t * t;
}

void TaillightEffect::onDisable()
{
  phase = -1;
  mode = TaillightEffectMode::Off;
}