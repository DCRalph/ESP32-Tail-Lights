#include "HeadlightEffect.h"
#include <cmath>
#include <Arduino.h> // For millis()

HeadlightEffect::HeadlightEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      mode(HeadlightEffectMode::Off),
      phase(-1),
      phase_start(0),
      T0(15.0f),          // 10 seconds for half brightness fill
      T1(0.2f),           // 0.2 seconds delay
      T2(1.0f),           // 1 second for full brightness fill
      T10(1.0f),          // 1 second for full brightness fill
      T20(1.0f),          // 1 second for full brightness fill
      T13(0.8f),          // 0.8 seconds for transition from full to split
      T14(0.8f),          // 0.8 seconds for transition from split to full
      headlight_size(32), // 10 LEDs from each edge by default
      ledsStepSize(3),    // Default to 1 LED at a time
      phase_0_progress(0.0f),
      phase_0_single_led_index(0),
      phase_0_single_led_progress(0.0f),
      phase_1_progress(0.0f),
      phase_2_progress(0.0f),
      phase_10_progress(0.0f),
      phase_20_progress(0.0f),
      phase_13_progress(0.0f),
      phase_14_progress(0.0f),
      red(false),
      blue(false),
      green(false),
      split(false),
      baseHueCenter(1.0f), // Default center hue is red.
      baseHueEdge(180.0f),
      hueCenter(0.0f), // Start with red
      hueEdge(240.0f), // End with blue
      hueOffset(0.0f),
      rainbowSpeed(120.0f),
      lastUpdate(0)
{
}

bool HeadlightEffect::isActive()
{
  // return active;
  return mode != HeadlightEffectMode::Off && phase != -1;
}

void HeadlightEffect::setOff()
{
  if (mode == HeadlightEffectMode::Off)
    return;

  if (mode == HeadlightEffectMode::CarOn)
  {
    phase = 20;
    phase_start = millis(); // Record the starting time (ms)
    phase_20_progress = 0.0f;
  }
  else
  {
    phase = -1;
    phase_start = 0;
  }
  mode = HeadlightEffectMode::Off;
}

void HeadlightEffect::setStartup()
{
  if (mode == HeadlightEffectMode::Startup)
    return;

  mode = HeadlightEffectMode::Startup;
  phase = 0;
  phase_start = millis(); // Record the starting time (ms)
  phase_0_progress = 0.0f;
  phase_0_single_led_index = 0;
  phase_0_single_led_progress = 0.0f;
  phase_0_start_single_led = 0;
  phase_1_progress = 0.0f;
  phase_2_progress = 0.0f;
}

void HeadlightEffect::setCarOn()
{
  if (mode == HeadlightEffectMode::CarOn)
    return;

  mode = HeadlightEffectMode::CarOn;
  phase = 10;
  phase_start = millis(); // Record the starting time (ms)
  phase_10_progress = 0.0f;
  phase_13_progress = 0.0f;
  phase_14_progress = 0.0f;
}

void HeadlightEffect::setMode(HeadlightEffectMode mode)
{
  if (this->mode == mode)
    return;

  this->mode = mode;
}

void HeadlightEffect::setMode(int mode)
{
  if (mode == 0)
    setOff();
  else if (mode == 1)
    setStartup();
  else if (mode == 2)
    setCarOn();
  else
    setOff();
}

HeadlightEffectMode HeadlightEffect::getMode()
{
  return mode;
}

void HeadlightEffect::setSplit(bool split)
{
  if (this->split == split)
    return;

  this->split = split;

  // If we're in CarOn mode and in steady states, transition between them
  if (mode == HeadlightEffectMode::CarOn)
  {
    if (phase >= 11 && phase <= 14)
    {
      if (split) // Transition from full strip to split mode
      {
        phase = 13;
        phase_start = millis();
        phase_13_progress = 0.0f;
      }
      else if (!split) // Transition from split mode to full strip
      {
        phase = 14;
        phase_start = millis();
        phase_14_progress = 0.0f;
      }
    }
  }
}

bool HeadlightEffect::getSplit()
{
  return split;
}

void HeadlightEffect::setColor(bool r, bool g, bool b)
{
  red = r;
  green = g;
  blue = b;
}

void HeadlightEffect::getColor(bool &r, bool &g, bool &b)
{
  r = red;
  g = green;
  b = blue;
}

void HeadlightEffect::update(LEDSegment *segment)
{
  // if (!active)
  if (mode == HeadlightEffectMode::Off && phase == -1)
    return;

  unsigned long now = millis();
  if (phase_start == 0)
    phase_start = now;

  uint16_t numLEDs = segment->getNumLEDs();
  uint16_t numLEDsHalf = numLEDs / 2;
  if (numLEDs % 2 == 1)
    numLEDsHalf += 1;
  uint16_t effective_size = std::min<uint16_t>(headlight_size, numLEDsHalf);

  // Convert elapsed time from milliseconds to seconds.
  float elapsed = (now - phase_start) / 1000.0f;

  unsigned long dtMillis = now - lastUpdate;
  float dtSeconds = dtMillis / 1000.0f;
  lastUpdate = now;

  // #########################################################
  // mode == HeadlightEffectMode::Startup
  // #########################################################

  if (phase == 0) // Phase 0: Filling from outside at half brightness
  {
    if (phase_0_start_single_led == 0)
    {
      phase_0_start_single_led = now;
    }

    float T0_perLed = T0 / effective_size;
    float elapsed_perLed = (now - phase_0_start_single_led) / 1000.0f;

    phase_0_progress = std::min(elapsed / T0, 1.0f); // probaly not needed
    phase_0_single_led_progress = std::min(elapsed_perLed / T0_perLed, 1.0f);

    int single_led_to_light = static_cast<float>(effective_size) * phase_0_single_led_progress;

    if (elapsed_perLed > T0_perLed || single_led_to_light >= (effective_size - phase_0_single_led_index) - ledsStepSize)
    {
      phase_0_single_led_index += ledsStepSize;
      phase_0_start_single_led = now;
    }

    if (elapsed > T0 || phase_0_single_led_index >= effective_size)
    {
      phase = 1;
      phase_start = now;
    }
  }
  else if (phase == 1) // Phase 1: Delay
  {
    phase_1_progress = std::min(elapsed / T1, 1.0f);

    if (elapsed > T1)
    {
      phase = 2;
      phase_start = now;
    }
  }
  else if (phase == 2) // Phase 2: Filling from outside at full brightness
  {
    phase_2_progress = std::min(elapsed / T2, 1.0f);

    if (elapsed > T2)
    {
      phase = 3;
      phase_start = now;
    }
  }
  else if (phase == 3) // Phase 3: Final phase - all LEDs at full brightness
  {
    // do nothing
  }

  // #########################################################
  // mode == HeadlightEffectMode::CarOn
  // #########################################################
  else if (phase == 10) // Phase 10: filling entire strip to full brightness
  {
    if (split)
    {
      phase = 12;
      phase_start = now;
    }

    phase_10_progress = std::min(elapsed / T10, 1.0f);

    if (elapsed > T10)
    {
      phase = 11;
      phase_start = now;
    }
  }
  else if (phase == 13) // Phase 13: Transition from full strip to split mode
  {
    phase_13_progress = std::min(elapsed / T13, 1.0f);

    if (elapsed > T13)
    {
      phase = 12;
      phase_start = now;
    }
  }
  else if (phase == 14) // Phase 14: Transition from split mode to full strip
  {
    phase_14_progress = std::min(elapsed / T14, 1.0f);

    if (elapsed > T14)
    {
      phase = 11;
      phase_start = now;
    }
  }

  // #########################################################
  // mode == HeadlightEffectMode::TurningOff
  // #########################################################
  else if (phase == 20) // Phase 20: fade full white strip to off by starting from the edges and moving inward
  {
    phase_20_progress = std::min(elapsed / T20, 1.0f);

    if (elapsed > T20)
    {
      phase = -1;
      phase_start = now;
      mode = HeadlightEffectMode::Off;
    }
  }

  if (red && green && blue)
  {
    hueOffset += rainbowSpeed * dtSeconds;
    // Wrap hueOffset to the range [0, 360)
    hueOffset = fmod(hueOffset, 360.0f);

    // Adjust the animated hues based on the offset
    hueCenter = baseHueCenter + hueOffset;
    hueEdge = baseHueEdge + hueOffset;

    // Wrap animated hues within the [0,360) range. use modulo operator to wrap the hue within [0,360).

    hueCenter = fmod(hueCenter, 360.0f);
    hueEdge = fmod(hueEdge, 360.0f);
  }
}

void HeadlightEffect::render(LEDSegment *segment, Color *buffer)
{
  // if (!active)
  if (mode == HeadlightEffectMode::Off && phase == -1)
    return;

  // Define colors

  float halfBrightness = 0.35f; // Half brightness version

  uint16_t numLEDs = segment->getNumLEDs();
  uint16_t numLEDsHalf = numLEDs / 2;
  if (numLEDs % 2 == 1)
    numLEDsHalf += 1;
  uint16_t effective_size = std::min<uint16_t>(headlight_size, numLEDsHalf);

  // For each phase, calculate which LEDs should be lit and at what brightness
  if (phase == 0) // Phase 0: Filling from outside at half brightness
  {
    int leds = floor(phase_0_progress * effective_size);

    int single_led_to_light = static_cast<float>(effective_size) * phase_0_single_led_progress;

    // Fill left side from the edge inward
    if (phase_0_single_led_index > 1)
    {
      for (int i = 0; i < phase_0_single_led_index; i++)
      {
        buffer[effective_size - i - 1] = _getColor(segment, effective_size - i - 1, numLEDsHalf) * halfBrightness;
        buffer[numLEDs - 1 - (effective_size - i - 1)] = buffer[effective_size - i - 1];
      }
    }

    // Highlight the current LED being filled
    for (int i = 0; i < ledsStepSize; i++)
    {
      int ledIndex = single_led_to_light + i;
      if (ledIndex < effective_size)
      {
        buffer[ledIndex] = _getColor(segment, ledIndex, numLEDsHalf) * halfBrightness;
        buffer[numLEDs - 1 - ledIndex] = buffer[ledIndex];
      }
    }
  }
  else if (phase == 1) // Phase 1: Delay
  {
    // fill with half brightness
    for (int i = 0; i < effective_size; i++)
    {
      buffer[i] = _getColor(segment, i, numLEDsHalf) * halfBrightness;
      buffer[numLEDs - 1 - i] = buffer[i];
    }
  }
  else if (phase == 2) // Phase 2: Filling from outside at full brightness
  {
    // First, set all LEDs that should be at half brightness
    for (int i = 0; i < effective_size; i++)
    {
      buffer[i] = _getColor(segment, i, numLEDsHalf) * halfBrightness; // Left side
      buffer[numLEDs - 1 - i] = buffer[i];                           // Right side
    }

    // Then, set LEDs that should be at full brightness based on progress
    int leftLeds = floor(phase_2_progress * effective_size);
    int rightLeds = floor(phase_2_progress * effective_size);

    // Fill left side from the edge inward with full brightness
    for (int i = 0; i < leftLeds; i++)
    {
      buffer[i] = _getColor(segment, i, numLEDsHalf);
    }

    // Fill right side from the edge inward with full brightness
    for (int i = 0; i < rightLeds; i++)
    {
      buffer[numLEDs - 1 - i] = buffer[i];
    }
  }
  else if (phase == 3) // Final phase - all LEDs at full brightness
  {
    // Set all headlight LEDs to full brightness
    for (int i = 0; i < effective_size; i++)
    {
      buffer[i] = _getColor(segment, i, numLEDsHalf); // Left side
      buffer[numLEDs - 1 - i] = buffer[i];          // Right side
    }
  }

  // #########################################################
  // mode == HeadlightEffectMode::CarOn
  // #########################################################
  else if (phase == 10) // Phase 10: filling entire strip to full brightness
  {
    int ledsToAnimate = numLEDs - effective_size * 2;
    int ledsToAnimateSide = ledsToAnimate / 2;

    float p = phase_10_progress;
    p = 3 * p * p - 2 * p * p * p;

    for (int i = 0; i < numLEDs; i++)
    {
      if (i <= effective_size)
        buffer[i] = _getColor(segment, i, numLEDsHalf); // Left side

      if (i >= numLEDs - effective_size - 1)
        buffer[i] = buffer[numLEDs - 1 - i];

      if (i > effective_size && i < effective_size + (ledsToAnimateSide * p))
        buffer[i] = _getColor(segment, i, numLEDsHalf);

      if (i > numLEDs - effective_size - ledsToAnimateSide && i > numLEDs - effective_size - (ledsToAnimateSide * p))
        buffer[i] = buffer[numLEDs - 1 - i];
    }
  }
  else if (phase == 11) // full strip full brightness
  {
    for (int i = 0; i < numLEDsHalf; i++)
    {
      buffer[i] = _getColor(segment, i, numLEDsHalf);
      buffer[numLEDs - 1 - i] = buffer[i];
    }
  }
  else if (phase == 12) // split strip full brightness
  {
    for (int i = 0; i < effective_size; i++)
    {
      buffer[i] = _getColor(segment, i, numLEDsHalf);
      buffer[numLEDs - 1 - i] = buffer[i];
    }
  }
  else if (phase == 13) // Transition from full strip to split mode
  {
    // Animate turning off middle section
    int middleLength = numLEDsHalf - effective_size;

    float p = phase_13_progress;
    p = 3 * p * p - 2 * p * p * p; // Smooth step

    int ledsToTurnOff = middleLength * p;

    // Handle left half of the strip
    for (int i = 0; i < numLEDsHalf; i++)
    {
      if (i < effective_size)
      {
        // Keep headlight section on
        buffer[i] = _getColor(segment, i, numLEDsHalf);
      }
      else if (i >= numLEDsHalf - ledsToTurnOff)
      {
        // Turn off middle section progressively from center outward
        buffer[i] = Color(0, 0, 0);
      }
      else
      {
        // Keep remaining middle section on until its turn
        buffer[i] = _getColor(segment, i, numLEDsHalf);
      }

      // Mirror to right side
      buffer[numLEDs - 1 - i] = buffer[i];
    }
  }
  else if (phase == 14) // Transition from split mode to full strip
  {
    // Animate turning on middle section
    int middleLength = numLEDsHalf - effective_size;

    float p = phase_14_progress;
    p = 3 * p * p - 2 * p * p * p; // Smooth step

    int ledsToTurnOn = middleLength * p;

    // Handle left half of the strip
    for (int i = 0; i < numLEDsHalf; i++)
    {
      if (i < effective_size)
      {
        // Keep headlight section on
        buffer[i] = _getColor(segment, i, numLEDsHalf);
      }
      else if (i < effective_size + ledsToTurnOn)
      {
        // Turn on middle section progressively from headlight outward
        buffer[i] = _getColor(segment, i, numLEDsHalf);
      }
      else
      {
        // Keep remaining middle section off until its turn
        buffer[i] = Color(0, 0, 0);
      }

      // Mirror to right side
      buffer[numLEDs - 1 - i] = buffer[i];
    }
  }

  // #########################################################
  // mode == HeadlightEffectMode::TurningOff
  // #########################################################
  else if (phase == 20)
  { // Phase 20: fade full white strip to off by starting

    float p = phase_20_progress;
    p = 3 * p * p - 2 * p * p * p;

    if (split)
    {
      // Only fade the headlight sections (effective_size LEDs from each edge)
      // Animation goes from inside out when split is true
      int ledsOffPerSide = effective_size * p;

      for (int i = 0; i < numLEDs; i++)
      {
        // Left side headlight section
        if (i < effective_size)
        {
          // Turn off LEDs from inside out: start at (effective_size - 1) and work towards 0
          if (i >= effective_size - ledsOffPerSide)
          {
            buffer[i] = Color(0, 0, 0);
          }
          else
          {
            buffer[i] = _getColor(segment, i, numLEDsHalf);
          }
        }
        // Right side headlight section
        else if (i >= numLEDs - effective_size)
        {
          // Turn off LEDs from inside out: start at (numLEDs - effective_size) and work towards end
          if (i < numLEDs - effective_size + ledsOffPerSide)
          {
            buffer[i] = Color(0, 0, 0);
          }
          else
          {
            buffer[i] = _getColor(segment, i, numLEDsHalf);
          }
        }
        // Middle section remains off when split is true
        else
        {
          buffer[i] = Color(0, 0, 0);
        }
      }
    }
    else
    {
      // Original behavior: fade entire strip from edges
      int ledsPerSide = numLEDsHalf;
      int ledsOffPerSide = ledsPerSide * p;

      for (int i = 0; i < numLEDsHalf; i++)
      {
        // If the LED is within the off-section on either side, turn it off.
        if (i < ledsOffPerSide)
        {
          buffer[i] = Color(0, 0, 0);
          buffer[numLEDs - 1 - i] = buffer[i];
        }
        else
        {
          buffer[i] = _getColor(segment, i, numLEDsHalf);
          buffer[numLEDs - 1 - i] = buffer[i];
        }
      }
    }
  }
  else if (phase == -1) // full strip off
  {
    for (int i = 0; i < numLEDs; i++)
    {
      buffer[i] = Color(0, 0, 0);
    }
  }
}

Color HeadlightEffect::_getColor(LEDSegment *segment, int i, int size)
{
  Color color;
  color.r = red ? 255 : 0;
  color.g = green ? 255 : 0;
  color.b = blue ? 255 : 0;

  bool rainbow = false;
  if (color == Color(255, 255, 255))
    rainbow = true;

  if (color == Color(0, 0, 0))
    color = Color(255, 255, 255);

  uint16_t numLEDs = segment->getNumLEDs();

  if (rainbow)
  {
    float normDist = (float)i / size; // 0 at edge, 1 at center

    float diff = hueEdge - hueCenter;
    if (diff < 0)
    {
      diff += 360.0f;
    }
    // Linearly interpolate along the positive direction.
    float hue = hueCenter - diff * normDist;

    // Normalize the hue to the [0, 360) range.
    hue = fmod(hue, 360.0f);
    if (hue < 0)
      hue += 360.0f;

    return Color::hsv2rgb(hue, 1.0f, 1.0f);
  }
  return color;
}

void HeadlightEffect::onDisable()
{
  phase = -1;
  mode = HeadlightEffectMode::Off;
}