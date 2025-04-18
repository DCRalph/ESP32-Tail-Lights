#pragma once

#include <stdint.h>
#include <vector>
#include "Effects.h"
#include <Arduino.h>
#include "FastLED.h"
// #include "LEDStripManager.h"
// ####################################
//  Uncomment this line to use double buffering for more complex transparent effects.
//  #define USE_2_BUFFERS
// ####################################

// A simple structure representing an RGB color.

// Enum to identify different LED strip types
enum class LEDStripType
{
  NONE,
  HEADLIGHT,
  TAILLIGHT,
  UNDERGLOW,
  INTERIOR,
  // Add more types as needed
};
struct Color
{
  uint8_t r;
  uint8_t g;
  uint8_t b;

  Color() : r(0), g(0), b(0) {}
  Color(uint8_t red, uint8_t green, uint8_t blue)
      : r(red), g(green), b(blue) {}

  static Color hsv2rgb(float h, float s, float v);

  static const Color WHITE;
  static const Color BLACK;
  static const Color RED;
  static const Color GREEN;
  static const Color BLUE;
  static const Color YELLOW;
  static const Color CYAN;
  static const Color MAGENTA;

  bool operator==(const Color &other) const
  {
    return r == other.r && g == other.g && b == other.b;
  }

  bool operator!=(const Color &other) const
  {
    return !(*this == other);
  }

  /**
   * @brief Multiplies the color by a scalar value
   * @param scalar A floating-point value between 0 and 1
   * @return A new Color object with RGB components scaled by the scalar value
   */
  Color operator*(float scalar) const
  {
    return Color(r * scalar, g * scalar, b * scalar);
  }
};

class LEDEffect; // Forward declaration of effect class

class LEDStrip
{

public:
  // Constructor that allocates an LED buffer based on the number of LEDs.
  LEDStrip(uint16_t numLEDs);
  virtual ~LEDStrip();

  // Add an effect to the manager.
  // Effects with higher priority override lower-priority ones.
  void addEffect(LEDEffect *effect);

  // Remove an effect.
  void removeEffect(LEDEffect *effect);

  // Update all effects (call update() and then render() for each effect)
  void updateEffects();

  // Access the internal FastLED buffer.
  CRGB *getFastLEDBuffer();

  // Access the internal LED buffer.
  Color *getBuffer();

  // Clear the LED buffer (set all LEDs to black/off)
  void clearBuffer();

  // Get the type of the LED strip.
  LEDStripType getType() const;

  // Get the total number of LEDs.
  uint16_t getNumLEDs() const;

  void setFPS(uint16_t fps);
  uint16_t getFPS() const;

  void setFliped(bool _fliped);
  bool getFliped();

  // Return the last frame's update duration (in microseconds).
  uint64_t getLastUpdateDuration() const;

  // Return the last frame's draw duration (in microseconds).
  uint64_t getLastDrawDuration() const;

  // Return combined frame duration (update + draw).
  uint64_t getLastFrameTime() const;

  void disableALlEffects();

private:
  friend class LEDStripConfig;
  LEDStripType type;
  uint16_t numLEDs;
  bool fliped;
  uint16_t fps;
  uint64_t lastUpdateTime;
  CRGB *leds; // fastled buffer
  Color *ledBuffer;
  std::vector<LEDEffect *> effects;

  // Time (in microseconds) it took for the last update and draw calls.
  uint64_t lastUpdateDuration;
  uint64_t lastDrawDuration;
};
