#pragma once

#include <stdint.h>
#include <vector>
#include "Effects.h"
#include <Arduino.h>
#include "FastLED.h"
#include "../TimeProfiler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ####################################
//  Uncomment this line to use double buffering for more complex transparent effects.
// #define USE_2_BUFFERS
// ####################################

class LEDEffect; // Forward declaration of effect class
class LEDStrip;

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
  uint8_t w;

  Color() : r(0), g(0), b(0), w(0) {}
  Color(uint8_t red, uint8_t green, uint8_t blue)
      : r(red), g(green), b(blue), w(0) {}
  Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
      : r(red), g(green), b(blue), w(white) {}

  static Color hsv2rgb(float h, float s, float v);
  static Color rgb2hsv(uint8_t r, uint8_t g, uint8_t b);

  uint32_t to32Bit() const;

  static const Color WHITE;
  static const Color BLACK;
  static const Color RED;
  static const Color ORANGE;
  static const Color GREEN;
  static const Color BLUE;
  static const Color YELLOW;
  static const Color CYAN;
  static const Color MAGENTA;

  bool operator==(const Color &other) const
  {
    return r == other.r && g == other.g && b == other.b && w == other.w;
  }

  bool operator!=(const Color &other) const
  {
    return !(*this == other);
  }

  /**
   * @brief Multiplies the color by a scalar value
   * @param scalar A floating-point value between 0 and 1
   * @return A new Color object with RGBW components scaled by the scalar value
   */
  Color operator*(float scalar) const
  {
    return Color(r * scalar, g * scalar, b * scalar, w * scalar);
  }

  void print() const;
  static void print(const Color &color);
  static void print(const Color *colors, uint16_t numLEDs);
  static void print(const CRGB *colors, uint16_t numLEDs);
};

// [[gnu::always_inline]]
// bool allBlack(const Color *colors, uint16_t numLEDs) noexcept
// {
//   const uint32_t black32 = Color::BLACK.to32Bit();

//   for (uint16_t i = 0; i < numLEDs; ++i)
//   {
//     if (colors[i].to32Bit() != black32)
//     {
//       return false;
//     }
//   }
//   return true; // every element was black
// }

class LEDSegment
{
private:
  Color *ledBuffer;
  uint16_t numLEDs;
  LEDStrip *parentStrip;

  std::vector<LEDEffect *> effects;

public:
  LEDSegment(LEDStrip *_parentStrip, String _name, uint16_t _startIndex, uint16_t _numLEDs);
  LEDSegment(LEDStrip *_parentStrip, String _name);
  ~LEDSegment();

  Color *getBuffer();
  uint16_t getNumLEDs();
  LEDStrip *getParentStrip();
  void setEnabled(bool enabled);
  bool getEnabled();

  void addEffect(LEDEffect *effect);
  void removeEffect(LEDEffect *effect);
  uint16_t effectCount();

  void updateEffects();

  void draw();
  void clearBuffer();

  // Mutex for segment buffer access
  SemaphoreHandle_t segmentMutex;

  String name;
  uint16_t startIndex;
  bool isEnabled;
  bool fliped;

private:
  // Private buffer clear without mutex (for internal use)
  void clearBufferUnsafe();
};

class LEDStrip
{

public:
  LEDStrip(String _name, uint16_t _numLEDs, uint8_t _ledPin);
  virtual ~LEDStrip();

  void addEffect(LEDEffect *effect);
  void removeEffect(LEDEffect *effect);

  void updateEffects();

  void draw(); // copy the buffer to the FastLED buffer
  void show(); // show the FastLED buffer

  String getName();

  CRGB *getFastLEDBuffer();
  Color *getBuffer();
  void clearBuffer();

  LEDStripType getType() const;

  uint16_t getNumLEDs() const;

  void setFliped(bool _fliped);
  bool getFliped();

  void setBrightness(uint8_t brightness);
  uint8_t getBrightness() const;

  LEDSegment *getMainSegment();
  LEDSegment *getSegment(String name);
  LEDSegment *getSegment(uint16_t index);
  std::vector<LEDSegment *> getSegments();

  void setEnabled(bool enabled);
  bool getEnabled() const;

  void setActive(bool active);
  bool getActive() const;

  // Public mutex for external buffer access
  SemaphoreHandle_t bufferMutex;

  CLEDController *controller;

private:
  friend class LEDStripConfig;
  friend class LEDSegment;
  LEDStripType type;
  uint16_t numLEDs;
  Color *ledBuffer;
  LEDSegment *mainSegment;
  std::vector<LEDSegment *> segments;
  String name;

  bool isEnabled; // led strip is enabled in the config
  bool isActive;  // led strip is currently enabled. active can be set to false to disable/turn off the strip independently of the config. strip can not be set to active if it is not enabled in the config.

  bool fliped;

  uint8_t ledPin;
  CRGB *leds;
  uint8_t brightness;

  void _initController();

  // Private buffer clear without mutex (for internal use)
  void clearBufferUnsafe();
};
