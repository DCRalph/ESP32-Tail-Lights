#pragma once

#include <stdint.h>
#include <vector>
#include "Effects.h"
#include <Arduino.h>
#include "FastLED.h"
#include "../TimeProfiler.h"

// ####################################
//  Uncomment this line to use double buffering for more complex transparent effects.
//  #define USE_2_BUFFERS
// ####################################

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

  uint32_t to32Bit();

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
};

class LEDEffect; // Forward declaration of effect class

class LEDStrip
{

public:
  // Constructor that allocates an LED buffer based on the number of LEDs.
  LEDStrip(uint16_t numLEDs, uint8_t ledPin);
  virtual ~LEDStrip();

  // Add an effect to the manager.
  // Effects with higher priority override lower-priority ones.
  void addEffect(LEDEffect *effect);

  // Remove an effect.
  void removeEffect(LEDEffect *effect);

  // Update all effects (call update() and then render() for each effect)
  void updateEffects();

  // draw the LED buffer to the physical LED strip
  void draw();
  void show();

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

  void setBrightness(uint8_t brightness);
  uint8_t getBrightness() const;

  // Task control functions
  void start();
  void stop();
  bool isRunning() const;

  // Public mutex for external buffer access
  SemaphoreHandle_t bufferMutex;

  CLEDController *controller;

private:
  friend class LEDStripConfig;
  LEDStripType type;
  uint16_t numLEDs;
  Color *ledBuffer;

  bool fliped;
  uint16_t fps;

  uint8_t ledPin;
  CRGB *leds;
  uint8_t brightness;

  std::vector<LEDEffect *> effects;

  // Task-related members
  TaskHandle_t taskHandle;
  bool running;
  String taskName;
  static void ledTask(void *parameter);
  void taskLoop();

  // Private buffer clear without mutex (for internal use)
  void clearBufferUnsafe();
};
