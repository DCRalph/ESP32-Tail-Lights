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
      lastUpdateTime(0),
      lastUpdateDuration(0),
      lastDrawDuration(0)
{

  leds = new CRGB[numLEDs];
  memset(leds, 0, numLEDs * sizeof(CRGB));
  ledBuffer = new Color[numLEDs];
  memset(ledBuffer, 0, numLEDs * sizeof(Color));
  switch (ledPin)
  {
  case 1:
    FastLED.addLeds<WS2812, 1, GRB>(leds, numLEDs);
    break;
  case 2:
    FastLED.addLeds<WS2812, 2, GRB>(leds, numLEDs);
    break;
  case 3:
    FastLED.addLeds<WS2812, 3, GRB>(leds, numLEDs);
    break;
  case 4:
    FastLED.addLeds<WS2812, 4, GRB>(leds, numLEDs);
    break;
  case 5:
    FastLED.addLeds<WS2812, 5, GRB>(leds, numLEDs);
    break;
  case 6:
    FastLED.addLeds<WS2812, 6, GRB>(leds, numLEDs);
    break;
  case 7:
    FastLED.addLeds<WS2812, 7, GRB>(leds, numLEDs);
    break;
  case 8:
    FastLED.addLeds<WS2812, 8, GRB>(leds, numLEDs);
    break;
  case 9:
    FastLED.addLeds<WS2812, 9, GRB>(leds, numLEDs);
    break;
  case 10:
    FastLED.addLeds<WS2812, 10, GRB>(leds, numLEDs);
    break;
  case 11:
    FastLED.addLeds<WS2812, 11, GRB>(leds, numLEDs);
    break;
  case 12:
    FastLED.addLeds<WS2812, 12, GRB>(leds, numLEDs);
    break;
  case 13:
    FastLED.addLeds<WS2812, 13, GRB>(leds, numLEDs);
    break;
  case 14:
    FastLED.addLeds<WS2812, 14, GRB>(leds, numLEDs);
    break;
  case 15:
    FastLED.addLeds<WS2812, 15, GRB>(leds, numLEDs);
    break;
  case 16:
    FastLED.addLeds<WS2812, 16, GRB>(leds, numLEDs);
    break;
  case 17:
    FastLED.addLeds<WS2812, 17, GRB>(leds, numLEDs);
    break;
  case 18:
    FastLED.addLeds<WS2812, 18, GRB>(leds, numLEDs);
    break;
  case 19:
    FastLED.addLeds<WS2812, 19, GRB>(leds, numLEDs);
    break;
  case 20:
    FastLED.addLeds<WS2812, 20, GRB>(leds, numLEDs);
    break;
  case 21:
    FastLED.addLeds<WS2812, 21, GRB>(leds, numLEDs);
    break;
  }
}

LEDStrip::~LEDStrip()
{
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
  // Record the total frame start time.
  uint64_t frameStart = micros();

  if (lastUpdateTime == 0)
  {
    lastUpdateTime = millis();
  }

  uint64_t currentTime = millis();
  uint64_t deltaTime = currentTime - lastUpdateTime;
  uint64_t frameInterval = 1000 / fps;

  if (deltaTime >= frameInterval)
  {
    lastUpdateTime = currentTime;
  }
  else
  {
    return;
  }

  // Clear the main LED buffer before applying effects.
  clearBuffer();

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

  // Measure update duration.
  lastUpdateDuration = micros() - frameStart;

  // Optionally, print the measured timings.
  // Serial.print("Update time: ");
  // Serial.print(lastUpdateDuration);
  // Serial.print(" us, Draw time: ");
  // Serial.print(lastDrawDuration);
  // Serial.print(" us, Total frame: ");
  // Serial.print(totalFrameTime);
  // Serial.println(" us");
}

void LEDStrip::draw()
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
}

// void LEDStrip::show()
// {
//   FastLED.show();
// }

CRGB *LEDStrip::getFastLEDBuffer() { return leds; }

Color *LEDStrip::getBuffer() { return ledBuffer; }

void LEDStrip::clearBuffer()
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

void LEDStrip::setFPS(uint16_t fps) { this->fps = fps; }

uint16_t LEDStrip::getFPS() const { return fps; }

void LEDStrip::setFliped(bool _fliped)
{
  fliped = _fliped;
}

bool LEDStrip::getFliped() { return fliped; };

void LEDStrip::setBrightness(uint8_t brightness)
{
  // strip->setBrightness(brightness);
  // FastLED.setBrightness(brightness);
}

uint8_t LEDStrip::getBrightness() const
{
  // return strip->getBrightness();
  return 0;
}

uint64_t LEDStrip::getLastUpdateDuration() const { return lastUpdateDuration; }

uint64_t LEDStrip::getLastDrawDuration() const { return lastDrawDuration; }

uint64_t LEDStrip::getLastFrameTime() const
{
  return lastUpdateDuration + lastDrawDuration;
}
