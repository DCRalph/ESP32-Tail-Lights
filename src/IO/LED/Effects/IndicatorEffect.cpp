#include "IndicatorEffect.h"
#include <Arduino.h>
#include <math.h>

IndicatorEffect::IndicatorEffect(LEDManager *_ledManager, Side side, uint8_t priority,
                                 bool transparent)
    : LEDEffect(_ledManager, priority, transparent),
      side(side),
      indicatorActive(false),
      fadeFactor(0.0f),
      // Default parameters – feel free to adjust.
      blinkCycle(1200),
      fadeInTime(250)
{
  bigIndicator = true;
  activatedTime = 0;
  otherIndicator = nullptr;
  synced = false;

  // Default color is amber/yellow.
  baseR = 255;
  baseG = 120;
  baseB = 0;
}

void IndicatorEffect::setOtherIndicator(IndicatorEffect *otherIndicator)
{
  this->otherIndicator = otherIndicator;
}

IndicatorEffect *IndicatorEffect::getOtherIndicator() const
{
  return otherIndicator;
}

void IndicatorEffect::setActive(bool active)
{
  // If state is unchanged, nothing to do.
  if (indicatorActive == active)
  {
    return;
  }

  indicatorActive = active;

  if (active)
  {
    blinkCycle = 3000;
    // If another indicator exists and is active, sync start times.
    if (otherIndicator != nullptr && otherIndicator->isActive())
    {
      // syncWithOtherIndicator();
    }
    else
    {
      // Otherwise, simply set this activatedTime.
    }
    activatedTime = millis();
  }
  else
  {
    // Clear fade factor if the indicator is turned off.
    fadeFactor = 0.0f;
    activatedTime = 0;

    if (synced == true)
    {
      synced = false;
      if (otherIndicator != nullptr)
        otherIndicator->synced = false;
    }
  }
}

bool IndicatorEffect::isActive() const
{
  return indicatorActive;
}

void IndicatorEffect::syncWithOtherIndicator()
{
  if (otherIndicator == nullptr)
  {
    return;
  }

  activatedTime = otherIndicator->activatedTime;
  synced = true;
  otherIndicator->synced = true;
}

void IndicatorEffect::update()
{
  if (!indicatorActive)
  {
    // Ensure fade factor remains 0 if not active.
    fadeFactor = 0.0f;
    return;
  }
  // Compute where we are within the blink cycle.
  uint64_t currentTime = millis();
  currentTime -= activatedTime;
  uint64_t timeInCycle = currentTime % blinkCycle;

  // For this design, the indicator is "on" only during the fade-in period.
  if (timeInCycle < (blinkCycle / 2))
  {
    // Fade factor increases linearly from 0 to 1 over fadeInTime.
    fadeFactor = (float)timeInCycle / fadeInTime;
    fadeFactor = constrain(fadeFactor, 0.0f, 1.0f);
  }
  else
  {
    // Turn indicator off for the remainder of the blink cycle.
    fadeFactor = 0.0f;
    if (blinkCycle != 1200)
    {
      blinkCycle = 1200;

      if (otherIndicator != nullptr && !synced && otherIndicator->isActive())
      {
        syncWithOtherIndicator();
      }
      else if (!synced)
      {
        activatedTime = millis() - 600;
      }
    }
  }
}

void IndicatorEffect::render(std::vector<Color> &buffer)
{
  // Do nothing if the indicator is inactive or the fade factor is 0.
  if (!indicatorActive)
    return;

  uint16_t regionLength = ledManager->getNumLEDs() / 5;

  if (bigIndicator)
  {
    regionLength = (ledManager->getNumLEDs() / 2) - regionLength;
  }

  if (regionLength <= 1)
  {
    // Handle error or simply return
    return;
  }

  // For the left indicator, the region covers indices 0 to regionLength-1.
  // We want the "inner" edge (the side facing the vehicle) to light first.
  // For the left indicator, that is index (regionLength-1).

  for (uint16_t i = 0; i < regionLength; i++)
  {
    // Compute normalized distance from the inner edge.
    // map i from [0, regionLength] to [0, 1]
    float d = static_cast<float>(regionLength - i) / regionLength;

    // Determine when LED i should begin lighting.
    // Here we use d directly as a threshold: the inner LED (d=0) lights up immediately,
    // while an outer LED (d close to 1) lights only when fadeFactor is nearly 1.
    float finalFactor = 0.0f;
    if (fadeFactor >= d)
    {
      // Check if the denominator is nearly zero to avoid division by zero.
      if (fabs(1.0f - d) < 1e-6)
        finalFactor = 1.0f;
      else
        finalFactor = (fadeFactor - d) / (1.0f - d);
    }

    finalFactor = constrain(finalFactor, 0.0f, 1.0f);

    // Compute final color for this LED.
    uint8_t r = baseR * finalFactor;
    uint8_t g = baseG * finalFactor;
    uint8_t b = baseB * finalFactor;

    // if (i == 0) // Debugging
    // {
    //   Serial.println("fadeFactor: " + String(fadeFactor) + " d: " + String(d) + " finalFactor: " + String(finalFactor));
    //   Serial.println("i: " + String(i) + " r: " + String(r) + " g: " + String(g) + " b: " + String(b));
    // }

    // Set the color in the buffer.
    // should fade in from the center edge
    if (!bigIndicator)
    {
      if (side == LEFT)
      {
        buffer[i] = Color(r, g, b);
      }
      else
      {
        buffer[ledManager->getNumLEDs() - i - 1] = Color(r, g, b);
      }
    }
    else
    {
      uint32_t center = ledManager->getNumLEDs() / 2;
      if (side == LEFT)
      {
        buffer[center - regionLength + i] = Color(r, g, b);
      }
      else
      {
        buffer[center + regionLength - i - 1] = Color(r, g, b);
      }
    }
  }
}
