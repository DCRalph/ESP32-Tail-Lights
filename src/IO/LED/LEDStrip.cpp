#include "LEDStrip.h"
#include <algorithm>

// Implementation of the Color structure.
// Range of h: [0, 360), s: [0, 1], v: [0, 1]
// Returns: RGB color with components in the range [0, 255].
Color Color::hsv2rgb(float h, float s, float v)
{
  float r, g, b;
  int i;
  float f, p, q, t;

  if (s == 0)
  {
    r = g = b = v;
    return Color(r * 255, g * 255, b * 255);
  }

  h /= 60; // sector 0 to 5
  i = floor(h);
  f = h - i;
  p = v * (1 - s);
  q = v * (1 - s * f);
  t = v * (1 - s * (1 - f));

  switch (i)
  {
  case 0:
    r = v;
    g = t;
    b = p;
    break;
  case 1:
    r = q;
    g = v;
    b = p;
    break;
  case 2:
    r = p;
    g = v;
    b = t;
    break;
  case 3:
    r = p;
    g = q;
    b = v;
    break;
  case 4:
    r = t;
    g = p;
    b = v;
    break;
  default: // case 5:
    r = v;
    g = p;
    b = q;
    break;
  }

  return Color(r * 255, g * 255, b * 255);
}

Color Color::rgb2hsv(uint8_t r, uint8_t g, uint8_t b)
{
  float h, s, v;
  float max = std::max(r, std::max(g, b));
  float min = std::min(r, std::min(g, b));

  v = max;
  s = max != 0 ? (max - min) / max : 0;

  if (s == 0)
  {
    h = 0;
  }
  else
  {
    h = 60 * (g - b) / (max - min);
  }

  return Color(h, s, v);
}

uint32_t Color::to32Bit() const // wwrrggbb
{
  // return (w << 24) | (r << 16) | (g << 8) | b;
  uint32_t v;
  memcpy(&v, this, sizeof(v));
  return v;
}

const Color Color::WHITE = Color(255, 255, 255);
const Color Color::BLACK = Color(0, 0, 0);
const Color Color::RED = Color(255, 0, 0);
const Color Color::ORANGE = Color(255, 40, 0);
const Color Color::GREEN = Color(0, 255, 0);
const Color Color::BLUE = Color(0, 0, 255);
const Color Color::YELLOW = Color(255, 255, 0);
const Color Color::CYAN = Color(0, 255, 255);
const Color Color::MAGENTA = Color(255, 0, 255);

LEDSegment::LEDSegment(LEDStrip *parentStrip, String name, uint16_t startIndex, uint16_t numLEDs)
    : name(name), startIndex(startIndex), numLEDs(numLEDs), parentStrip(parentStrip), isEnabled(true)
{
  if (startIndex >= parentStrip->numLEDs)
  {
    Serial.println("LEDSegment: startIndex >= parentStrip->numLEDs");
    return;
  }

  if (startIndex + numLEDs - 1 > parentStrip->numLEDs)
  {
    Serial.println("LEDSegment: startIndex + numLEDs - 1 > parentStrip->numLEDs");
    return;
  }

  for (auto segment : parentStrip->segments)
  {
    if (segment->name == name)
    {
      Serial.println("LEDSegment: name already exists");
      return;
    }
    // if (segment->startIndex <= startIndex && segment->startIndex + segment->numLEDs - 1 >= startIndex)
    // {
    //   Serial.println("LEDSegment: segment overlaps with existing segment");
    //   return;
    // }
  }

  // Create mutex for segment buffer access
  segmentMutex = xSemaphoreCreateMutex();

  // ledBuffer = parentStrip->getBuffer() + startIndex;
  ledBuffer = new Color[numLEDs];
  memset(ledBuffer, 0, numLEDs * sizeof(Color));

  parentStrip->segments.push_back(this);
  isEnabled = parentStrip->isEnabled;
}

LEDSegment::LEDSegment(LEDStrip *parentStrip, String name)
    : name(name), startIndex(0), numLEDs(parentStrip->numLEDs), parentStrip(parentStrip), isEnabled(true)
{

  if (parentStrip->numLEDs == 0)
  {
    Serial.println("LEDSegment: parentStrip->numLEDs == 0");
    return;
  }

  if (parentStrip->segments.size() > 0)
  {
    Serial.println("LEDSegment: parentStrip->segments.size() > 0");
    return;
  }

  // Create mutex for segment buffer access
  segmentMutex = xSemaphoreCreateMutex();

  ledBuffer = new Color[numLEDs];
  memset(ledBuffer, 0, numLEDs * sizeof(Color));

  parentStrip->segments.push_back(this);
  isEnabled = parentStrip->isEnabled;
}

LEDSegment::~LEDSegment()
{
  parentStrip->segments.erase(std::remove(parentStrip->segments.begin(), parentStrip->segments.end(), this), parentStrip->segments.end());
  delete[] ledBuffer;

  // Delete mutex
  if (segmentMutex != nullptr)
  {
    vSemaphoreDelete(segmentMutex);
  }
}

Color *LEDSegment::getBuffer()
{
  return ledBuffer;
}

uint16_t LEDSegment::getNumLEDs()
{
  return numLEDs;
}

LEDStrip *LEDSegment::getParentStrip()
{
  return parentStrip;
}

void LEDSegment::setEnabled(bool enabled)
{
  isEnabled = enabled;
  if (!enabled)
    clearBuffer();
}

bool LEDSegment::getEnabled()
{
  return isEnabled;
}

void LEDSegment::addEffect(LEDEffect *effect)
{
  effects.push_back(effect);
  std::sort(effects.begin(), effects.end(),
            [](const LEDEffect *a, const LEDEffect *b)
            {
              return a->getPriority() < b->getPriority();
            });
}

void LEDSegment::removeEffect(LEDEffect *effect)
{
  effects.erase(std::remove(effects.begin(), effects.end(), effect), effects.end());
}

void LEDSegment::updateEffects()
{
  // Take mutex before accessing buffer
  if (xSemaphoreTake(segmentMutex, portMAX_DELAY) == pdTRUE)
  {
    // Start timing the update effects
    String profilerKey = name + "_UpdateEffectsSeg";
    timeProfiler.start(profilerKey);

    clearBufferUnsafe();

#ifdef USE_2_BUFFERS
    Color tempBuffer[numLEDs] = {Color::BLACK};
#endif

    for (auto effect : effects)
    {

      effect->update(this);

#ifdef USE_2_BUFFERS
      // Render the current effect into tempBuffer.
      effect->render(this, tempBuffer);
#else
      // Render the current effect directly into ledBuffer.
      effect->render(this, ledBuffer);
#endif

#ifdef USE_2_BUFFERS
      // Merge the rendered result from tempBuffer into ledBuffer.
      if (effect->isTransparent())
      {
        // For transparent effects, only copy pixels that are not black.
        uint32_t black32 = Color::BLACK.to32Bit();
        for (uint16_t i = 0; i < numLEDs; i++)
        {
          if (tempBuffer[i].to32Bit() != black32)
            ledBuffer[i] = tempBuffer[i];
        }
      }
      else
      {
        // For opaque effects, overwrite the entire ledBuffer.
        ledBuffer = tempBuffer;
      }
#endif
    }

    // Stop timing the update effects
    timeProfiler.stop(profilerKey);

    // Release mutex after buffer access
    xSemaphoreGive(segmentMutex);
  }
}

void LEDSegment::draw()
{
  if (xSemaphoreTake(segmentMutex, portMAX_DELAY) == pdTRUE)
  {
    // if (allBlack(ledBuffer, numLEDs))
    //   return;

    uint16_t endIndex = startIndex + numLEDs - 1;
    uint32_t black32 = Color::BLACK.to32Bit();
    for (uint16_t i = 0; i < numLEDs; i++)
    {

      if (!fliped) // Normal order. copy to parent strip
      {
        if (ledBuffer[i].to32Bit() != black32)
          parentStrip->ledBuffer[startIndex + i] = ledBuffer[i];
      }
      else
      { // Fliped order. copy to parent strip in reverse order
        if (ledBuffer[endIndex - i].to32Bit() != black32)
          parentStrip->ledBuffer[endIndex - i] = ledBuffer[i];
      }
    }
    xSemaphoreGive(segmentMutex);
  }
}

void LEDSegment::clearBuffer()
{
  if (xSemaphoreTake(segmentMutex, portMAX_DELAY) == pdTRUE)
  {
    clearBufferUnsafe();
    xSemaphoreGive(segmentMutex);
  }
}

void LEDSegment::clearBufferUnsafe()
{
  memset(ledBuffer, 0, numLEDs * sizeof(Color));
}

LEDStrip::LEDStrip(uint16_t numLEDs, uint8_t ledPin)
    : numLEDs(numLEDs),
      ledPin(ledPin),
      fliped(false),
      fps(100),
      brightness(255),
      taskHandle(nullptr),
      running(false)
{
  // Create task name based on pin
  taskName = "LED_P" + String(ledPin);

  // Create mutex for buffer access
  bufferMutex = xSemaphoreCreateMutex();

  leds = new CRGB[numLEDs];
  memset(leds, 0, numLEDs * sizeof(CRGB));
  ledBuffer = new Color[numLEDs];
  memset(ledBuffer, 0, numLEDs * sizeof(Color));

  _initController();

  setBrightness(255);

  // Create main segment
  mainSegment = new LEDSegment(this, taskName);
}

LEDStrip::~LEDStrip()
{
  // Stop the task if it's running
  stop();

  // Clean up mutex
  if (bufferMutex != nullptr)
  {
    vSemaphoreDelete(bufferMutex);
  }

  delete[] leds;
  delete[] ledBuffer;
}

void LEDStrip::addEffect(LEDEffect *effect)
{
  if (!mainSegment)
  {
    Serial.println("LEDStrip::addEffect: No main segment found");
    return;
  }
  mainSegment->addEffect(effect);
}

void LEDStrip::removeEffect(LEDEffect *effect)
{
  if (!mainSegment)
  {
    Serial.println("LEDStrip::removeEffect: No main segment found");
    return;
  }
  mainSegment->removeEffect(effect);
}

void LEDStrip::updateEffects()
{
  // Take mutex before accessing buffer
  if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
  {
    // Start timing the update effects
    String profilerKey = taskName + "_UpdateEffects";
    timeProfiler.start(profilerKey);

    clearBufferUnsafe();

    for (auto segment : segments)
    {
      segment->updateEffects();
    }

    timeProfiler.stop(profilerKey);

    xSemaphoreGive(bufferMutex);
  }
}

void LEDStrip::draw()
{
  if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
  {

    if (!isEnabled)
      return;

    for (auto segment : segments)
    {
      segment->draw();
    }

    if (!fliped)
    {
      for (uint16_t i = 0; i < numLEDs; i++)
        leds[i] = CRGB(ledBuffer[i].r, ledBuffer[i].g, ledBuffer[i].b);
    }
    else
    {
      for (uint16_t i = 0; i < numLEDs; i++)
        leds[numLEDs - 1 - i] = CRGB(ledBuffer[i].r, ledBuffer[i].g, ledBuffer[i].b);
    }

    xSemaphoreGive(bufferMutex);
  }
}

void LEDStrip::show()
{
  if (xSemaphoreTake(fastledMutex, portMAX_DELAY) == pdPASS)
  {
    controller->showLeds(brightness);
    xSemaphoreGive(fastledMutex);
  }
}

CRGB *LEDStrip::getFastLEDBuffer() { return leds; }

Color *LEDStrip::getBuffer() { return ledBuffer; }

void LEDStrip::clearBuffer()
{
  if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
  {
    clearBufferUnsafe();
    for (auto segment : segments)
      segment->clearBuffer();
    xSemaphoreGive(bufferMutex);
  }
}

void LEDStrip::clearBufferUnsafe()
{
  memset(ledBuffer, 0, numLEDs * sizeof(Color));
}

LEDStripType LEDStrip::getType() const { return type; }

uint16_t LEDStrip::getNumLEDs() const { return numLEDs; }

void LEDStrip::setFPS(uint16_t fps)
{
  this->fps = fps;
  // If task is running, restart it to apply new FPS
  // if (running)
  // {
  //   stop();
  //   start();
  // }
}

uint16_t LEDStrip::getFPS() const { return fps; }

void LEDStrip::setFliped(bool _fliped)
{
  fliped = _fliped;
}

bool LEDStrip::getFliped() { return fliped; };

void LEDStrip::setBrightness(uint8_t brightness)
{
  this->brightness = brightness;
}

uint8_t LEDStrip::getBrightness() const
{
  return brightness;
}

LEDSegment *LEDStrip::getMainSegment()
{
  if (mainSegment)
    return mainSegment;
  if (segments.size() > 0)
    return segments[0];
  Serial.println("LEDStrip::getMainSegment: No main segment found");
  return nullptr;
}

LEDSegment *LEDStrip::getSegment(String name)
{
  for (auto segment : segments)
  {
    if (segment->name == name)
      return segment;
  }
  return nullptr;
}

LEDSegment *LEDStrip::getSegment(uint16_t index)
{
  if (index >= segments.size())
    return nullptr;
  return segments[index];
}

void LEDStrip::setEnabled(bool enabled)
{
  isEnabled = enabled;
  if (!enabled)
    clearBuffer();
}

bool LEDStrip::getEnabled() const
{
  return isEnabled;
}

// Task control functions
void LEDStrip::start()
{
  // if (!running && taskHandle == nullptr)
  // {
  //   running = true;
  //   xTaskCreatePinnedToCore(ledTask, taskName.c_str(), 4096, this, 1, &taskHandle, 1);
  // }
}

void LEDStrip::stop()
{
  if (running && taskHandle != nullptr)
  {
    running = false;
    vTaskDelete(taskHandle);
    taskHandle = nullptr;
  }
}

bool LEDStrip::isRunning() const
{
  return running;
}

// Static task function
void LEDStrip::ledTask(void *parameter)
{
  LEDStrip *strip = static_cast<LEDStrip *>(parameter);
  strip->_taskLoop();
}

// Task loop implementation
void LEDStrip::_taskLoop()
{
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t frameDelay = pdMS_TO_TICKS(1000 / fps);

  // Serial.println("LEDStrip::_taskLoop: Starting task loop, delay: " + String(frameDelay) + " fps: " + String(fps));
  // vTaskDelay(1000 / portTICK_PERIOD_MS);

  while (running)
  {
    String frameProfilerKey = taskName + "_Frame";
    String drawProfilerKey = taskName + "_Draw";
    String showProfilerKey = taskName + "_Show";

    // Start timing the entire frame
    timeProfiler.start(frameProfilerKey);
    timeProfiler.increment(taskName + "_FPS");

    // Time the draw operation
    timeProfiler.start(drawProfilerKey);
    draw();
    timeProfiler.stop(drawProfilerKey);

    // Time the show operation
    timeProfiler.start(showProfilerKey);
    show();
    timeProfiler.stop(showProfilerKey);

    // Stop timing the entire frame
    timeProfiler.stop(frameProfilerKey);

    // Wait for next frame
    // vTaskDelayUntil(&lastWakeTime, frameDelay);
    vTaskDelay(frameDelay);
  }

  // Clean up task when stopping
  vTaskDelete(nullptr);
}

// fucking stupid fastled init hack
// im probably too redarded to understand why this is good
void LEDStrip::_initController()
{
  switch (ledPin)
  {
  case 1:
    controller = &FastLED.addLeds<WS2812B, 1, GRB>(leds, numLEDs);
    break;
  case 2:
    controller = &FastLED.addLeds<WS2812B, 2, GRB>(leds, numLEDs);
    break;
  case 3:
    controller = &FastLED.addLeds<WS2812B, 3, GRB>(leds, numLEDs);
    break;
  case 4:
    controller = &FastLED.addLeds<WS2812B, 4, GRB>(leds, numLEDs);
    break;
  case 5:
    controller = &FastLED.addLeds<WS2812B, 5, GRB>(leds, numLEDs);
    break;
  case 6:
    controller = &FastLED.addLeds<WS2812B, 6, GRB>(leds, numLEDs);
    break;
  case 7:
    controller = &FastLED.addLeds<WS2812B, 7, GRB>(leds, numLEDs);
    break;
  case 8:
    controller = &FastLED.addLeds<WS2812B, 8, GRB>(leds, numLEDs);
    break;
  case 9:
    controller = &FastLED.addLeds<WS2812B, 9, GRB>(leds, numLEDs);
    break;
  case 10:
    controller = &FastLED.addLeds<WS2812B, 10, GRB>(leds, numLEDs);
    break;
  case 11:
    controller = &FastLED.addLeds<WS2812B, 11, GRB>(leds, numLEDs);
    break;
  case 12:
    controller = &FastLED.addLeds<WS2812B, 12, GRB>(leds, numLEDs);
    break;
  case 13:
    controller = &FastLED.addLeds<WS2812B, 13, GRB>(leds, numLEDs);
    break;
  case 14:
    controller = &FastLED.addLeds<WS2812B, 14, GRB>(leds, numLEDs);
    break;
  case 15:
    controller = &FastLED.addLeds<WS2812B, 15, GRB>(leds, numLEDs);
    break;
  case 16:
    controller = &FastLED.addLeds<WS2812B, 16, GRB>(leds, numLEDs);
    break;
  case 17:
    controller = &FastLED.addLeds<WS2812B, 17, GRB>(leds, numLEDs);
    break;
  case 18:
    controller = &FastLED.addLeds<WS2812B, 18, GRB>(leds, numLEDs);
    break;
  case 19:
    controller = &FastLED.addLeds<WS2812B, 19, GRB>(leds, numLEDs);
    break;
  case 20:
    controller = &FastLED.addLeds<WS2812B, 20, GRB>(leds, numLEDs);
    break;
  case 21:
    controller = &FastLED.addLeds<WS2812B, 21, GRB>(leds, numLEDs);
    break;
  }
}