#include "HeadlightStartupEffect.h"
#include <cmath>
#include <Arduino.h> // For millis()

HeadlightStartupEffect::HeadlightStartupEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      active(false),
      phase(0),
      phase_start(0),
      T0(10.0f),          // 2 seconds for half brightness fill
      T1(0.2f),           // 1 second delay
      T2(1.0f),           // 2 seconds for full brightness fill
      headlight_size(20), // 10 LEDs from each edge by default
      ledsStepSize(2),    // Default to 1 LED at a time
      phase_0_progress(0.0f),
      phase_0_single_led_index(0),
      phase_0_single_led_progress(0.0f),
      phase_1_progress(0.0f),
      phase_2_progress(0.0f)
{
}

void HeadlightStartupEffect::setActive(bool a)
{
  if (active == a)
    return;

  active = a;
  if (active)
  {
    phase = 0;
    phase_start = millis(); // Record the starting time (ms)
    phase_0_progress = 0.0f;
    phase_0_single_led_index = 0;
    phase_0_single_led_progress = 0.0f;
    phase_0_start_single_led = 0;
    phase_1_progress = 0.0f;
    phase_2_progress = 0.0f;
  }
  else
  {
    phase = 0;
    phase_start = 0;
  }
}

bool HeadlightStartupEffect::isActive()
{
  return active;
}

void HeadlightStartupEffect::update(LEDStrip *strip)
{
  if (!active)
    return;

  unsigned long now = millis();
  if (phase_start == 0)
    phase_start = now;

  uint16_t numLEDs = strip->getNumLEDs();
  uint16_t effective_size = std::min<uint16_t>(headlight_size, numLEDs / 2);

  // Convert elapsed time from milliseconds to seconds.
  float elapsed = (now - phase_start) / 1000.0f;

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
  else if (phase == 1) // Phase 1: Filling from outside at full brightness
  {
    phase_1_progress = std::min(elapsed / T1, 1.0f);

    if (elapsed > T1)
    {
      phase = 2;
      phase_start = now;
    }
  }
  else if (phase == 2) // Phase 1: Filling from outside at full brightness
  {
    phase_2_progress = std::min(elapsed / T2, 1.0f);

    if (elapsed > T1)
    {
      phase = 2;
    }
  }
}

void HeadlightStartupEffect::render(LEDStrip *strip, Color *buffer)
{
  if (!active)
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
}