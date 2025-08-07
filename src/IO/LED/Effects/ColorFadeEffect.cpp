#include "ColorFadeEffect.h"
#include <cmath>

// Hardcoded color list - attractive color sequence
const Color ColorFadeEffect::colorList[] = {
    Color::RED,      // Red
    Color::ORANGE,   // Orange
    Color::YELLOW,   // Yellow
    Color::GREEN,    // Green
    Color::CYAN,     // Cyan
    Color::BLUE,     // Blue
    Color::MAGENTA,  // Magenta
    Color::WHITE     // White
};

const uint8_t ColorFadeEffect::numColors = sizeof(colorList) / sizeof(colorList[0]);

ColorFadeEffect::ColorFadeEffect(uint8_t priority, bool transparent)
    : LEDEffect(priority, transparent),
      active(false),
      holdTime(2.0f),    // Default: hold each color for 2 seconds
      fadeTime(1.0f),    // Default: fade over 1 second
      progress(0.0f),
      currentColorIndex(0),
      inFadePhase(false),
      lastUpdateTime(0)
{
  name = "ColorFade";
}

void ColorFadeEffect::setActive(bool _active)
{
  active = _active;
  if (_active && lastUpdateTime == 0)
  {
    lastUpdateTime = SyncManager::syncMillis();
    progress = 0.0f;
    currentColorIndex = 0;
    inFadePhase = false;
  }
}

bool ColorFadeEffect::isActive() const
{
  return active;
}

void ColorFadeEffect::setSyncData(ColorFadeSyncData syncData)
{
  active = syncData.active;
  holdTime = syncData.holdTime;
  fadeTime = syncData.fadeTime;
  progress = syncData.progress;
  currentColorIndex = syncData.currentColorIndex;
  inFadePhase = syncData.inFadePhase;
  
  // Clamp currentColorIndex to valid range
  if (currentColorIndex >= numColors) {
    currentColorIndex = 0;
  }
}

ColorFadeSyncData ColorFadeEffect::getSyncData()
{
  ColorFadeSyncData syncData = {
      .active = active,
      .holdTime = holdTime,
      .fadeTime = fadeTime,
      .progress = progress,
      .currentColorIndex = currentColorIndex,
      .inFadePhase = inFadePhase
  };
  return syncData;
}

void ColorFadeEffect::update(LEDSegment *segment)
{
  // Return if the effect is not active
  if (!active)
    return;

  unsigned long currentTime = SyncManager::syncMillis();
  
  // Initialize last update time if needed
  if (lastUpdateTime == 0)
  {
    lastUpdateTime = currentTime;
    return;
  }

  // Calculate elapsed time in seconds
  unsigned long dtMillis = currentTime - lastUpdateTime;
  float dtSeconds = dtMillis / 1000.0f;
  lastUpdateTime = currentTime;

  // Update progress based on current phase
  float phaseTime = inFadePhase ? fadeTime : holdTime;
  
  if (phaseTime > 0.0f) {
    progress += dtSeconds / phaseTime;
  }

  // Check if we need to transition to the next phase
  if (progress >= 1.0f) {
    if (inFadePhase) {
      // Finished fading, move to next color and start holding
      currentColorIndex = getNextColorIndex();
      inFadePhase = false;
    } else {
      // Finished holding, start fading to next color
      inFadePhase = true;
    }
    progress = 0.0f;
  }
}

void ColorFadeEffect::render(LEDSegment *segment, Color *buffer)
{
  if (!active)
    return;

  uint16_t numLEDs = segment->getNumLEDs();
  Color currentColor;

  if (inFadePhase) {
    // We're fading between currentColorIndex and the next color
    uint8_t nextColorIndex = getNextColorIndex();
    currentColor = interpolateColors(colorList[currentColorIndex], colorList[nextColorIndex], progress);
  } else {
    // We're holding on the current color
    currentColor = colorList[currentColorIndex];
  }

  // Set all LEDs to the current color
  for (uint16_t i = 0; i < numLEDs; i++) {
    buffer[i] = currentColor;
  }
}

void ColorFadeEffect::onDisable()
{
  active = false;
}

Color ColorFadeEffect::interpolateColors(const Color& fromColor, const Color& toColor, float t)
{
  // Clamp t to [0, 1] range
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;

  // Linear interpolation between colors
  uint8_t r = fromColor.r + (toColor.r - fromColor.r) * t;
  uint8_t g = fromColor.g + (toColor.g - fromColor.g) * t;
  uint8_t b = fromColor.b + (toColor.b - fromColor.b) * t;
  uint8_t w = fromColor.w + (toColor.w - fromColor.w) * t;

  return Color(r, g, b, w);
}

uint8_t ColorFadeEffect::getNextColorIndex()
{
  return (currentColorIndex + 1) % numColors;
}