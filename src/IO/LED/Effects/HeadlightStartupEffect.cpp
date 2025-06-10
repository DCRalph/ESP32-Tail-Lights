#include "HeadlightStartupEffect.h"
#include <cmath>
#include <Arduino.h> // For millis()

HeadlightStartupEffect::HeadlightStartupEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      mode(HeadlightStartupEffectMode::Off),
      phase(0),
      phase_start(0),
      T0(10.0f),          // 10 seconds for half brightness fill
      T1(0.2f),           // 0.2 seconds delay
      T2(1.0f),           // 1 second for full brightness fill
      T10(1.0f),          // 1 second for full brightness fill
      T20(1.0f),          // 1 second for full brightness fill
      headlight_size(20), // 10 LEDs from each edge by default
      ledsStepSize(2),    // Default to 1 LED at a time
      phase_0_progress(0.0f),
      phase_0_single_led_index(0),
      phase_0_single_led_progress(0.0f),
      phase_1_progress(0.0f),
      phase_2_progress(0.0f),
      phase_10_progress(0.0f),
      phase_20_progress(0.0f),
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

bool HeadlightStartupEffect::isActive()
{
  // return active;
  return mode != HeadlightStartupEffectMode::Off;
}

void HeadlightStartupEffect::setOff()
{
  if (mode == HeadlightStartupEffectMode::Off || mode == HeadlightStartupEffectMode::TurningOff)
    return;

  if (mode == HeadlightStartupEffectMode::CarOn)
  {
    mode = HeadlightStartupEffectMode::TurningOff;
    phase = 20;
    phase_start = millis(); // Record the starting time (ms)
    phase_20_progress = 0.0f;
  }
  else
  {
    mode = HeadlightStartupEffectMode::Off;
    phase = 0;
    phase_start = 0;
  }
}

void HeadlightStartupEffect::setStartup()
{
  if (mode == HeadlightStartupEffectMode::Startup)
    return;

  mode = HeadlightStartupEffectMode::Startup;
  phase = 0;
  phase_start = millis(); // Record the starting time (ms)
  phase_0_progress = 0.0f;
  phase_0_single_led_index = 0;
  phase_0_single_led_progress = 0.0f;
  phase_0_start_single_led = 0;
  phase_1_progress = 0.0f;
  phase_2_progress = 0.0f;
}

void HeadlightStartupEffect::setCarOn()
{
  if (mode == HeadlightStartupEffectMode::CarOn)
    return;

  mode = HeadlightStartupEffectMode::CarOn;
  phase = 10;
  phase_start = millis(); // Record the starting time (ms)
  phase_10_progress = 0.0f;
}

void HeadlightStartupEffect::setMode(HeadlightStartupEffectMode mode)
{
  if (this->mode == mode)
    return;

  this->mode = mode;
}

void HeadlightStartupEffect::setMode(int mode)
{
  if (mode == 0)
    setOff();
  else if (mode == 1)
    setOff();
  else if (mode == 2)
    setStartup();
  else if (mode == 3)
    setCarOn();
  else
    setOff();
}

HeadlightStartupEffectMode HeadlightStartupEffect::getMode()
{
  return mode;
}

void HeadlightStartupEffect::setSplit(bool split)
{
  this->split = split;
}

bool HeadlightStartupEffect::getSplit()
{
  return split;
}

void HeadlightStartupEffect::setColor(bool r, bool g, bool b)
{
  red = r;
  green = g;
  blue = b;
}

void HeadlightStartupEffect::getColor(bool &r, bool &g, bool &b)
{
  r = red;
  g = green;
  b = blue;
}

void HeadlightStartupEffect::update(LEDStrip *strip)
{
  // if (!active)
  if (mode == HeadlightStartupEffectMode::Off)
    return;

  unsigned long now = millis();
  if (phase_start == 0)
    phase_start = now;

  uint16_t numLEDs = strip->getNumLEDs();
  uint16_t effective_size = std::min<uint16_t>(headlight_size, numLEDs / 2);

  // Convert elapsed time from milliseconds to seconds.
  float elapsed = (now - phase_start) / 1000.0f;

  unsigned long dtMillis = now - lastUpdate;
  float dtSeconds = dtMillis / 1000.0f;
  lastUpdate = now;

  // #########################################################
  // mode == HeadlightStartupEffectMode::Startup
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
  // mode == HeadlightStartupEffectMode::CarOn
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

  // #########################################################
  // mode == HeadlightStartupEffectMode::TurningOff
  // #########################################################
  else if (phase == 20) // Phase 20: fade full white strip to off by starting from the edges and moving inward
  {
    phase_20_progress = std::min(elapsed / T20, 1.0f);

    if (elapsed > T20)
    {
      phase = 21;
      phase_start = now;
      mode = HeadlightStartupEffectMode::Off;
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

void HeadlightStartupEffect::render(LEDStrip *strip, Color *buffer)
{
  // if (!active)
  if (mode == HeadlightStartupEffectMode::Off)
    return;

  // Define colors

  float halfBrightness = 0.35f; // Half brightness version

  uint16_t numLEDs = strip->getNumLEDs();
  uint16_t effective_size = std::min<uint16_t>(headlight_size, numLEDs / 2);

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
        buffer[effective_size - i - 1] = _getColor(strip, effective_size - i - 1, effective_size) * halfBrightness;
        buffer[numLEDs - 1 - (effective_size - i - 1)] = buffer[effective_size - i - 1];
      }
    }

    // Highlight the current LED being filled
    for (int i = 0; i < ledsStepSize; i++)
    {
      int ledIndex = single_led_to_light + i;
      if (ledIndex < effective_size)
      {
        buffer[ledIndex] = _getColor(strip, ledIndex, effective_size) * halfBrightness;
        buffer[numLEDs - 1 - ledIndex] = buffer[ledIndex];
      }
    }
  }
  else if (phase == 1) // Phase 1: Delay
  {
    // fill with half brightness
    for (int i = 0; i < effective_size; i++)
    {
      buffer[i] = _getColor(strip, i, effective_size) * halfBrightness;
      buffer[numLEDs - 1 - i] = buffer[i];
    }
  }
  else if (phase == 2) // Phase 2: Filling from outside at full brightness
  {
    // First, set all LEDs that should be at half brightness
    for (int i = 0; i < effective_size; i++)
    {
      buffer[i] = _getColor(strip, i, effective_size) * halfBrightness; // Left side
      buffer[numLEDs - 1 - i] = buffer[i];                              // Right side
    }

    // Then, set LEDs that should be at full brightness based on progress
    int leftLeds = floor(phase_2_progress * effective_size);
    int rightLeds = floor(phase_2_progress * effective_size);

    // Fill left side from the edge inward with full brightness
    for (int i = 0; i < leftLeds; i++)
    {
      buffer[i] = _getColor(strip, i, effective_size);
    }

    // Fill right side from the edge inward with full brightness
    for (int i = 0; i < rightLeds; i++)
    {
      buffer[numLEDs - 1 - i] = _getColor(strip, numLEDs - 1 - i, effective_size);
    }
  }
  else if (phase == 3) // Final phase - all LEDs at full brightness
  {
    // Set all headlight LEDs to full brightness
    for (int i = 0; i < effective_size; i++)
    {
      buffer[i] = _getColor(strip, i, effective_size); // Left side
      buffer[numLEDs - 1 - i] = buffer[i];             // Right side
    }
  }

  // #########################################################
  // mode == HeadlightStartupEffectMode::CarOn
  // #########################################################
  else if (phase == 10) // Phase 10: filling entire strip to full brightness
  {
    int ledsToAnimate = numLEDs - effective_size * 2;
    int ledsToAnimateSide = ledsToAnimate / 2;

    float p = phase_10_progress;
    p = 3 * p * p - 2 * p * p * p;

    for (int i = 0; i < numLEDs; i++)
    {
      if (i < effective_size)
        buffer[i] = _getColor(strip, i, effective_size); // Left side

      if (i >= numLEDs - effective_size)
        buffer[i] = _getColor(strip, i, effective_size); // Right side

      if (i > effective_size && i < effective_size + (ledsToAnimateSide * p))
      {
        buffer[i] = _getColor(strip, i, effective_size);
      }

      if (i > numLEDs - effective_size - ledsToAnimateSide && i > numLEDs - effective_size - (ledsToAnimateSide * p))
      {
        buffer[i] = _getColor(strip, i, effective_size);
      }
    }
  }
  else if (phase == 11) // full strip full brightness
  {
    for (int i = 0; i < numLEDs; i++)
    {
      buffer[i] = _getColor(strip, i, numLEDs);
    }
  }
  else if (phase == 12) // full strip full brightness
  {
    for (int i = 0; i < effective_size; i++)
    {
      buffer[i] = _getColor(strip, i, effective_size);
      buffer[numLEDs - 1 - i] = buffer[i];
    }
  }

  // #########################################################
  // mode == HeadlightStartupEffectMode::TurningOff
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
            buffer[i] = _getColor(strip, i, effective_size);
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
            buffer[i] = _getColor(strip, i, effective_size);
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
      int ledsPerSide = numLEDs / 2;
      int ledsOffPerSide = ledsPerSide * p;

      for (int i = 0; i < numLEDs; i++)
      {
        // If the LED is within the off-section on either side, turn it off.
        if (i < ledsOffPerSide || i >= numLEDs - ledsOffPerSide)
        {
          buffer[i] = Color(0, 0, 0);
        }
        else
        {
          buffer[i] = _getColor(strip, i, numLEDs);
        }
      }
    }
  }
  else if (phase == 21) // full strip off
  {
    for (int i = 0; i < numLEDs; i++)
    {
      buffer[i] = Color(0, 0, 0);
    }
  }
}

Color HeadlightStartupEffect::_getColor(LEDStrip *strip, int i, int size)
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

  uint16_t numLEDs = strip->getNumLEDs();

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