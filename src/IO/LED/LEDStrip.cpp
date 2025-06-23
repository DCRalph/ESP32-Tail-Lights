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

uint32_t Color::to32Bit() // wwrrggbb
{
  return (w << 24) | (r << 16) | (g << 8) | b;
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

LEDStrip::~LEDStrip()
{
  // Stop the task if it's running
  stop();

  // Clean up mutex
  if (bufferMutex != nullptr)
  {
    vSemaphoreDelete(bufferMutex);
  }

  for (auto effect : effects)
  {
    delete effect;
  }
  effects.clear();
  delete[] leds;
  delete[] ledBuffer;
}

void LEDStrip::addEffect(LEDEffect *effect)
{
  effects.push_back(effect);
  // Sort effects by priority so that lower priority effects are rendered first.
  std::sort(effects.begin(), effects.end(),
            [](const LEDEffect *a, const LEDEffect *b)
            {
              return a->getPriority() < b->getPriority();
            });
}

void LEDStrip::removeEffect(LEDEffect *effect)
{
  effects.erase(std::remove(effects.begin(), effects.end(), effect),
                effects.end());
}

void LEDStrip::updateEffects()
{
  // Take mutex before accessing buffer
  if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
  {
    // Start timing the update effects
    String profilerKey = taskName + "_UpdateEffects";
    timeProfiler.start(profilerKey);

    // Clear the main LED buffer before applying effects.
    clearBufferUnsafe();

#ifdef USE_2_BUFFERS
    // Create a temporary buffer with the same size as the main LED buffer.
    // Using the default constructor for Color will initialize all pixels to black.
    Color *tempBuffer = new Color[numLEDs];
#endif

    // --- Update and render each effect ---
    for (auto effect : effects)
    {
      // Update the effect.
      effect->update(this);

#ifdef USE_2_BUFFERS
      // Clear temporary buffer to black before rendering this effect.
      for (auto &c : tempBuffer)
      {
        c = Color(0, 0, 0);
      }

      // Render the current effect into tempBuffer.
      effect->render(tempBuffer);
#else
      // Render the current effect directly into ledBuffer.
      effect->render(this, ledBuffer);
#endif

#ifdef USE_2_BUFFERS
      // Merge the rendered result from tempBuffer into ledBuffer.
      if (effect->isTransparent())
      {
        // For transparent effects, only copy pixels that are not black.
        for (size_t i = 0; i < ledBuffer.size(); i++)
        {
          if (tempBuffer[i].r != 0 && tempBuffer[i].g != 0 && tempBuffer[i].b != 0)
          {
            ledBuffer[i] = tempBuffer[i];
          }
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
    xSemaphoreGive(bufferMutex);
  }
}

void LEDStrip::draw()
{
  if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
  {
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
  controller->showLeds(brightness);
  // FastLED.show();
}

CRGB *LEDStrip::getFastLEDBuffer() { return leds; }

Color *LEDStrip::getBuffer() { return ledBuffer; }

void LEDStrip::clearBuffer()
{
  if (xSemaphoreTake(bufferMutex, portMAX_DELAY) == pdTRUE)
  {
    for (uint16_t i = 0; i < numLEDs; i++)
    {
      ledBuffer[i].r = 0;
      ledBuffer[i].g = 0;
      ledBuffer[i].b = 0;
    }
    xSemaphoreGive(bufferMutex);
  }
}

void LEDStrip::clearBufferUnsafe()
{
  for (uint16_t i = 0; i < numLEDs; i++)
  {
    ledBuffer[i].r = 0;
    ledBuffer[i].g = 0;
    ledBuffer[i].b = 0;
  }
}

LEDStripType LEDStrip::getType() const { return type; }

uint16_t LEDStrip::getNumLEDs() const { return numLEDs; }

void LEDStrip::setFPS(uint16_t fps)
{
  this->fps = fps;
  // If task is running, restart it to apply new FPS
  if (running)
  {
    stop();
    start();
  }
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

// Task control functions
void LEDStrip::start()
{
  if (!running && taskHandle == nullptr)
  {
    running = true;
    xTaskCreatePinnedToCore(ledTask, taskName.c_str(), 4096, this, 1, &taskHandle, 0);
  }
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
  strip->taskLoop();
}

// Task loop implementation
void LEDStrip::taskLoop()
{
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t frameDelay = pdMS_TO_TICKS(1000 / fps);

  // Serial.println("LEDStrip::taskLoop: Starting task loop, delay: " + String(frameDelay) + " fps: " + String(fps));
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
