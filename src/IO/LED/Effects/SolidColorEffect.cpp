#include "SolidColorEffect.h"
#include <cmath>

SolidColorEffect::SolidColorEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      active(false),
      colorPreset(SolidColorPreset::WHITE),
      customColor(255, 255, 255)
{
}

void SolidColorEffect::setActive(bool _active)
{
  active = _active;
}

bool SolidColorEffect::isActive() const
{
  return active;
}

void SolidColorEffect::setColorPreset(SolidColorPreset preset)
{
  colorPreset = preset;
}

SolidColorPreset SolidColorEffect::getColorPreset() const
{
  return colorPreset;
}

void SolidColorEffect::setCustomColor(uint8_t r, uint8_t g, uint8_t b)
{
  customColor = Color(r, g, b);
}

void SolidColorEffect::getCustomColor(uint8_t &r, uint8_t &g, uint8_t &b) const
{
  r = customColor.r;
  g = customColor.g;
  b = customColor.b;
}

bool SolidColorEffect::isUsingCustomColor() const
{
  return colorPreset == SolidColorPreset::CUSTOM;
}

void SolidColorEffect::setSyncData(SolidColorSyncData syncData)
{
  active = syncData.active;
  colorPreset = syncData.colorPreset;
  customColor = Color(syncData.customR, syncData.customG, syncData.customB);
}

SolidColorSyncData SolidColorEffect::getSyncData()
{
  SolidColorSyncData syncData = {
      .active = active,
      .colorPreset = colorPreset,
      .customR = customColor.r,
      .customG = customColor.g,
      .customB = customColor.b};
  return syncData;
}

Color SolidColorEffect::getCurrentColor() const
{
  if (colorPreset == SolidColorPreset::CUSTOM)
  {
    return customColor;
  }
  else
  {
    return presetToColor(colorPreset);
  }
}

Color SolidColorEffect::presetToColor(SolidColorPreset preset) const
{
  switch (preset)
  {
  case SolidColorPreset::OFF:
    return Color(0, 0, 0);
  case SolidColorPreset::RED:
    return Color(255, 0, 0);
  case SolidColorPreset::GREEN:
    return Color(0, 255, 0);
  case SolidColorPreset::BLUE:
    return Color(0, 0, 255);
  case SolidColorPreset::WHITE:
    return Color(255, 255, 255);
  case SolidColorPreset::YELLOW:
    return Color(255, 255, 0);
  case SolidColorPreset::CYAN:
    return Color(0, 255, 255);
  case SolidColorPreset::MAGENTA:
    return Color(255, 0, 255);
  case SolidColorPreset::ORANGE:
    return Color(255, 40, 0);
  case SolidColorPreset::PURPLE:
    return Color(128, 0, 128);
  case SolidColorPreset::LIME:
    return Color(50, 205, 50);
  case SolidColorPreset::PINK:
    return Color(255, 0, 75);
  case SolidColorPreset::TEAL:
    return Color(0, 128, 128);
  case SolidColorPreset::INDIGO:
    return Color(75, 0, 130);
  case SolidColorPreset::GOLD:
    return Color(255, 215, 0);
  case SolidColorPreset::SILVER:
    return Color(192, 192, 192);
  case SolidColorPreset::CUSTOM:
    return customColor;
  default:
    return Color(255, 255, 255);
  }
}

void SolidColorEffect::update(LEDStrip *strip)
{
  // Solid color effect doesn't need complex updates - it's static
  // Just return if not active
  if (!active)
    return;
}

void SolidColorEffect::render(LEDStrip *strip, Color *buffer)
{
  if (!active)
    return;

  Color currentColor = getCurrentColor();
  uint16_t numLEDs = strip->getNumLEDs();

  // Fill all LEDs with the current color
  for (uint16_t i = 0; i < numLEDs; i++)
  {
    buffer[i] = currentColor;
  }
}

void SolidColorEffect::onDisable()
{
  active = false;
}