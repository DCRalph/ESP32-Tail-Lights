#include "LEDStripManager.h"
#include "LEDStrip.h" // Include this for full definitions
#include "../../config.h"
#include <Arduino.h>
#include <set> // Add include for std::set
#include <map>
#include "IO/TimeProfiler.h"

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
  drawFPS = 150;
  ledTaskHandle = NULL;
  taskRunning = false;
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
  // Serial.println("LEDStripManager::addLEDStrip: Setting FPS to " + String(drawFPS));
  strips[config.type].strip->setFPS(drawFPS);
  strips[config.type].strip->setBrightness(255);
  // strips[config.type].strip->start();
}

void LEDStripManager::setBrightness(uint8_t brightness)
{
  for (auto &pair : strips)
  {
    if (pair.second.strip)
      pair.second.strip->setBrightness(brightness);
  }
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
  timeProfiler.start("drawEffects", TimeUnit::MICROSECONDS);
  timeProfiler.increment("ledFps");

  // Draw all strips
  for (auto &pair : strips)
  {
    if (pair.second.strip)
    {
      pair.second.strip->draw();
      pair.second.strip->show();
    }
  }

  timeProfiler.stop("drawEffects");
}

// Task management functions
void LEDStripManager::startTask()
{
  if (taskRunning)
  {
    Serial.println("LEDStripManager: Task already running");
    return;
  }

  taskRunning = true;

  // Create the LED task with high priority for smooth LED updates
  xTaskCreatePinnedToCore(
      ledTask,        // Task function
      "LEDStripTask", // Task name
      4096,           // Stack size
      this,           // Parameter passed to task
      5,              // Priority (higher priority for LED updates)
      &ledTaskHandle, // Task handle
      0               // Core to run on
  );

  Serial.println("LEDStripManager: Task started");
}

void LEDStripManager::stopTask()
{
  if (!taskRunning)
  {
    Serial.println("LEDStripManager: Task not running");
    return;
  }

  taskRunning = false;

  if (ledTaskHandle != NULL)
  {
    vTaskDelete(ledTaskHandle);
    ledTaskHandle = NULL;
  }

  Serial.println("LEDStripManager: Task stopped");
}

bool LEDStripManager::isTaskRunning()
{
  return taskRunning;
}

// Static task function for FreeRTOS
void LEDStripManager::ledTask(void *parameter)
{
  LEDStripManager *manager = static_cast<LEDStripManager *>(parameter);

  // Calculate delay in ticks based on FPS
  const TickType_t delayTicks = pdMS_TO_TICKS(1000 / manager->drawFPS);

  Serial.println("LEDStripManager: Task loop started with FPS: " + String(manager->drawFPS));

  while (manager->taskRunning)
  {
    // Draw all strips
    manager->draw();

    // Delay to maintain FPS
    vTaskDelay(delayTicks);
  }

  // Clean up when task ends
  manager->ledTaskHandle = NULL;
  vTaskDelete(NULL);
}
