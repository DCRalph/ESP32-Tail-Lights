#include "HeadlightEffect.h"
#include <Arduino.h>
#include <cmath>

// Ease in function for smooth lighting transitions
static inline float easeIn(float t)
{
  return t * t;
}

// Ease in/out function using a cosine interpolation for smoother transitions
static inline float easeInOut(float t)
{
  return 0.5f * (1.0f - cosf(t * PI));
}

HeadlightEffect::HeadlightEffect(uint8_t priority,
                                 bool transparent)
    : LEDEffect(priority, transparent),
      lastUpdate(0),
      headlightActive(false),
      split(false),
      ledPerSide(15),
      red(false),
      blue(false),
      green(false),
      intensity(0.0f),
      maxIntensity(1.0f),
      rampDuration(0.3f), // Ramp up time in seconds
      progress(0.0f),
      animationSpeed(1.0f), // Default 1 second for a full animation cycle
      baseHueCenter(1.0f),  // Default center hue is red.
      baseHueEdge(180.0f),
      hueCenter(0.0f), // Start with red
      hueEdge(240.0f), // End with blue
      hueOffset(0.0f),
      rainbowSpeed(120.0f) // 120 degrees per second (full color cycle in 3 seconds)
{
}

void HeadlightEffect::setActive(bool active)
{
  // Only update if state is changing
  if (active == headlightActive)
  {
    return;
  }

  headlightActive = active;

  // Start with current intensity to allow smooth transitions
  if (!headlightActive)
  {
    // Keep the current intensity for fade-out animation
  }
}

bool HeadlightEffect::isActive() const
{
  return headlightActive;
}

void HeadlightEffect::setSplit(bool _split)
{
  split = _split;
}

bool HeadlightEffect::getSplit() const
{
  return split;
}

void HeadlightEffect::setColor(bool r, bool g, bool b)
{
  red = r;
  green = g;
  blue = b;
}

void HeadlightEffect::getColor(bool &r, bool &g, bool &b) const
{
  r = red;
  g = green;
  b = blue;
}

void HeadlightEffect::update(LEDStrip *strip)
{
  unsigned long currentTime = millis();
  if (lastUpdate == 0)
  {
    lastUpdate = currentTime;
    return;
  }

  // Compute elapsed time in seconds
  unsigned long dtMillis = currentTime - lastUpdate;
  float dtSeconds = dtMillis / 1000.0f;
  lastUpdate = currentTime;

  // Update intensity based on active state
  if (headlightActive)
  {
    // Increase intensity to maximum
    intensity += dtSeconds / rampDuration;
    if (intensity > maxIntensity)
    {
      intensity = maxIntensity;
    }

    // Update progress for animation
    progress += dtSeconds / animationSpeed;
    if (progress > 1.0f)
    {
      progress = 1.0f;
    }
  }
  else
  {
    // Decrease intensity to zero
    intensity -= dtSeconds / (rampDuration * 0.5f); // Faster fade-out
    if (intensity < 0.0f)
    {
      intensity = 0.0f;
    }

    // Update progress for animation
    progress -= dtSeconds / (animationSpeed * 0.75f); // Faster fade-out
    if (progress < 0.0f)
    {
      progress = 0.0f;
    }
  }

  // Update rainbow mode animation
  if (red && green && blue && headlightActive)
  {
    // Update the cumulative hue offset based on time
    hueOffset += rainbowSpeed * dtSeconds;
    // Wrap hueOffset to the range [0, 360)
    hueOffset = fmod(hueOffset, 360.0f);

    // Adjust the animated hues based on the offset
    hueCenter = baseHueCenter + hueOffset;
    hueEdge = baseHueEdge + hueOffset;

    // Wrap animated hues within the [0,360) range. use modulo operator to wrap the hue within [0,360).

    hueCenter = fmod(hueCenter, 360.0f);
    hueEdge = fmod(hueEdge, 360.0f);

    // log the hue values to the serial monitor.
    // Serial.print("Hue Center: ");
    // Serial.print(hueCenter);
    // Serial.print(" Hue Edge: ");
    // Serial.print(hueEdge);
    // Serial.print(" Hue Offset: ");
    // Serial.println(hueOffset);
  }
}

void HeadlightEffect::render(LEDStrip *strip, Color *buffer)
{
  if (!headlightActive && intensity <= 0.0f)
  {
    return;
  }

  uint16_t numLEDs = strip->getNumLEDs();

  // Apply easing for smoother transition
  float smoothIntensity = easeIn(intensity);

  // Apply easing to the progress for smoother animation
  float p = easeInOut(progress);

  // Check if in rainbow mode (all three color flags are true)
  bool rainbowMode = red && green && blue;

  if (split)
  {
    // Split mode - only light up the first and last sections
    for (uint16_t i = 0; i < ledPerSide; i++)
    {
      // Calculate animation factor based on position (fading from edges)
      float normDist = (float)i / ledPerSide; // 0 at edge, 1 at center
      float animationFactor = (normDist <= p) ? 1.0f : 0.0f;

      Color c;

      if (rainbowMode)
      {
        // For rainbow mode, interpolate between hueEdge and hueCenter based on position
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
        c = Color::hsv2rgb(hue, 1.0f, smoothIntensity * animationFactor);
      }
      else
      {
        // Standard solid color mode
        uint8_t baseRed = red ? 255 : ((!red && !green && !blue) ? 255 : 0);
        uint8_t baseGreen = green ? 255 : ((!red && !green && !blue) ? 255 : 0);
        uint8_t baseBlue = blue ? 255 : ((!red && !green && !blue) ? 255 : 0);

        // First section (left side)
        c = Color(
            static_cast<uint8_t>(baseRed * smoothIntensity * animationFactor),
            static_cast<uint8_t>(baseGreen * smoothIntensity * animationFactor),
            static_cast<uint8_t>(baseBlue * smoothIntensity * animationFactor));
      }

      // Apply to first section (left side)
      buffer[i] = c;

      // Apply to last section (right side)
      uint16_t endIndex = numLEDs - i - 1;
      buffer[endIndex] = c;
    }
  }
  else
  {
    // Full mode - light up all LEDs with fade-in from edges
    uint16_t mid = numLEDs / 2;

    for (uint16_t i = 0; i < numLEDs; i++)
    {

      // Calculate distance from center (normalized)
      float normDist = (mid > 0)
                           ? fabs(static_cast<int>(i) - static_cast<int>(mid)) /
                                 static_cast<float>(mid)
                           : 0.0f;

      // Animation factor based on position and progress
      float animationFactor = (normDist <= p) ? 1.0f : 0.0f;

      if (rainbowMode)
      {
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

        // Convert HSV to RGB with intensity factor
        buffer[i] = Color::hsv2rgb(hue, 1.0f, smoothIntensity * animationFactor);
      }
      else
      {
        // Standard solid color mode
        uint8_t baseRed = red ? 255 : ((!red && !green && !blue) ? 255 : 0);
        uint8_t baseGreen = green ? 255 : ((!red && !green && !blue) ? 255 : 0);
        uint8_t baseBlue = blue ? 255 : ((!red && !green && !blue) ? 255 : 0);

        buffer[i].r = static_cast<uint8_t>(baseRed * smoothIntensity * animationFactor);
        buffer[i].g = static_cast<uint8_t>(baseGreen * smoothIntensity * animationFactor);
        buffer[i].b = static_cast<uint8_t>(baseBlue * smoothIntensity * animationFactor);
      }
    }
  }
}