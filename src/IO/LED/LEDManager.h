#pragma once

#include <stdint.h>
#include <vector>
#include "Effects.h"
#include <Arduino.h>

// ####################################
//  Uncomment this line to use double buffering for more complex transparent effects.
//  #define USE_2_BUFFERS
// ####################################

// A simple structure representing an RGB color.
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

  Color operator*(float scalar) const
  {
    return Color(r * scalar, g * scalar, b * scalar);
  }
};

class LEDEffect; // Forward declaration of effect class

class LEDManager
{
public:
  // Constructor that allocates an LED buffer based on the number of LEDs.
  LEDManager(uint16_t numLEDs);
  virtual ~LEDManager();

  // Add an effect to the manager.
  // Effects with higher priority override lower-priority ones.
  void addEffect(LEDEffect *effect);

  // Remove an effect.
  void removeEffect(LEDEffect *effect);

  // Update all effects (call update() and then render() for each effect)
  void updateEffects();

  // Output the current LED buffer to the physical LED strip.
  // This function calls a user-specified callback if one has been set.
  virtual void draw();

  // Define the callback for drawing.
  void setDrawFunction(void (*drawFunction)());

  // Access the internal LED buffer.
  std::vector<Color> &getBuffer();

  // Clear the LED buffer (set all LEDs to black/off)
  void clearBuffer();

  // Get the total number of LEDs.
  uint16_t getNumLEDs() const;

  void setFPS(uint16_t fps);
  uint16_t getFPS() const;

  // Return the last frame's update duration (in microseconds).
  uint64_t getLastUpdateDuration() const;

  // Return the last frame's draw duration (in microseconds).
  uint64_t getLastDrawDuration() const;

  // Return combined frame duration (update + draw).
  uint64_t getLastFrameTime() const;

  void disableALlEffects();

protected:
  uint16_t numLEDs;
  uint16_t fps;
  uint64_t lastUpdateTime;
  std::vector<Color> ledBuffer;
  std::vector<LEDEffect *> effects;

  // Callback function pointer, set by the user.
  void (*drawCallback)();

  // Time (in microseconds) it took for the last update and draw calls.
  uint64_t lastUpdateDuration;
  uint64_t lastDrawDuration;
};
