#pragma once

#include "LEDStrip.h"
// #include "FastLED.h"
#include <map>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Structure to store LED strip configuration
struct LEDStripConfig
{
  LEDStripType type;
  LEDStrip *strip;  // Pointer to the LEDManager for this strip
  std::string name; // Human-readable name for the strip

  // Default constructor
  LEDStripConfig() : type(LEDStripType::NONE), strip(nullptr), name("") {}

  LEDStripConfig(LEDStripType _type, LEDStrip *_strip, const std::string &_name) : type(_type),
                                                                                   strip(_strip),
                                                                                   name(_name)
  {
    _strip->type = type;
  }
};

class LEDStripManager
{
public:
  static LEDStripManager *instance;
  static LEDStripManager *getInstance();

  // Constructor
  LEDStripManager();

  // Destructor
  ~LEDStripManager();

  // Initialize the LED strip manager with the different strip configurations
  void begin();

  // Get a LEDManager instance by strip type
  LEDStrip *getStrip(LEDStripType type);

  // Get the LED buffer for a specific strip type
  Color *getStripBuffer(LEDStripType type);

  // Get the number of LEDs for a specific strip type
  uint16_t getStripLEDCount(LEDStripType type);

  // Check if a strip type is enabled
  bool isStripEnabled(LEDStripType type);

  // Add an LED strip configuration
  void addLEDStrip(const LEDStripConfig &config);

  // Set global brightness for all strips
  void setBrightness(uint8_t brightness);

  // update all effects
  void updateEffects();

  // draw all strips
  void draw();

  // Task management functions
  void startTask();
  void stopTask();
  bool isTaskRunning();

private:
  // Map of LED strip configurations by type
  std::map<LEDStripType, LEDStripConfig> strips;

  uint64_t lastDrawTime;
  uint16_t drawFPS;

  // Task-related members
  TaskHandle_t ledTaskHandle;
  bool taskRunning;

  // Static task function for FreeRTOS
  static void ledTask(void *parameter);
};