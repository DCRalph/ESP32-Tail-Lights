#pragma once

#include "config.h"
#include "FastLED.h"
#include <vector>

#define STATUS_LED_COUNT 2

enum class RGB_MODE
{
  Manual,
  Rainbow,
  Pulsing,
  Blink
};

class StatusLed
{
public:
  StatusLed();
  ~StatusLed();

  void begin();
  void show();

  void setLED(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
  void setAllLEDs(uint8_t r, uint8_t g, uint8_t b);

  // Mode-based functionality
  void setMode(RGB_MODE mode);
  void setPrevMode();
  void goBackSteps(uint8_t steps);
  void clearHistory();
  uint8_t getHistorySize();
  RGB_MODE getHistoryAt(uint8_t index); // 0 = most recent
  std::vector<RGB_MODE> getFullHistory();
  RGB_MODE getMode();

  // Manual mode controls
  void off();
  void setColor(uint8_t r, uint8_t g, uint8_t b);
  void setColor(uint32_t color);
  void setColor565(uint16_t color);

  // Animation controls
  void setPulsingColor(uint32_t color);
  void blink(uint32_t color, uint8_t speed, uint8_t count);

  // Speed controls
  void setRainbowSpeed(uint8_t speed) { _rainbowSpeed = speed; }
  void setPulsingSpeed(uint8_t speed) { _pulsingSpeed = speed; }

private:
  // uint8_t _pin;
  bool _initialized;
  CRGB _leds[STATUS_LED_COUNT];
  CLEDController *_controller;
  uint8_t _brightness = 255;

  // Mode management
  RGB_MODE _mode = RGB_MODE::Manual;
  uint8_t _maxModeHistory = 10;
  std::vector<RGB_MODE> _modeHistory;
  uint32_t _prevManualColor = 0;

  // Animation task handles
  TaskHandle_t _rainbowHandle = NULL;
  uint8_t _rainbowSpeed = 5;

  TaskHandle_t _pulsingHandle = NULL;
  uint8_t _pulsingSpeed = 2;
  uint32_t _pulsingColor = 0;

  TaskHandle_t _blinkHandle = NULL;
  uint8_t _blinkSpeed = 2;
  uint8_t _blinkCount = 0;
  uint32_t _blinkColor = 0;

  // Private helper methods
  void _setColor(uint8_t r, uint8_t g, uint8_t b);
  void _setColor(uint32_t color);
  void _updateModeHistory(RGB_MODE oldMode);
  void _setMode(RGB_MODE newMode);

  // Animation methods
  void _startRainbow();
  void _stopRainbow();
  void _rainbow();

  void _startPulsing();
  void _stopPulsing();
  void _pulsing();

  void _startBlink();
  void _stopBlink();
  void _blink();

  // Static task wrapper functions
  static void _rainbowTask(void *pvParameters);
  static void _pulsingTask(void *pvParameters);
  static void _blinkTask(void *pvParameters);
};

extern StatusLed statusLed;