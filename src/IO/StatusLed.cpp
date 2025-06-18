#include "StatusLed.h"
#include "esp_log.h"

static const char *TAG = "StatusLed";

StatusLeds::StatusLeds()
{
  _initialized = false;
  _brightness = 255;
}

StatusLeds::~StatusLeds()
{
  // Nothing to clean up for now
}

void StatusLeds::begin()
{
#ifdef OUTPUT_STATUS_LED_PIN
  if (!_initialized)
  {
    // Initialize FastLED - you may need to adjust the LED type and pin
    _controller = &FastLED.addLeds<WS2812B, OUTPUT_STATUS_LED_PIN, GRB>(_leds, STATUS_LED_COUNT);
    memset(_leds, 0, sizeof(_leds));
    _initialized = true;

    show();
    ESP_LOGI(TAG, "StatusLeds initialized on pin %d with %d LEDs", OUTPUT_STATUS_LED_PIN, STATUS_LED_COUNT);
  }
#endif
}

void StatusLeds::show()
{
  if (_initialized)
  {
    _controller->showLeds(_brightness);
  }
  else
  {
    ESP_LOGE(TAG, "StatusLeds::show() called but not initialized");
  }
}

CRGB *StatusLeds::getLedPtr(uint8_t index)
{
  if (!_initialized)
  {
    ESP_LOGE(TAG, "StatusLeds::getLedPtr() called but not initialized");
    return nullptr;
  }

  if (index >= STATUS_LED_COUNT)
  {
    ESP_LOGE(TAG, "StatusLeds::getLedPtr() index %d out of range (max: %d)", index, STATUS_LED_COUNT - 1);
    return nullptr;
  }

  return &_leds[index];
}

void StatusLeds::setBrightness(uint8_t brightness)
{
  _brightness = brightness;
}

uint8_t StatusLeds::getBrightness() const
{
  return _brightness;
}

void StatusLeds::startShowTask()
{
  xTaskCreatePinnedToCore(_showTask, "StatusLedShowTask", 4096, this, 1, &_showTaskHandle, 0);
}

void StatusLeds::stopShowTask()
{
  vTaskDelete(_showTaskHandle);
  _showTaskHandle = NULL;
}

bool StatusLeds::isShowTaskRunning()
{
  return _showTaskHandle != NULL;
}

void StatusLeds::_showTask(void *pvParameters)
{
  StatusLeds *statusLeds = (StatusLeds *)pvParameters;
  while (true)
  {
    statusLeds->show();
    vTaskDelay(pdMS_TO_TICKS(1000 / 30)); // 50fps
  }
}

// ================================
// StatusLed
// ================================

StatusLed::StatusLed()
{
  _initialized = false;
  _mode = RGB_MODE::Manual;
  _maxModeHistory = 10;
  _ledPtr = nullptr;
  _controller = nullptr;
}

StatusLed::~StatusLed()
{
  // Stop all running tasks
  _stopRainbow();
  _stopPulsing();
  _stopBlink();
}

void StatusLed::begin(StatusLeds *controller, CRGB *ledPtr)
{
  if (controller && ledPtr)
  {
    _controller = controller;
    _ledPtr = ledPtr;
    _initialized = true;

    // Initialize task names
    uint8_t ledIndex = _ledPtr - _controller->getLedPtr(0) + 1;
    snprintf(_rainbowTaskName, sizeof(_rainbowTaskName), "Rainbow_%d", ledIndex);
    snprintf(_pulsingTaskName, sizeof(_pulsingTaskName), "Pulsing_%d", ledIndex);
    snprintf(_blinkTaskName, sizeof(_blinkTaskName), "Blink_%d", ledIndex);

    // Initialize LED to off
    *_ledPtr = CRGB(0, 0, 0);
    // _show();

    ESP_LOGI(TAG, "StatusLed initialized with controller");
  }
}

// ================================
// Private helper methods
// ================================

void StatusLed::_setColor(uint8_t r, uint8_t g, uint8_t b)
{
  if (!_initialized)
  {
    ESP_LOGE(TAG, "StatusLed::_setColor() called but not initialized");
    return;
  }

  if (!_ledPtr)
  {
    ESP_LOGE(TAG, "StatusLed::_setColor() called but LED pointer is null");
    return;
  }

  *_ledPtr = CRGB(r, g, b);
  // _show();
  ESP_LOGD(TAG, "Color set to %d %d %d", r, g, b);
}

void StatusLed::_setColor(uint32_t color)
{
  _setColor((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
}

void StatusLed::setOverideColor(uint8_t r, uint8_t g, uint8_t b)
{
  setOverideColor((r << 16) | (g << 8) | b);
}

void StatusLed::setOverideColor(uint32_t color)
{
  _overideColor = color;

  if (_mode == RGB_MODE::Overide)
  {
    _setColor((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
  }
}

// void StatusLed::_show()
// {
//   if (_controller)
//   {
//     _controller->show();
//   }
//   else
//   {
//     ESP_LOGE(TAG, "StatusLed::_show() called but controller is null");
//   }
// }

void StatusLed::_updateModeHistory(RGB_MODE oldMode)
{
  if (_modeHistory.size() >= _maxModeHistory)
    _modeHistory.erase(_modeHistory.begin());
  _modeHistory.push_back(oldMode);
}

void StatusLed::_setMode(RGB_MODE newMode)
{
  _mode = newMode;

  ESP_LOGI(TAG, "Mode changed to %d", (int)_mode);

  // Stop all animations first
  _stopRainbow();
  _stopPulsing();
  _stopBlink();

  // Start appropriate animation
  switch (_mode)
  {
  case RGB_MODE::Rainbow:
    _startRainbow();
    break;
  case RGB_MODE::Pulsing:
    _startPulsing();
    break;
  case RGB_MODE::Blink:
    _startBlink();
    break;
  case RGB_MODE::Overide:
    _setColor((_overideColor >> 16) & 0xFF, (_overideColor >> 8) & 0xFF, _overideColor & 0xFF);
    break;
  case RGB_MODE::Manual:
    _setColor(_prevManualColor);
    // _show();
    break;
  }
}

// ================================
// Rainbow animation
// ================================

void StatusLed::_startRainbow()
{
  if (_rainbowHandle == NULL)
  {
    xTaskCreate(_rainbowTask, _rainbowTaskName, 4096, this, 1, &_rainbowHandle);
  }
}

void StatusLed::_stopRainbow()
{
  if (_rainbowHandle != NULL)
  {
    vTaskDelete(_rainbowHandle);
    _rainbowHandle = NULL;
  }
}

void StatusLed::_rainbow()
{
  while (_mode == RGB_MODE::Rainbow)
  {
    // Red to Green
    for (uint8_t i = 0; i < 255 && _mode == RGB_MODE::Rainbow; i++)
    {
      _setColor(255 - i, i, 0);
      vTaskDelay(pdMS_TO_TICKS(_rainbowSpeed / 3 / 255));
    }

    // Green to Blue
    for (uint8_t i = 0; i < 255 && _mode == RGB_MODE::Rainbow; i++)
    {
      _setColor(0, 255 - i, i);
      vTaskDelay(pdMS_TO_TICKS(_rainbowSpeed / 3 / 255));
    }

    // Blue to Red
    for (uint8_t i = 0; i < 255 && _mode == RGB_MODE::Rainbow; i++)
    {
      _setColor(i, 0, 255 - i);
      vTaskDelay(pdMS_TO_TICKS(_rainbowSpeed / 3 / 255));
    }
    // _show();
  }
}

void StatusLed::_rainbowTask(void *pvParameters)
{
  StatusLed *statusLed = (StatusLed *)pvParameters;
  statusLed->_rainbow();
  statusLed->_rainbowHandle = NULL;
  vTaskDelete(NULL);
}

// ================================
// Pulsing animation
// ================================

void StatusLed::_startPulsing()
{
  if (_pulsingHandle == NULL)
  {
    xTaskCreate(_pulsingTask, _pulsingTaskName, 4096, this, 1, &_pulsingHandle);
  }
}

void StatusLed::_stopPulsing()
{
  if (_pulsingHandle != NULL)
  {
    vTaskDelete(_pulsingHandle);
    _pulsingHandle = NULL;
  }
}

void StatusLed::_pulsing()
{
  uint8_t r = (_pulsingColor >> 16) & 0xFF;
  uint8_t g = (_pulsingColor >> 8) & 0xFF;
  uint8_t b = _pulsingColor & 0xFF;

  while (_mode == RGB_MODE::Pulsing)
  {
    // Fade from the original color to black
    for (uint8_t i = 0; i < 255 && _mode == RGB_MODE::Pulsing; i++)
    {
      uint8_t red = r - (r * i / 255);
      uint8_t green = g - (g * i / 255);
      uint8_t blue = b - (b * i / 255);

      _setColor(red, green, blue);
      // _show();
      vTaskDelay(pdMS_TO_TICKS(_pulsingSpeed));
    }

    // Fade from black back to the original color
    for (uint8_t i = 0; i < 255 && _mode == RGB_MODE::Pulsing; i++)
    {
      uint8_t red = (r * i / 255);
      uint8_t green = (g * i / 255);
      uint8_t blue = (b * i / 255);

      _setColor(red, green, blue);
      // _show();
      vTaskDelay(pdMS_TO_TICKS(_pulsingSpeed));
    }
  }
}

void StatusLed::_pulsingTask(void *pvParameters)
{
  StatusLed *statusLed = (StatusLed *)pvParameters;
  statusLed->_pulsing();
  statusLed->_pulsingHandle = NULL;
  vTaskDelete(NULL);
}

// ================================
// Blink animation
// ================================

void StatusLed::_startBlink()
{
  if (_blinkHandle == NULL)
  {
    xTaskCreate(_blinkTask, _blinkTaskName, 4096, this, 1, &_blinkHandle);
  }
}

void StatusLed::_stopBlink()
{
  if (_blinkHandle != NULL)
  {
    vTaskDelete(_blinkHandle);
    _blinkHandle = NULL;
  }
}

void StatusLed::_blink()
{
  vTaskDelay(pdMS_TO_TICKS(5)); // Small delay before starting

  for (uint8_t i = 0; i < _blinkCount; i++)
  {
    _setColor(_blinkColor);
    // _show();
    vTaskDelay(pdMS_TO_TICKS(_blinkSpeed * 100)); // Convert to ms

    _setColor(0, 0, 0);
    // _show();
    vTaskDelay(pdMS_TO_TICKS(_blinkSpeed * 100));
  }

  // Return to previous mode
  setPrevMode();
}

void StatusLed::_blinkTask(void *pvParameters)
{
  StatusLed *statusLed = (StatusLed *)pvParameters;
  statusLed->_blink();
  statusLed->_blinkHandle = NULL;
  vTaskDelete(NULL);
}

// ================================
// Public API - Mode Management
// ================================

void StatusLed::setMode(RGB_MODE mode)
{
  if (!_initialized)
  {
    ESP_LOGE(TAG, "StatusLed::setMode() called but not initialized");
    return;
  }

  if (_mode == mode)
  {
    // ESP_LOGI(TAG, "SetMode() called with the same mode");
    return;
  }

  if (_mode == RGB_MODE::Blink)
  {
    ESP_LOGI(TAG, "Not changed because current mode is Blink");
    return;
  }

  ESP_LOGI(TAG, "Mode changed to %d", (int)mode);
  _updateModeHistory(_mode);
  _setMode(mode);
}

void StatusLed::setPrevMode()
{
  if (_modeHistory.size() == 0)
  {
    ESP_LOGI(TAG, "SetPrevMode() called with no history");
    _setMode(RGB_MODE::Manual);
    return;
  }

  ESP_LOGI(TAG, "[PREV] Mode changed to %d", (int)_modeHistory.back());

  _setMode(_modeHistory.back());
  _modeHistory.pop_back();
}

void StatusLed::goBackSteps(uint8_t steps)
{
  if (steps == 0)
  {
    ESP_LOGI(TAG, "goBackSteps() called with 0 steps");
    return;
  }

  if (_modeHistory.size() == 0)
  {
    ESP_LOGI(TAG, "goBackSteps() called with no history");
    _setMode(RGB_MODE::Manual);
    return;
  }

  // Limit steps to available history
  uint8_t actualSteps = (steps > _modeHistory.size()) ? _modeHistory.size() : steps;

  ESP_LOGI(TAG, "[HISTORY] Going back %d steps (requested %d)", actualSteps, steps);

  // Get the target mode (actualSteps back from the end)
  RGB_MODE targetMode = _modeHistory[_modeHistory.size() - actualSteps];

  // Remove the steps we're going back
  for (uint8_t i = 0; i < actualSteps; i++)
  {
    _modeHistory.pop_back();
  }

  _setMode(targetMode);
  // _show();
}

void StatusLed::clearHistory()
{
  ESP_LOGI(TAG, "History cleared (%d entries)", _modeHistory.size());
  _modeHistory.clear();
}

uint8_t StatusLed::getHistorySize()
{
  return _modeHistory.size();
}

RGB_MODE StatusLed::getHistoryAt(uint8_t index)
{
  if (index >= _modeHistory.size())
  {
    ESP_LOGW(TAG, "getHistoryAt() index %d out of range (size: %d)", index, _modeHistory.size());
    return RGB_MODE::Manual; // Default fallback
  }

  // Index 0 = most recent, so we need to reverse the indexing
  return _modeHistory[_modeHistory.size() - 1 - index];
}

std::vector<RGB_MODE> StatusLed::getFullHistory()
{
  // Return a copy of the history (most recent first)
  std::vector<RGB_MODE> reversedHistory;
  for (int i = _modeHistory.size() - 1; i >= 0; i--)
  {
    reversedHistory.push_back(_modeHistory[i]);
  }
  return reversedHistory;
}

RGB_MODE StatusLed::getMode()
{
  return _mode;
}

// ================================
// Public API - Manual Mode Controls
// ================================

void StatusLed::setColor(uint8_t r, uint8_t g, uint8_t b)
{
  if (!_initialized)
  {
    ESP_LOGE(TAG, "StatusLed::setColor() called but not initialized");
    return;
  }

  _prevManualColor = (r << 16) | (g << 8) | b;

  if (_mode != RGB_MODE::Manual)
  {
    ESP_LOGD(TAG, "SetColor() called while not in Manual mode");
    return;
  }

  ESP_LOGD(TAG, "Color set to %d %d %d", r, g, b);
  _setColor(r, g, b);
}

void StatusLed::setColor(uint32_t color)
{
  setColor((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
}

void StatusLed::setColor565(uint16_t color)
{
  setColor((color >> 8) & 0xF8, (color >> 3) & 0xFC, (color << 3) & 0xF8);
}

void StatusLed::setPrevColor(uint8_t r, uint8_t g, uint8_t b)
{
  _prevManualColor = (r << 16) | (g << 8) | b;
}

void StatusLed::off()
{
  if (!_initialized)
  {
    ESP_LOGE(TAG, "StatusLed::off() called but not initialized");
    return;
  }

  _prevManualColor = 0;

  if (_mode != RGB_MODE::Manual)
  {
    ESP_LOGD(TAG, "Off() called while not in Manual mode");
    return;
  }

  _setColor(0, 0, 0);
}

// ================================
// Public API - Animation Controls
// ================================

void StatusLed::setPulsingColor(uint32_t color)
{
  _pulsingColor = color;
}

void StatusLed::setPulsingColor(uint8_t _r, uint8_t _g, uint8_t _b)
{
  _pulsingColor = (_r << 16) | (_g << 8) | _b;
}

void StatusLed::blink(uint32_t color, uint8_t speed, uint8_t count)
{
  if (!_initialized)
  {
    ESP_LOGE(TAG, "StatusLed::blink() called but not initialized");
    return;
  }

  if (_mode == RGB_MODE::Blink)
  {
    ESP_LOGI(TAG, "Blink() called while already in Blink mode");
    return;
  }

  _blinkColor = color;
  _blinkSpeed = speed;
  _blinkCount = count;

  _updateModeHistory(_mode);
  _setMode(RGB_MODE::Blink);
}

// ================================
// Global instances
// ================================

StatusLeds statusLeds;
StatusLed statusLed1;
StatusLed statusLed2;