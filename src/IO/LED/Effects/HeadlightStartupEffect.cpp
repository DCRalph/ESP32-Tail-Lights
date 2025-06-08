#include "HeadlightStartupEffect.h"
#include <cmath>
#include <Arduino.h> // For millis()

HeadlightStartupEffect::HeadlightStartupEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      // active(false),
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
      phase_2_progress(0.0f)
{
}

// void HeadlightStartupEffect::setActive(bool a)
// {
//   if (active == a)
//     return;

//   active = a;
//   if (active)
//   {
//     phase = 0;
//     phase_start = millis(); // Record the starting time (ms)
//     phase_0_progress = 0.0f;
//     phase_0_single_led_index = 0;
//     phase_0_single_led_progress = 0.0f;
//     phase_0_start_single_led = 0;
//     phase_1_progress = 0.0f;
//     phase_2_progress = 0.0f;
//   }
//   else
//   {
//     phase = 0;
//     phase_start = 0;
//   }
// }

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
}

void HeadlightStartupEffect::render(LEDStrip *strip, Color *buffer)
{
  // if (!active)
  if (mode == HeadlightStartupEffectMode::Off)
    return;

  // Define colors
  Color color = Color(255, 255, 255);

  Color halfBrightness = color * 0.35f; // Half brightness version

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
        buffer[effective_size - i - 1] = halfBrightness;
        buffer[numLEDs - 1 - (effective_size - i - 1)] = halfBrightness;
      }
    }

    // Highlight the current LED being filled
    for (int i = 0; i < ledsStepSize; i++)
    {
      int ledIndex = single_led_to_light + i;
      if (ledIndex < effective_size)
      {
        buffer[ledIndex] = halfBrightness;
        buffer[numLEDs - 1 - ledIndex] = halfBrightness;
      }
    }
  }
  else if (phase == 1) // Phase 1: Delay
  {
    // fill with half brightness
    for (int i = 0; i < effective_size; i++)
    {
      buffer[i] = halfBrightness;
      buffer[numLEDs - 1 - i] = halfBrightness;
    }
  }
  else if (phase == 2) // Phase 2: Filling from outside at full brightness
  {
    // First, set all LEDs that should be at half brightness
    for (int i = 0; i < effective_size; i++)
    {
      buffer[i] = halfBrightness;               // Left side
      buffer[numLEDs - 1 - i] = halfBrightness; // Right side
    }

    // Then, set LEDs that should be at full brightness based on progress
    int leftLeds = floor(phase_2_progress * effective_size);
    int rightLeds = floor(phase_2_progress * effective_size);

    // Fill left side from the edge inward with full brightness
    for (int i = 0; i < leftLeds; i++)
    {
      buffer[i] = color;
    }

    // Fill right side from the edge inward with full brightness
    for (int i = 0; i < rightLeds; i++)
    {
      buffer[numLEDs - 1 - i] = color;
    }
  }
  else if (phase == 3) // Final phase - all LEDs at full brightness
  {
    // Set all headlight LEDs to full brightness
    for (int i = 0; i < effective_size; i++)
    {
      buffer[i] = color;               // Left side
      buffer[numLEDs - 1 - i] = color; // Right side
    }
  }

  // #########################################################
  // mode == HeadlightStartupEffectMode::CarOn
  // #########################################################
  else if (phase == 10) // Phase 10: filling entire strip to full brightness
  {
    int ledsToAnimate = numLEDs - effective_size * 2;
    int ledsToAnimateSide = ledsToAnimate / 2;

    for (int i = 0; i < numLEDs; i++)
    {
      if (i < effective_size)
        buffer[i] = color; // Left side

      if (i >= numLEDs - effective_size)
        buffer[i] = color; // Right side

      if (i > effective_size && i < effective_size + (ledsToAnimateSide * phase_10_progress))
      {
        buffer[i] = color;
      }

      if (i > numLEDs - effective_size - ledsToAnimateSide && i > numLEDs - effective_size - (ledsToAnimateSide * phase_10_progress))
      {
        buffer[i] = color;
      }
    }
  }
  else if (phase == 11) // full strip full brightness
  {
    for (int i = 0; i < numLEDs; i++)
    {
      buffer[i] = color;
    }
  }

  // #########################################################
  // mode == HeadlightStartupEffectMode::TurningOff
  // #########################################################
  else if (phase == 20)
  { // Phase 20: fade full white strip to off by starting

    int ledsPerSide = numLEDs / 2;
    int ledsOffPerSide = ledsPerSide * phase_20_progress;

    for (int i = 0; i < numLEDs; i++)
    {
      // If the LED is within the off-section on either side, turn it off.
      if (i < ledsOffPerSide || i >= numLEDs - ledsOffPerSide)
      {
        buffer[i] = Color(0, 0, 0);
      }
      else
      {
        buffer[i] = color;
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