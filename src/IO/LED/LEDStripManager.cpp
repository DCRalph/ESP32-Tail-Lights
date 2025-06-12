#include "LEDStripManager.h"
#include "LEDStrip.h" // Include this for full definitions
#include "../../config.h"
#include <Arduino.h>
#include <set> // Add include for std::set
#include <map>

// Initialize static instance pointer
LEDStripManager *LEDStripManager::instance = nullptr;

LEDStripManager *LEDStripManager::getInstance()
{
  if (!instance)
  {
    instance = new LEDStripManager();
  }
  return instance;
}

LEDStripManager::LEDStripManager()
{
  // Store this instance in the static pointer for callbacks to use
  instance = this;
  drawFPS = 100;
}

LEDStripManager::~LEDStripManager()
{
  // Clean up all LEDManager instances
  for (auto &pair : strips)
  {
    if (pair.second.strip)
    {
      delete pair.second.strip;
      pair.second.strip = nullptr;
    }
  }

  strips.clear();

  // Clear the static instance
  if (instance == this)
  {
    instance = nullptr;
  }
}

void LEDStripManager::begin()
{
  setBrightness(255);
}

LEDStrip *LEDStripManager::getStrip(LEDStripType type)
{
  if (strips.find(type) != strips.end())
  {
    return strips[type].strip;
  }
  return nullptr;
}

Color *LEDStripManager::getStripBuffer(LEDStripType type)
{
  if (strips.find(type) != strips.end() && strips[type].strip)
  {
    return strips[type].strip->getBuffer();
  }
  return nullptr;
}

uint16_t LEDStripManager::getStripLEDCount(LEDStripType type)
{
  if (strips.find(type) != strips.end())
  {
    return strips[type].strip ? strips[type].strip->getNumLEDs() : 0;
  }
  return 0;
}

bool LEDStripManager::isStripEnabled(LEDStripType type)
{
  return strips.count(type) > 0;
}

void LEDStripManager::addLEDStrip(const LEDStripConfig &config)
{

  if (strips.find(config.type) != strips.end())
  {
    Serial.println("LEDStripManager::addLEDStrip: LED strip already exists");
    return;
  }

  // Add to the map of strips
  strips[config.type] = config;
}

void LEDStripManager::setBrightness(uint8_t brightness)
{
  FastLED.setBrightness(brightness);
}

void LEDStripManager::updateEffects()
{

  for (auto &pair : strips)
  {
    if (pair.second.strip)
    {
      pair.second.strip->updateEffects();
    }
  }
}

void LEDStripManager::draw()
{
  if (millis() - lastDrawTime < 1000 / drawFPS)
    return;

  lastDrawTime = millis();

  // Draw all strips
  for (auto &pair : strips)
  {
    if (pair.second.strip)
      pair.second.strip->draw();
      pair.second.strip->show();
  }

}
