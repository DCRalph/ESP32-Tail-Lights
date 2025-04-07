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

const Color Color::WHITE = Color(255, 255, 255);
const Color Color::BLACK = Color(0, 0, 0);
const Color Color::RED = Color(255, 0, 0);
const Color Color::GREEN = Color(0, 255, 0);
const Color Color::BLUE = Color(0, 0, 255);
const Color Color::YELLOW = Color(255, 255, 0);
const Color Color::CYAN = Color(0, 255, 255);
const Color Color::MAGENTA = Color(255, 0, 255);

LEDStrip::LEDStrip(uint16_t numLEDs)
    : numLEDs(numLEDs),
      ledBuffer(numLEDs),
      fps(100),
      lastUpdateTime(0),
      lastUpdateDuration(0),
      lastDrawDuration(0)
{
  leds = new CRGB[numLEDs];
}

LEDStrip::~LEDStrip()
{
  for (auto effect : effects)
  {
    delete effect;
  }
  effects.clear();
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
  std::vector<Color> tempBuffer(ledBuffer.size());
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

CRGB *LEDStrip::getFastLEDBuffer() { return leds; }

std::vector<Color> &LEDStrip::getBuffer() { return ledBuffer; }

void LEDStrip::clearBuffer()
{
  for (auto &led : ledBuffer)
  {
    led.r = 0;
    led.g = 0;
    led.b = 0;
  }
}

uint16_t LEDStrip::getNumLEDs() const { return numLEDs; }

void LEDStrip::setFPS(uint16_t fps) { this->fps = fps; }

uint16_t LEDStrip::getFPS() const { return fps; }

uint64_t LEDStrip::getLastUpdateDuration() const { return lastUpdateDuration; }

uint64_t LEDStrip::getLastDrawDuration() const { return lastDrawDuration; }

uint64_t LEDStrip::getLastFrameTime() const
{
  return lastUpdateDuration + lastDrawDuration;
}

void LEDStrip::disableALlEffects()
{
  // for(auto e : effects){
  //   e->setActive(false);
  // }
}