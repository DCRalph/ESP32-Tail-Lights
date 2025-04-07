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

  // Set initial brightness for all strips
  FastLED.setBrightness(255);
}

LEDStrip *LEDStripManager::getStrip(LEDStripType type)
{
  if (strips.find(type) != strips.end())
  {
    return strips[type].strip;
  }
  return nullptr;
}

CRGB *LEDStripManager::getStripBuffer(LEDStripType type)
{
  if (strips.find(type) != strips.end())
  {
    return strips[type].strip ? strips[type].strip->getFastLEDBuffer() : nullptr;
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
  // We'll use an approach that works without direct access to effect objects
  // Create a map to store pointers to LEDStrip instances that we've processed
  std::map<LEDStrip *, bool> processedStrips;

  // First pass: Mark all strips
  for (auto &pair : strips)
  {
    if (pair.second.strip)
    {
      processedStrips[pair.second.strip] = false;
    }
  }

  // Second pass: Update each strip exactly once
  for (auto &processed : processedStrips)
  {
    if (!processed.second && processed.first)
    {
      // Update this strip's effects
      processed.first->updateEffects();

      // Mark as processed
      processed.second = true;
    }
  }
}

void LEDStripManager::draw()
{
  // Draw all strips
  for (auto &pair : strips)
  {
    if (pair.second.strip)
    {
      LEDStrip *strip = pair.second.strip;
      auto buf = strip->getBuffer();

      for (int i = 0; i < strip->getNumLEDs(); i++)
      {
        strip->getFastLEDBuffer()[i] = CRGB(buf[i].r, buf[i].g, buf[i].b);
      }
    }
  }

  // Send the LED data to the physical LED strips
  FastLED.show();
}
