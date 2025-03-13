#include "RGBEffect.h"
#include <cmath>

RGBEffect::RGBEffect(LEDManager *_ledManager, uint8_t priority, bool transparent)
    : LEDEffect(_ledManager, priority, transparent),
      active(false),
      baseHueCenter(1.0f), // Default center hue is red.
      baseHueEdge(270.0f), // Default edge hue is violet.
      speed(180.0f),       // Default speed: 60 degrees per second.
      hueOffset(0.0f),
      lastUpdateTime(0)
{
  // Initialize the animated hues to the base values.
  hueCenter = baseHueCenter;
  hueEdge = baseHueEdge;
}

void RGBEffect::setActive(bool _active)
{
  active = _active;
  if (_active && lastUpdateTime == 0)
  {
    lastUpdateTime = millis();
  }
}

bool RGBEffect::isActive() const
{
  return active;
}

void RGBEffect::update()
{
  // Return if the effect is not active.
  if (!active)
    return;

  unsigned long currentTime = millis();
  // Initialize last update if needed.
  if (lastUpdateTime == 0)
  {
    lastUpdateTime = currentTime;
    return;
  }

  // Calculate elapsed time in seconds.
  unsigned long dtMillis = currentTime - lastUpdateTime;
  float dtSeconds = dtMillis / 1000.0f;
  lastUpdateTime = currentTime;

  // Update the cumulative hue offset.
  // speed is in degrees per second.
  hueOffset += speed * dtSeconds;
  // Wrap hueOffset to the range [0, 360).
  hueOffset = fmod(hueOffset, 360.0f);

  // Adjust the animated hues based on the base values and the offset.
  hueCenter = baseHueCenter + hueOffset;
  hueEdge = baseHueEdge + hueOffset;

  // Wrap animated hues within the [0,360) range. use modulo operator to wrap the hue within [0,360).

  hueCenter = fmod(hueCenter, 360.0f);
  hueEdge = fmod(hueEdge, 360.0f);

  // // log the hue values to the serial monitor.
  // Serial.print("Hue Center: ");
  // Serial.print(hueCenter);
  // Serial.print(" Hue Edge: ");
  // Serial.print(hueEdge);
  // Serial.print(" Hue Offset: ");
  // Serial.println(hueOffset);
}
void RGBEffect::render(std::vector<Color> &buffer)
{
  if (!active)
    return;

  uint16_t num = ledManager->getNumLEDs();
  uint16_t mid = num / 2;

  // For every LED, compute the normalized distance from the center.
  // At the center (distance = 0) use hueCenter; at the edges (distance = 1) use hueEdge.
  for (uint16_t i = 0; i < num; i++)
  {
    float normDist = (mid > 0)
                         ? fabs(static_cast<int>(i) - static_cast<int>(mid)) /
                               static_cast<float>(mid)
                         : 0.0f;

    // Compute the positive angular difference.
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

    Color c = Color::hsv2rgb(hue, 1.0f, 1.0f);
    buffer[i] = c;
  }
}