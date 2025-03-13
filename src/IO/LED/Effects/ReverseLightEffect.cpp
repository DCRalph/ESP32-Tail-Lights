#include "ReverseLightEffect.h"
#include <Arduino.h>
#include <math.h>

// Ease in/out function using a cosine interpolation.
// t: input value between 0 and 1.
static float easeInOut(float t)
{
  return 0.5f * (1.0f - cosf(t * PI));
}

ReverseLightEffect::ReverseLightEffect(LEDManager *_ledManager, uint8_t priority, bool transparent)
    : LEDEffect(_ledManager, priority, transparent),
      active(false),
      animationSpeed(1.0f), // default 2 seconds for a full animation cycle
      startTime(0)
{
  progress = 0.0f;
}

void ReverseLightEffect::setActive(bool _active)
{
  if (active == _active)
    return;

  active = _active;

  if (active)
  {
    startTime = millis();
    // progress = 0.0f;
  }
  else
  {
    // progress = 1.0f;
  }
}

bool ReverseLightEffect::isActive() const
{
  return active;
}

bool ReverseLightEffect::isAnimating() const
{
  return active || progress > 0.0f;
}

void ReverseLightEffect::update()
{
  // Get the current time in milliseconds.
  unsigned long currentTime = millis();

  // Calculate elapsed time in seconds.
  float deltaTime = (currentTime - startTime) / 1000.0f;

  // Update startTime for the next update call.
  startTime = currentTime;

  // Determine how much progress to change.
  float deltaProgress = deltaTime / animationSpeed;

  // Increase or decrease progress based on whether the effect is active.
  if (active)
  {
    progress += deltaProgress;
    if (progress > 1.0f)
    {
      progress = 1.0f;
    }
  }
  else
  {
    progress -= deltaProgress;
    if (progress < 0.0f)
    {
      progress = 0.0f;
    }
  }
}

void ReverseLightEffect::render(std::vector<Color> &buffer)
{
  // If not active and progress is 0, do not modify the buffer.
  if (!active && progress <= 0.0f)
  {
    return;
  }

  uint16_t numLEDs = ledManager->getNumLEDs();
  // Compute the center index. This works for even or odd numbers,
  // though you might adjust if you need a more symmetrical behavior.
  float center = round(numLEDs / 2.0f);

  // Apply an easing function to the progress (for smoother animation)
  float p = easeInOut(progress);

  // For a hard threshold effect we interpret p as the normalized radius
  // (relative to the center) within which LEDs are fully lit.
  // When p==0, no LED is lit. When p==1, even the farthest LED is lit.
  for (uint16_t i = 0; i < numLEDs; i++)
  {
    float distance = fabs(center - i);
    // Compute normalized distance.
    float normalizedDistance = distance / center;

    // If the LED’s normalized distance is less than or equal to p,
    // then it is fully lit (brightness = 1), otherwise off (brightness = 0).
    float brightness = (normalizedDistance <= p) ? 1.0f : 0.0f;

    buffer[i] = Color(255 * brightness,
                      255 * brightness,
                      255 * brightness);
  }
}
